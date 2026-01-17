#include "bluetooth/device_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <dbus/dbus.h>

#define BLUEZ_SERVICE "org.bluez"
#define OBJECT_MANAGER_INTERFACE "org.freedesktop.DBus.ObjectManager"
#define ADAPTER_INTERFACE "org.bluez.Adapter1"
#define DEVICE_INTERFACE "org.bluez.Device1"

/* Internal device manager structure */
struct DeviceManager {
    DeviceManagerConfig config;
    DBusConnection* conn;
    pthread_mutex_t mutex;
    GHashTable* devices;           // key: MAC address, value: BluetoothDevice*
    bool scanning;
    bool running;
    pthread_t thread;
    char* adapter_path;
};

/* Convert DBus string to device type */
static DeviceType get_device_type_from_class(uint32_t class) {
    uint32_t major_class = (class >> 8) & 0x1F;
    uint32_t minor_class = (class >> 2) & 0x3F;
    
    switch (major_class) {
        case 0x04:  // Audio/Video
            if (minor_class == 0x04)  // Headset
                return DEVICE_AUDIO_SINK;
            else if (minor_class == 0x08)  // Hands-free
                return DEVICE_AUDIO_SINK;
            else if (minor_class == 0x03)  // Microphone
                return DEVICE_AUDIO_SOURCE;
            return DEVICE_AUDIO_SINK;
            
        case 0x05:  // Peripheral
            switch (minor_class) {
                case 0x01:  // Keyboard
                    return DEVICE_KEYBOARD;
                case 0x02:  // Mouse
                    return DEVICE_MOUSE;
                case 0x03:  // Keyboard/Mouse combo
                    return DEVICE_KEYBOARD;
            }
            return DEVICE_INPUT;
            
        case 0x02:  // Phone
            return DEVICE_PHONE;
            
        case 0x01:  // Computer
            return DEVICE_COMPUTER;
            
        default:
            return DEVICE_UNKNOWN;
    }
}

/* Parse DBus message for device properties */
static BluetoothDevice* parse_device_properties(DBusMessageIter *iter) {
    BluetoothDevice* device = calloc(1, sizeof(BluetoothDevice));
    if (!device) return NULL;
    
    DBusMessageIter dict_iter;
    dbus_message_iter_recurse(iter, &dict_iter);
    
    while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry_iter;
        char *key = NULL;
        
        dbus_message_iter_recurse(&dict_iter, &entry_iter);
        dbus_message_iter_get_basic(&entry_iter, &key);
        
        dbus_message_iter_next(&entry_iter);
        
        if (strcmp(key, "Address") == 0) {
            char *address;
            dbus_message_iter_get_basic(&entry_iter, &address);
            strncpy(device->address, address, sizeof(device->address) - 1);
        } else if (strcmp(key, "Name") == 0) {
            char *name;
            dbus_message_iter_get_basic(&entry_iter, &name);
            strncpy(device->name, name, sizeof(device->name) - 1);
        } else if (strcmp(key, "Alias") == 0) {
            char *alias;
            dbus_message_iter_get_basic(&entry_iter, &alias);
            strncpy(device->alias, alias, sizeof(device->alias) - 1);
        } else if (strcmp(key, "Class") == 0) {
            uint32_t class;
            dbus_message_iter_get_basic(&entry_iter, &class);
            device->class = class;
            device->type = get_device_type_from_class(class);
        } else if (strcmp(key, "Paired") == 0) {
            dbus_bool_t paired;
            dbus_message_iter_get_basic(&entry_iter, &paired);
            device->paired = paired;
        } else if (strcmp(key, "Trusted") == 0) {
            dbus_bool_t trusted;
            dbus_message_iter_get_basic(&entry_iter, &trusted);
            device->trusted = trusted;
        } else if (strcmp(key, "Blocked") == 0) {
            dbus_bool_t blocked;
            dbus_message_iter_get_basic(&entry_iter, &blocked);
            device->blocked = blocked;
        } else if (strcmp(key, "RSSI") == 0) {
            int16_t rssi;
            dbus_message_iter_get_basic(&entry_iter, &rssi);
            device->rssi = (int8_t)rssi;
        }
        
        dbus_message_iter_next(&dict_iter);
    }
    
    // If alias is empty, copy name to alias
    if (strlen(device->alias) == 0 && strlen(device->name) > 0) {
        strncpy(device->alias, device->name, sizeof(device->alias) - 1);
    }
    
    return device;
}

