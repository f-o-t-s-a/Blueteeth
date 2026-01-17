#include "bluetooth/connection_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <dbus/dbus.h>
#include <glib.h>

#define BLUEZ_SERVICE "org.bluez"
#define DEVICE_INTERFACE "org.bluez.Device1"
#define ADAPTER_INTERFACE "org.bluez.Adapter1"

/* Internal connection manager structure */
struct ConnectionManager {
    ConnectionManagerConfig config;
    DBusConnection* conn;
    pthread_mutex_t mutex;
    GHashTable* connections;  // device_address -> ConnectionState
    ConnectionStateCallback state_callback;
    PairingCallback pairing_callback;
    void* pairing_user_data;
};

/* Helper to convert address to device path format */
static char* address_to_path(const char* address) {
    char path[256];
    snprintf(path, sizeof(path), "/org/bluez/hci0/dev_%s", address);
    
    // Replace colons with underscores
    for (char* p = path; *p; p++) {
        if (*p == ':') *p = '_';
    }
    
    return strdup(path);
}

/* Helper to get device path from BlueZ */
static char* get_device_path(ConnectionManager* manager, const char* address) {
    DBusError error;
    DBusMessage *msg, *reply;
    char *device_path = NULL;
    
    dbus_error_init(&error);
    
    msg = dbus_message_new_method_call(BLUEZ_SERVICE,
                                       "/",
                                       "org.freedesktop.DBus.ObjectManager",
                                       "GetManagedObjects");
    
    reply = dbus_connection_send_with_reply_and_block(manager->conn, msg, 3000, &error);
    dbus_message_unref(msg);
    
    if (!reply) {
        printf("DEBUG: GetManagedObjects failed: %s\n", error.message);
        dbus_error_free(&error);
        // Fall back to constructed path
        return address_to_path(address);
    }
    
    DBusMessageIter iter, array_iter;
    dbus_message_iter_init(reply, &iter);
    
    if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
        dbus_message_iter_recurse(&iter, &array_iter);
        
        while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_DICT_ENTRY) {
            DBusMessageIter entry_iter, iface_iter;
            char *object_path = NULL;
            
            dbus_message_iter_recurse(&array_iter, &entry_iter);
            dbus_message_iter_get_basic(&entry_iter, &object_path);
            
            // Check if this path matches our device
            dbus_message_iter_next(&entry_iter);
            dbus_message_iter_recurse(&entry_iter, &iface_iter);
            
            while (dbus_message_iter_get_arg_type(&iface_iter) == DBUS_TYPE_DICT_ENTRY) {
                DBusMessageIter iface_entry;
                char *interface = NULL;
                
                dbus_message_iter_recurse(&iface_iter, &iface_entry);
                dbus_message_iter_get_basic(&iface_entry, &interface);
                
                if (strcmp(interface, DEVICE_INTERFACE) == 0) {
                    // Found a device, check if it matches our address
                    char* addr_in_path = strrchr(object_path, '/');
                    if (addr_in_path) {
                        addr_in_path++; // Skip the '/'
                        
                        // Convert dev_XX_XX_XX to XX:XX:XX format
                        char converted[18];
                        int j = 0;
                        for (int i = 4; addr_in_path[i] && j < 17; i++) {
                            if (addr_in_path[i] == '_') {
                                converted[j++] = ':';
                            } else {
                                converted[j++] = addr_in_path[i];
                            }
                        }
                        converted[j] = '\0';
                        
                        if (strcasecmp(converted, address) == 0) {
                            device_path = strdup(object_path);
                            dbus_message_unref(reply);
                            return device_path;
                        }
                    }
                }
                
                dbus_message_iter_next(&iface_iter);
            }
            
            dbus_message_iter_next(&array_iter);
        }
    }
    
    dbus_message_unref(reply);
    
    // If not found in managed objects, construct path
    return address_to_path(address);
}

