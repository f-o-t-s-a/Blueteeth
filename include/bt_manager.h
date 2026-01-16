#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

int main(int argc, char **argv)
{
    inquiry_info *devices = NULL;
    int max_responses = 255;
    int num_responses;
    int dev_id, sock, len, flags;
    int i;
    char addr[19] = { 0 };
    char name[248] = { 0 };

    // Get the default Bluetooth adapter
    dev_id = hci_get_route(NULL);
    if (dev_id < 0) {
        perror("Error: No Bluetooth adapter found");
        exit(1);
    }

    // Open a socket to the Bluetooth device
    sock = hci_open_dev(dev_id);
    if (sock < 0) {
        perror("Error: Cannot open Bluetooth device");
        exit(1);
    }

    printf("Scanning for Bluetooth devices...\n");
    printf("This may take up to 10 seconds...\n\n");

    // Allocate memory for device info
    len = sizeof(inquiry_info) * max_responses;
    devices = (inquiry_info*)malloc(len);
    if (!devices) {
        perror("Error: Cannot allocate memory");
        close(sock);
        exit(1);
    }

    // Perform inquiry (scan for devices)
    // Parameters: device_id, inquiry_length (1.28s * 8 = ~10s), max_responses, 
    //             lap (NULL = general inquiry), &devices, flags
    flags = IREQ_CACHE_FLUSH;
    num_responses = hci_inquiry(dev_id, 8, max_responses, NULL, &devices, flags);
    
    if (num_responses < 0) {
        perror("Error: Inquiry failed");
        free(devices);
        close(sock);
        exit(1);
    }

    printf("Found %d device(s)\n\n", num_responses);

    // Display information about each device found
    for (i = 0; i < num_responses; i++) {
        ba2str(&(devices[i].bdaddr), addr);
        memset(name, 0, sizeof(name));
        
        // Try to get the device name
        if (hci_read_remote_name(sock, &(devices[i].bdaddr), sizeof(name), 
                                  name, 0) < 0) {
            strcpy(name, "[Unknown]");
        }

        printf("Device %d:\n", i + 1);
        printf("  Address: %s\n", addr);
        printf("  Name: %s\n", name);
        printf("  Class: 0x%02x%02x%02x\n", 
               devices[i].dev_class[2],
               devices[i].dev_class[1], 
               devices[i].dev_class[0]);
        printf("\n");
    }

    // Cleanup
    free(devices);
    close(sock);

    return 0;
}