/* DBus error handler */
static void handle_dbus_error(DBusError *error, DeviceManager *manager) {
    if (manager->config.on_error) {
        manager->config.on_error(ERR_DBUS, error->message, manager->config.user_data);
    }
    dbus_error_free(error);
}

/* Get default Bluetooth adapter */
static char* get_default_adapter(DeviceManager* manager) {
    DBusError error;
    DBusMessage *msg, *reply;
    char *adapter_path = NULL;
    
    dbus_error_init(&error);
    
    msg = dbus_message_new_method_call(BLUEZ_SERVICE,
                                       "/",
                                       OBJECT_MANAGER_INTERFACE,
                                       "GetManagedObjects");
    
    if (!msg) {
        if (manager->config.on_error) {
            manager->config.on_error(ERR_DBUS, "Failed to create DBus message", 
                                   manager->config.user_data);
        }
        return NULL;
    }
    
    reply = dbus_connection_send_with_reply_and_block(manager->conn, msg, 1000, &error);
    dbus_message_unref(msg);
    
    if (!reply) {
        handle_dbus_error(&error, manager);
        return NULL;
    }
    
    DBusMessageIter iter, array_iter;
    dbus_message_iter_init(reply, &iter);
    
    if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
        dbus_message_iter_recurse(&iter, &array_iter);
        
        while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_DICT_ENTRY) {
            DBusMessageIter entry_iter, value_iter;
            char *object_path = NULL;
            
            dbus_message_iter_recurse(&array_iter, &entry_iter);
            dbus_message_iter_get_basic(&entry_iter, &object_path);
            
            // Check if this object has the Adapter1 interface
            dbus_message_iter_next(&entry_iter);
            dbus_message_iter_recurse(&entry_iter, &value_iter);
            
            while (dbus_message_iter_get_arg_type(&value_iter) == DBUS_TYPE_DICT_ENTRY) {
                DBusMessageIter interface_iter;
                char *interface_name = NULL;
                
                dbus_message_iter_recurse(&value_iter, &interface_iter);
                dbus_message_iter_get_basic(&interface_iter, &interface_name);
                
                if (strcmp(interface_name, ADAPTER_INTERFACE) == 0) {
                    adapter_path = strdup(object_path);
                    dbus_message_unref(reply);
                    return adapter_path;
                }
                
                dbus_message_iter_next(&value_iter);
            }
            
            dbus_message_iter_next(&array_iter);
        }
    }
    
    dbus_message_unref(reply);
    return NULL;
}

/* Call BlueZ StartDiscovery method */
static ErrorCode bluez_start_discovery(DeviceManager* manager) {
    DBusError error;
    DBusMessage *msg, *reply;
    
    if (!manager->adapter_path) {
        if (manager->config.on_error) {
            manager->config.on_error(ERR_BLUEZ, "No Bluetooth adapter found", 
                                   manager->config.user_data);
        }
        return ERR_BLUEZ;
    }
    
    dbus_error_init(&error);
    
    msg = dbus_message_new_method_call(BLUEZ_SERVICE,
                                       manager->adapter_path,
                                       ADAPTER_INTERFACE,
                                       "StartDiscovery");
    
    if (!msg) {
        if (manager->config.on_error) {
            manager->config.on_error(ERR_DBUS, "Failed to create DBus message", 
                                   manager->config.user_data);
        }
        return ERR_DBUS;
    }
    
    reply = dbus_connection_send_with_reply_and_block(manager->conn, msg, 1000, &error);
    dbus_message_unref(msg);
    
    if (!reply) {
        handle_dbus_error(&error, manager);
        return ERR_BLUEZ;
    }
    
    dbus_message_unref(reply);
    return SUCCESS;
}

/* Call BlueZ StopDiscovery method */
static ErrorCode bluez_stop_discovery(DeviceManager* manager) {
    DBusError error;
    DBusMessage *msg, *reply;
    
    if (!manager->adapter_path) {
        return ERR_BLUEZ;
    }
    
    dbus_error_init(&error);
    
    msg = dbus_message_new_method_call(BLUEZ_SERVICE,
                                       manager->adapter_path,
                                       ADAPTER_INTERFACE,
                                       "StopDiscovery");
    
    if (!msg) {
        if (manager->config.on_error) {
            manager->config.on_error(ERR_DBUS, "Failed to create DBus message", 
                                   manager->config.user_data);
        }
        return ERR_DBUS;
    }
    
    reply = dbus_connection_send_with_reply_and_block(manager->conn, msg, 1000, &error);
    dbus_message_unref(msg);
    
    if (!reply) {
        handle_dbus_error(&error, manager);
        return ERR_BLUEZ;
    }
    
    dbus_message_unref(reply);
    return SUCCESS;
}