/* Update connection state */
static void update_connection_state(ConnectionManager* manager,
                                   const char* device_address,
                                   ConnectionState state) {
    pthread_mutex_lock(&manager->mutex);
    
    ConnectionState* new_state = malloc(sizeof(ConnectionState));
    *new_state = state;
    g_hash_table_replace(manager->connections, strdup(device_address), new_state);
    
    pthread_mutex_unlock(&manager->mutex);
    
    if (manager->state_callback) {
        manager->state_callback(device_address, state, manager->config.user_data);
    }
}

/* Public API Implementation */

ConnectionManager* connection_manager_create(const ConnectionManagerConfig* config) {
    ConnectionManager* manager = calloc(1, sizeof(ConnectionManager));
    if (!manager) return NULL;
    
    manager->config = *config;
    
    if (pthread_mutex_init(&manager->mutex, NULL) != 0) {
        free(manager);
        return NULL;
    }
    
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
    
    manager->connections = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
    return manager;
}

ErrorCode connection_manager_connect(ConnectionManager* manager, 
                                     const char* device_address) {
    if (!manager || !device_address) return ERR_INVALID_ARG;
    
    printf("Attempting to connect to: %s\n", device_address);
    
    char* device_path = get_device_path(manager, device_address);
    printf("DEBUG: Using device path: %s\n", device_path);
    
    update_connection_state(manager, device_address, STATE_CONNECTING);
    
    DBusError error;
    DBusMessage *msg, *reply;
    
    dbus_error_init(&error);
    
    msg = dbus_message_new_method_call(BLUEZ_SERVICE,
                                       device_path,
                                       DEVICE_INTERFACE,
                                       "Connect");
    
    if (!msg) {
        free(device_path);
        update_connection_state(manager, device_address, STATE_FAILED);
        return ERR_DBUS;
    }
    
    reply = dbus_connection_send_with_reply_and_block(manager->conn, msg, 
                                                      manager->config.connection_timeout * 1000, 
                                                      &error);
    dbus_message_unref(msg);
    
    if (!reply) {
        fprintf(stderr, "Connect failed: %s\n", error.message);
        dbus_error_free(&error);
        free(device_path);
        update_connection_state(manager, device_address, STATE_FAILED);
        return ERR_CONNECTION;
    }
    
    dbus_message_unref(reply);
    free(device_path);
    
    printf("Connect successful!\n");
    update_connection_state(manager, device_address, STATE_CONNECTED);
    return SUCCESS;
}

ErrorCode connection_manager_disconnect(ConnectionManager* manager, 
                                        const char* device_address) {
    if (!manager || !device_address) return ERR_INVALID_ARG;
    
    printf("Disconnecting from: %s\n", device_address);
    update_connection_state(manager, device_address, STATE_DISCONNECTING);
    
    char* device_path = get_device_path(manager, device_address);
    
    DBusError error;
    DBusMessage *msg, *reply;
    
    dbus_error_init(&error);
    
    msg = dbus_message_new_method_call(BLUEZ_SERVICE,
                                       device_path,
                                       DEVICE_INTERFACE,
                                       "Disconnect");
    
    if (!msg) {
        free(device_path);
        update_connection_state(manager, device_address, STATE_FAILED);
        return ERR_DBUS;
    }
    
    reply = dbus_connection_send_with_reply_and_block(manager->conn, msg, 5000, &error);
    dbus_message_unref(msg);
    
    if (!reply) {
        fprintf(stderr, "Disconnect failed: %s\n", error.message);
        dbus_error_free(&error);
        free(device_path);
        update_connection_state(manager, device_address, STATE_FAILED);
        return ERR_CONNECTION;
    }
    
    dbus_message_unref(reply);
    free(device_path);
    
    printf("Disconnect successful!\n");
    update_connection_state(manager, device_address, STATE_DISCONNECTED);
    return SUCCESS;
}

