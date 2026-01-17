#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "bluetooth/device_manager.h"
#include "bluetooth/connection_manager.h"

static DeviceManager* dev_manager = NULL;
static ConnectionManager* conn_manager = NULL;
static volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    (void)sig;
    running = 0;
    printf("\nShutting down...\n");
}

void on_device_discovered(BluetoothDevice* device, void* user_data) {
    (void)user_data;
    printf("Discovered: %s (%s) RSSI: %d\n", 
           device->alias, device->address, device->rssi);
}

void on_scan_status(bool scanning, void* user_data) {
    (void)user_data;
    printf("Scan %s\n", scanning ? "started" : "stopped");
}

void on_error(ErrorCode error __attribute__((unused)), const char* message, void* user_data) {
    (void)user_data;
    fprintf(stderr, "Error: %s\n", message);
}

void on_connection_state(const char* device_address, ConnectionState state, void* user_data) {
    (void)user_data;
    const char* state_str;
    switch (state) {
        case STATE_DISCONNECTED: state_str = "DISCONNECTED"; break;
        case STATE_CONNECTING: state_str = "CONNECTING"; break;
        case STATE_CONNECTED: state_str = "CONNECTED"; break;
        case STATE_DISCONNECTING: state_str = "DISCONNECTING"; break;
        case STATE_FAILED: state_str = "FAILED"; break;
        default: state_str = "UNKNOWN"; break;
    }
    printf("Device %s: %s\n", device_address, state_str);
}

void on_pairing(const char* device_address, bool success, const char* error_message, void* user_data) {
    (void)user_data;
    if (success) {
        printf("Pairing with %s successful\n", device_address);
    } else {
        fprintf(stderr, "Pairing with %s failed: %s\n", device_address, error_message);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <device_address>\n", argv[0]);
        fprintf(stderr, "Example: %s AA:BB:CC:DD:EE:FF\n", argv[0]);
        return 1;
    }
    
    const char* target_address = argv[1];
    
    signal(SIGINT, signal_handler);
    
    printf("=== Bluetooth Connection Manager Test ===\n");
    printf("Target device: %s\n\n", target_address);
    
    // Create device manager (for scanning)
    DeviceManagerConfig dev_config = {
        .scan_duration = 0,
        .filter_duplicates = true,
        .on_discovered = on_device_discovered,
        .on_scan_status = on_scan_status,
        .on_error = on_error,
        .user_data = NULL
    };
    
    dev_manager = device_manager_create(&dev_config);
    if (!dev_manager) {
        fprintf(stderr, "Failed to create device manager\n");
        return 1;
    }
    
    // Create connection manager
    ConnectionManagerConfig conn_config = {
        .connection_timeout = 10,  // 10 seconds
        .auto_reconnect = false,
        .auto_trust = true,
        .user_data = NULL
    };
    
    conn_manager = connection_manager_create(&conn_config);
    if (!conn_manager) {
        fprintf(stderr, "Failed to create connection manager\n");
        device_manager_destroy(dev_manager);
        return 1;
    }
    
    // Set callbacks
    connection_manager_set_state_callback(conn_manager, on_connection_state);
    connection_manager_set_pairing_callback(conn_manager, on_pairing);
    
    printf("1. Starting scan to find device...\n");
    device_manager_start_discovery(dev_manager);
    sleep(5);  // Scan for 5 seconds
    
    printf("\n2. Stopping scan...\n");
    device_manager_stop_discovery(dev_manager);
    
    printf("\n3. Checking if device is in range...\n");
    BluetoothDevice* device = device_manager_get_device(dev_manager, target_address);
    if (!device) {
        fprintf(stderr, "Device %s not found. Make sure:\n", target_address);
        fprintf(stderr, "1. Device is in range and discoverable\n");
        fprintf(stderr, "2. Device Bluetooth is turned on\n");
        device_manager_destroy(dev_manager);
        connection_manager_destroy(conn_manager);
        return 1;
    }
    
    printf("Device found: %s (%s)\n", device->alias, device->address);
    printf("Paired: %s, Trusted: %s\n", 
           device->paired ? "Yes" : "No",
           device->trusted ? "Yes" : "No");
    
    // Ask user what to do
    printf("\nChoose action:\n");
    printf("1. Connect\n");
    printf("2. Pair then Connect\n");
    printf("3. Disconnect (if connected)\n");
    printf("4. Trust device\n");
    printf("5. Exit\n");
    
    int choice;
    printf("\nEnter choice (1-5): ");
    if (scanf("%d", &choice) != 1) {
        fprintf(stderr, "Invalid input\n");
        device_manager_destroy(dev_manager);
        connection_manager_destroy(conn_manager);
        return 1;
    }
    
    switch (choice) {
        case 1:  // Connect
            printf("\nConnecting to %s...\n", target_address);
            if (connection_manager_connect(conn_manager, target_address) == SUCCESS) {
                printf("Connected successfully!\n");
                
                // Keep connection for 10 seconds
                printf("Keeping connection for 10 seconds...\n");
                sleep(10);
                
                printf("Disconnecting...\n");
                connection_manager_disconnect(conn_manager, target_address);
            }
            break;
            
        case 2:  // Pair then Connect
            printf("\nPairing with %s...\n", target_address);
            if (connection_manager_pair(conn_manager, target_address) == SUCCESS) {
                printf("Paired successfully!\n");
                
                printf("Connecting...\n");
                if (connection_manager_connect(conn_manager, target_address) == SUCCESS) {
                    printf("Connected successfully!\n");
                    sleep(5);
                    connection_manager_disconnect(conn_manager, target_address);
                }
            }
            break;
            
        case 3:  // Disconnect
            printf("\nDisconnecting from %s...\n", target_address);
            connection_manager_disconnect(conn_manager, target_address);
            break;
            
        case 4:  // Trust
            printf("\nTrusting device %s...\n", target_address);
            connection_manager_trust(conn_manager, target_address);
            break;
            
        case 5:  // Exit
            printf("\nExiting...\n");
            break;
            
        default:
            printf("\nInvalid choice\n");
    }
    
    // Cleanup
    printf("\nCleaning up...\n");
    device_manager_destroy(dev_manager);
    connection_manager_destroy(conn_manager);
    
    printf("Test complete!\n");
    return 0;
}