/* Signal handler for InterfacesAdded */
static void handle_interfaces_added(DeviceManager* manager, DBusMessage* message) {
    DBusMessageIter iter, dict_iter;
    char *object_path;
    
    dbus_message_iter_init(message, &iter);
    dbus_message_iter_get_basic(&iter, &object_path);
    
    dbus_message_iter_next(&iter);
    
    if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
        dbus_message_iter_recurse(&iter, &dict_iter);
        
        while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY) {
            DBusMessageIter entry_iter;
            char *interface = NULL;
            
            dbus_message_iter_recurse(&dict_iter, &entry_iter);
            dbus_message_iter_get_basic(&entry_iter, &interface);
            
            if (strcmp(interface, DEVICE_INTERFACE) == 0) {
                dbus_message_iter_next(&entry_iter);
                
                DBusMessageIter properties_iter;
                dbus_message_iter_recurse(&entry_iter, &properties_iter);
                
                BluetoothDevice* device = parse_device_properties(&properties_iter);
                if (device) {
                    pthread_mutex_lock(&manager->mutex);
                    
                    // Check if device already exists
                    BluetoothDevice* existing = g_hash_table_lookup(manager->devices, device->address);
                    if (existing) {
                        free(device);
                    } else {
                        g_hash_table_insert(manager->devices, strdup(device->address), device);
                        
                        if (manager->config.on_discovered) {
                            manager->config.on_discovered(device, manager->config.user_data);
                        }
                    }
                    
                    pthread_mutex_unlock(&manager->mutex);
                }
                break;
            }
            
            dbus_message_iter_next(&dict_iter);
        }
    }
}

/* Main DBus monitoring thread */
static void* dbus_monitor_thread(void* arg) {
    DeviceManager* manager = (DeviceManager*)arg;
    
    while (manager->running) {
        // Process DBus events
        while (dbus_connection_read_write_dispatch(manager->conn, 100)) {
            DBusMessage* msg = dbus_connection_pop_message(manager->conn);
            
            if (!msg) {
                continue;
            }
            
            // Check for InterfacesAdded signal
            if (dbus_message_is_signal(msg, "org.freedesktop.DBus.ObjectManager", 
                                      "InterfacesAdded")) {
                handle_interfaces_added(manager, msg);
            }
            
            // Check for PropertiesChanged signal on devices
            else if (dbus_message_is_signal(msg, "org.freedesktop.DBus.Properties", 
                                          "PropertiesChanged")) {
                // Handle property changes (RSSI updates, etc.)
                // TODO: Implement property change handling
            }
            
            dbus_message_unref(msg);
        }
        
        // Small sleep to prevent busy waiting
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 10000000L;  // 10ms in nanoseconds
        nanosleep(&ts, NULL);
    }
    
    return NULL;
}

/* Public API Implementation */

DeviceManager* device_manager_create(const DeviceManagerConfig* config) {
    DeviceManager* manager = calloc(1, sizeof(DeviceManager));
    if (!manager) return NULL;
    
    // Copy config
    manager->config = *config;
    
    // Initialize mutex
    if (pthread_mutex_init(&manager->mutex, NULL) != 0) {
        free(manager);
        return NULL;
    }
    
    // Initialize DBus connection
    DBusError error;
    dbus_error_init(&error);
    
    manager->conn = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if (!manager->conn) {
        fprintf(stderr, "Failed to connect to D-Bus: %s\n", error.message);
        dbus_error_free(&error);
        pthread_mutex_destroy(&manager->mutex);
        free(manager);
        return NULL;
    }
    
    // Try to request name, but don't fail if we can't get it
    // We don't actually need to own a name for scanning
    dbus_error_init(&error);
    int ret = dbus_bus_request_name(manager->conn, "com.blueteeth.btmanager", 
                                   DBUS_NAME_FLAG_DO_NOT_QUEUE, &error);
    
    if (dbus_error_is_set(&error)) {
        fprintf(stderr, "Warning: Could not request D-Bus name: %s\n", error.message);
        fprintf(stderr, "Continuing without exclusive service name...\n");
        dbus_error_free(&error);
    }
    
    // Add match for signals
    dbus_bus_add_match(manager->conn,
                      "type='signal',interface='org.freedesktop.DBus.ObjectManager',"
                      "member='InterfacesAdded'",
                      &error);
    
    if (dbus_error_is_set(&error)) {
        fprintf(stderr, "Failed to add match rule: %s\n", error.message);
        dbus_error_free(&error);
        dbus_connection_unref(manager->conn);
        pthread_mutex_destroy(&manager->mutex);
        free(manager);
        return NULL;
    }
    
    dbus_bus_add_match(manager->conn,
                      "type='signal',interface='org.freedesktop.DBus.Properties',"
                      "member='PropertiesChanged',arg0='org.bluez.Device1'",
                      &error);
    
    if (dbus_error_is_set(&error)) {
        fprintf(stderr, "Failed to add match rule: %s\n", error.message);
        dbus_error_free(&error);
        // Don't fail here - continue without this match.......................(for tests)..
    }
    
    // Create hash table for devices
    manager->devices = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
    
    // Get default adapter
    manager->adapter_path = get_default_adapter(manager);
    
    // Start monitoring thread
    manager->running = true;
    if (pthread_create(&manager->thread, NULL, dbus_monitor_thread, manager) != 0) {
        g_hash_table_destroy(manager->devices);
        dbus_connection_unref(manager->conn);
        pthread_mutex_destroy(&manager->mutex);
        free(manager);
        return NULL;
    }
    
    return manager;
}

