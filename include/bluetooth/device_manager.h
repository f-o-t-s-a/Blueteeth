#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "common.h"
#include <glib.h>

typedef struct DeviceManager DeviceManager;

/* Device manager configuration */
typedef struct {
    int scan_duration;                    // Scan duration in seconds (0 = continuous)
    bool filter_duplicates;              // Filter duplicate device discoveries
    DeviceDiscoveredCallback on_discovered;
    ScanStatusCallback on_scan_status;
    ErrorCallback on_error;
    void* user_data;                     // User data for callbacks
} DeviceManagerConfig;

/* Initialize device manager */
DeviceManager* device_manager_create(const DeviceManagerConfig* config);

/* Start device discovery */
ErrorCode device_manager_start_discovery(DeviceManager* manager);

/* Stop device discovery */
ErrorCode device_manager_stop_discovery(DeviceManager* manager);

/* Get discovered devices */
GList* device_manager_get_devices(DeviceManager* manager);

/* Get device by address */
BluetoothDevice* device_manager_get_device(DeviceManager* manager, const char* address);

/* Set device alias */
ErrorCode device_manager_set_alias(DeviceManager* manager, const char* address, const char* alias);

/* Remove device */
ErrorCode device_manager_remove_device(DeviceManager* manager, const char* address);

/* Cleanup */
void device_manager_destroy(DeviceManager* manager);

#endif /* DEVICE_MANAGER_H */
