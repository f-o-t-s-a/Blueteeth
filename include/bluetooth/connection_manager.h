#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include "common.h"
#include <glib.h>

typedef struct ConnectionManager ConnectionManager;

/* Connection manager configuration */
typedef struct {
    int connection_timeout;           // Timeout in seconds for connection attempts
    bool auto_reconnect;              // Attempt to reconnect if connection drops
    bool auto_trust;                  // Automatically trust connected devices
    void* user_data;                  // User data for callbacks
} ConnectionManagerConfig;

/* Connection state change callback */
typedef void (*ConnectionStateCallback)(const char* device_address, 
                                        ConnectionState state, 
                                        void* user_data);

/* Pairing completion callback */
typedef void (*PairingCallback)(const char* device_address, 
                                bool success, 
                                const char* error_message, 
                                void* user_data);

/* Initialize connection manager */
ConnectionManager* connection_manager_create(const ConnectionManagerConfig* config);

/* Connect to a device */
ErrorCode connection_manager_connect(ConnectionManager* manager, 
                                     const char* device_address);

/* Disconnect from a device */
ErrorCode connection_manager_disconnect(ConnectionManager* manager, 
                                        const char* device_address);

/* Pair with a device */
ErrorCode connection_manager_pair(ConnectionManager* manager, 
                                  const char* device_address);

/* Remove pairing with a device */
ErrorCode connection_manager_unpair(ConnectionManager* manager, 
                                    const char* device_address);

/* Trust a device */
ErrorCode connection_manager_trust(ConnectionManager* manager, 
                                   const char* device_address);

/* Block a device */
ErrorCode connection_manager_block(ConnectionManager* manager, 
                                   const char* device_address);

/* Get connection state for a device */
ConnectionState connection_manager_get_state(ConnectionManager* manager, 
                                             const char* device_address);

/* Set callbacks */
void connection_manager_set_state_callback(ConnectionManager* manager, 
                                           ConnectionStateCallback callback);
void connection_manager_set_pairing_callback(ConnectionManager* manager, 
                                             PairingCallback callback);

/* Cleanup */
void connection_manager_destroy(ConnectionManager* manager);

#endif /* CONNECTION_MANAGER_H */