ErrorCode device_manager_start_discovery(DeviceManager* manager) {
    if (!manager) return ERR_INVALID_ARG;
    
    pthread_mutex_lock(&manager->mutex);
    
    if (manager->scanning) {
        pthread_mutex_unlock(&manager->mutex);
        return SUCCESS;
    }
    
    ErrorCode err = bluez_start_discovery(manager);
    if (err == SUCCESS) {
        manager->scanning = true;
        
        if (manager->config.on_scan_status) {
            manager->config.on_scan_status(true, manager->config.user_data);
        }
    }
    
    pthread_mutex_unlock(&manager->mutex);
    return err;
}

ErrorCode device_manager_stop_discovery(DeviceManager* manager) {
    if (!manager) return ERR_INVALID_ARG;
    
    pthread_mutex_lock(&manager->mutex);
    
    if (!manager->scanning) {
        pthread_mutex_unlock(&manager->mutex);
        return SUCCESS;
    }
    
    ErrorCode err = bluez_stop_discovery(manager);
    if (err == SUCCESS) {
        manager->scanning = false;
        
        if (manager->config.on_scan_status) {
            manager->config.on_scan_status(false, manager->config.user_data);
        }
    }
    
    pthread_mutex_unlock(&manager->mutex);
    return err;
}

GList* device_manager_get_devices(DeviceManager* manager) {
    if (!manager) return NULL;
    
    pthread_mutex_lock(&manager->mutex);
    GHashTableIter iter;
    gpointer key, value;
    GList* list = NULL;
    
    g_hash_table_iter_init(&iter, manager->devices);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        list = g_list_append(list, value);
    }
    
    pthread_mutex_unlock(&manager->mutex);
    return list;
}

BluetoothDevice* device_manager_get_device(DeviceManager* manager, const char* address) {
    if (!manager || !address) return NULL;
    
    pthread_mutex_lock(&manager->mutex);
    BluetoothDevice* device = g_hash_table_lookup(manager->devices, address);
    pthread_mutex_unlock(&manager->mutex);
    
    return device;
}

ErrorCode device_manager_set_alias(DeviceManager* manager, const char* address, const char* alias) {
    if (!manager || !address || !alias) return ERR_INVALID_ARG;
    
    pthread_mutex_lock(&manager->mutex);
    
    BluetoothDevice* device = g_hash_table_lookup(manager->devices, address);
    if (!device) {
        pthread_mutex_unlock(&manager->mutex);
        return ERR_NO_DEVICE;
    }
    
    strncpy(device->alias, alias, sizeof(device->alias) - 1);
    
    // TODO: Save alias to configuration file/database
    
    pthread_mutex_unlock(&manager->mutex);
    return SUCCESS;
}

void device_manager_destroy(DeviceManager* manager) {
    if (!manager) return;
    
    // Stop scanning if active
    if (manager->scanning) {
        bluez_stop_discovery(manager);
    }
    
    // Stop monitoring thread
    manager->running = false;
    pthread_join(manager->thread, NULL);
    
    // Cleanup
    g_hash_table_destroy(manager->devices);
    dbus_connection_unref(manager->conn);
    pthread_mutex_destroy(&manager->mutex);
    free(manager->adapter_path);
    free(manager);
}
