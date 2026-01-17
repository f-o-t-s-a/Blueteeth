#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "device_manager.h"

static DeviceManager* manager = NULL;
static volatile int running = 1;

void signal_handler(int sig) {
    running = 0;
    printf("\nShutting down...\n");
}

void on_device_discovered(BluetoothDevice* device, void* user_data) {
    printf("Device: %s (%s) - RSSI: %d\n", device->alias, device->address, device->rssi);
}

void on_scan_status(bool scanning, void* user_data) {
    printf("Scan %s\n", scanning ? "started" : "stopped");
}

void on_error(ErrorCode error, const char* message, void* user_data) {
    fprintf(stderr, "Error: %s\n", message);
}

int main() {
    signal(SIGINT, signal_handler);
    
    DeviceManagerConfig config = {
        .scan_duration = 0,
        .filter_duplicates = true,
        .on_discovered = on_device_discovered,
        .on_scan_status = on_scan_status,
        .on_error = on_error,
        .user_data = NULL
    };
    
    manager = device_manager_create(&config);
    if (!manager) {
        fprintf(stderr, "Failed to create device manager\n");
        return 1;
    }
    
    printf("Starting Bluetooth scan...\n");
    
    if (device_manager_start_discovery(manager) != SUCCESS) {
        fprintf(stderr, "Failed to start discovery\n");
        device_manager_destroy(manager);
        return 1;
    }
    
    printf("Scanning for 10 seconds...\n");
    sleep(10);
    
    device_manager_stop_discovery(manager);
    printf("Scan stopped\n");
    
    // List all discovered devices
    printf("\nDiscovered devices:\n");
    GList* devices = device_manager_get_devices(manager);
    GList* iter = devices;
    
    while (iter) {
        BluetoothDevice* device = (BluetoothDevice*)iter->data;
        printf("- %s (%s) Type: %d RSSI: %d\n", 
               device->alias, device->address, device->type, device->rssi);
        iter = iter->next;
    }
    
    g_list_free(devices);
    device_manager_destroy(manager);
    
    printf("Done!\n");
    return 0;
}
