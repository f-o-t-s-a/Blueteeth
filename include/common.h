#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>

/* Error codes */
typedef enum {
    SUCCESS = 0,
    ERR_INVALID_ARG = -1,
    ERR_MEMORY = -2,
    ERR_DBUS = -3,
    ERR_BLUEZ = -4,
    ERR_THREAD = -5,
    ERR_IPC = -6,
    ERR_NO_DEVICE = -7,
    ERR_CONNECTION = -8
} ErrorCode;

/* Device types */
typedef enum {
    DEVICE_UNKNOWN = 0,
    DEVICE_AUDIO_SINK,
    DEVICE_AUDIO_SOURCE,
    DEVICE_INPUT,
    DEVICE_KEYBOARD,
    DEVICE_MOUSE,
    DEVICE_PHONE,
    DEVICE_COMPUTER
} DeviceType;

/* Connection state */
typedef enum {
    STATE_DISCONNECTED = 0,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_DISCONNECTING,
    STATE_FAILED
} ConnectionState;

/* Bluetooth device structure */
typedef struct {
    char address[18];          // MAC address (XX:XX:XX:XX:XX:XX)
    char name[256];           // Device name
    char alias[256];          // User-defined alias
    DeviceType type;          // Device type
    ConnectionState state;    // Current connection state
    int8_t rssi;             // Signal strength
    bool paired;             // Is device paired?
    bool trusted;            // Is device trusted?
    bool blocked;            // Is device blocked?
    uint32_t class;          // Device class
    void* user_data;         // User-defined data
} BluetoothDevice;

/* Callback function types */
typedef void (*DeviceDiscoveredCallback)(BluetoothDevice* device, void* user_data);
typedef void (*ScanStatusCallback)(bool scanning, void* user_data);
typedef void (*ErrorCallback)(ErrorCode error, const char* message, void* user_data);

#endif /* COMMON_H */
