#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "bluetooth/device_manager.h"

static DeviceManager* manager = NULL;
static volatile int running = 1;

void signal_handler(int sig) {
    (void)sig;  // Mark parameter as unused
    running = 0;
    printf("\nShutting down...\n");
}

void on_device_discovered(BluetoothDevice* device, void* user_data) {
    (void)user_data;  // Mark parameter as unused
    printf("Device: %s (%s) - RSSI: %d\n", device->alias, device->address, device->rssi);
}

void on_scan_status(bool scanning, void* user_data) {
    (void)user_data;  // Mark parameter as unused
    printf("Scan %s\n", scanning ? "started" : "stopped");
}

void on_error(ErrorCode error __attribute__((unused)), const char* message, void* user_data) {
    (void)user_data;  // Mark parameter as unused
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
        fprintf(stderr, "Failed to create device manager..\n");
        return 1;
    }
    
    printf("Starting Bluetooth scan...\n");
    
    if (device_manager_start_discovery(manager) != SUCCESS) {
        fprintf(stderr, "Failed to start discovery\n");
        device_manager_destroy(manager);
        return 1;
    }
    
    printf("Scanning for 10 seconds (Press Ctrl+C to stop early)...\n");
    
    // Sleep with interrupt checking
    for (int i = 0; i < 10 && running; i++) {
        sleep(1);
    }
    
    if (running) {
        device_manager_stop_discovery(manager);
        printf("Scan stopped..\n");
    } else {
        printf("Scan interrupted by user..\n");
    }
    
    // List all discovered devices
    printf("\nDiscovered devices:\n");
    GList* devices = device_manager_get_devices(manager);
    
    if (devices == NULL) {
        printf("No devices found..\n");
    } else {
        GList* iter = devices;
        int count = 0;
        
        while (iter) {
            BluetoothDevice* device = (BluetoothDevice*)iter->data;
            printf("- %s (%s) Type: %d RSSI: %d\n", 
                   device->alias, device->address, device->type, device->rssi);
            iter = iter->next;
            count++;
        }
        
        printf("\nTotal devices found: %d\n", count);
        g_list_free(devices);
    }
    
    device_manager_destroy(manager);
    
    printf("Done!!..\n");
    return 0;
}