ErrorCode connection_manager_pair(ConnectionManager* manager, 
                                  const char* device_address) {
    if (!manager || !device_address) return ERR_INVALID_ARG;
    
    printf("Attempting to pair with: %s\n", device_address);
    
    char* device_path = get_device_path(manager, device_address);
    
    DBusError error;
    DBusMessage *msg, *reply;
    
    dbus_error_init(&error);
    
    msg = dbus_message_new_method_call(BLUEZ_SERVICE,
                                       device_path,
                                       DEVICE_INTERFACE,
                                       "Pair");
    
    if (!msg) {
        free(device_path);
        if (manager->pairing_callback) {
            manager->pairing_callback(device_address, false, "Failed to create D-Bus message", 
                                     manager->pairing_user_data);
        }
        return ERR_DBUS;
    }
    
    reply = dbus_connection_send_with_reply_and_block(manager->conn, msg, 30000, &error);
    dbus_message_unref(msg);
    
    if (!reply) {
        fprintf(stderr, "Pair failed: %s\n", error.message);
        if (manager->pairing_callback) {
            manager->pairing_callback(device_address, false, error.message, 
                                     manager->pairing_user_data);
        }
        dbus_error_free(&error);
        free(device_path);
        return ERR_PAIRING;
    }
    
    dbus_message_unref(reply);
    free(device_path);
    
    printf("Pairing successful!\n");
    
    if (manager->pairing_callback) {
        manager->pairing_callback(device_address, true, NULL, manager->pairing_user_data);
    }
    
    // Auto-trust if configured
    if (manager->config.auto_trust) {
        connection_manager_trust(manager, device_address);
    }
    
    return SUCCESS;
}

ErrorCode connection_manager_trust(ConnectionManager* manager, 
                                   const char* device_address) {
    if (!manager || !device_address) return ERR_INVALID_ARG;
    
    printf("Setting device as trusted: %s\n", device_address);
    
    char* device_path = get_device_path(manager, device_address);
    
    DBusError error;
    DBusMessage *msg, *reply;
    
    dbus_error_init(&error);
    
    // Set the "Trusted" property to true
    msg = dbus_message_new_method_call(BLUEZ_SERVICE,
                                       device_path,
                                       "org.freedesktop.DBus.Properties",
                                       "Set");
    
    if (!msg) {
        free(device_path);
        return ERR_DBUS;
    }
    
    const char* interface = DEVICE_INTERFACE;
    const char* property = "Trusted";
    dbus_bool_t trusted = TRUE;
    
    DBusMessageIter iter, value_iter;
    dbus_message_iter_init_append(msg, &iter);
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &interface);
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &property);
    
    dbus_message_iter_open_container(&iter, DBUS_TYPE_VARIANT, "b", &value_iter);
    dbus_message_iter_append_basic(&value_iter, DBUS_TYPE_BOOLEAN, &trusted);
    dbus_message_iter_close_container(&iter, &value_iter);
    
    reply = dbus_connection_send_with_reply_and_block(manager->conn, msg, 5000, &error);
    dbus_message_unref(msg);
    
    if (!reply) {
        fprintf(stderr, "Trust failed: %s\n", error.message);
        dbus_error_free(&error);
        free(device_path);
        return ERR_DBUS;
    }
    
    dbus_message_unref(reply);
    free(device_path);
    
    printf("Device trusted successfully!\n");
    return SUCCESS;
}

void connection_manager_set_state_callback(ConnectionManager* manager, 
                                           ConnectionStateCallback callback) {
    if (manager) {
        manager->state_callback = callback;
    }
}

void connection_manager_set_pairing_callback(ConnectionManager* manager, 
                                             PairingCallback callback) {
    if (manager) {
        manager->pairing_callback = callback;
    }
}

ConnectionState connection_manager_get_state(ConnectionManager* manager, 
                                             const char* device_address) {
    if (!manager || !device_address) return STATE_DISCONNECTED;
    
    pthread_mutex_lock(&manager->mutex);
    ConnectionState* state = g_hash_table_lookup(manager->connections, device_address);
    ConnectionState result = state ? *state : STATE_DISCONNECTED;
    pthread_mutex_unlock(&manager->mutex);
    
    return result;
}

void connection_manager_destroy(ConnectionManager* manager) {
    if (!manager) return;
    
    if (manager->connections) {
        g_hash_table_destroy(manager->connections);
    }
    
    if (manager->conn) {
        dbus_connection_unref(manager->conn);
    }
    
    pthread_mutex_destroy(&manager->mutex);
    free(manager);
}
