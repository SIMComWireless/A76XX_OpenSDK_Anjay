# Anjay LWM2M Port for SIMCOM A7672E

This directory contains the platform adaptation layer for running the Anjay LWM2M client on the SIMCOM A7672E cellular module.

## Directory Structure

```
port/
├── inc/
│   ├── anjay_simcom_config.h   # Configuration macros (bootstrap, buffers, etc.)
│   └── simcom_platform.h       # Platform abstraction API
├── src/
│   ├── net_impl.c              # Network layer (UDP/TCP socket wrapper)
│   ├── platform_impl.c         # Logging and utilities
│   └── anjay_main.c            # Main client entry point
├── CMakeLists.txt              # CMake build file
├── anjay_port.mak              # Makefile snippet for SDK integration
└── README.md                   # This file
```

## Quick Start

### 1. Configure Bootstrap Server

Edit `inc/anjay_simcom_config.h` and fill in your Bootstrap server details:

```c
#define BOOTSTRAP_SERVER_URI        "coaps://your-server:5684"
#define BOOTSTRAP_SECURITY_MODE     ANJAY_SECURITY_PSK
#define BOOTSTRAP_PSK_IDENTITY      "your_identity"
#define BOOTSTRAP_PSK_KEY           "your_psk_key"
#define ANJAY_ENDPOINT_NAME         "your_device_name"
```

### 2. Initialize avs_commons Submodule

```bash
cd SIMCOM_SDK_SET/public_libs/Anjay
git submodule update --init --recursive
```

### 3. Build Integration

#### Option A: Using Makefile

Include `anjay_port.mak` in your application's build process:

```makefile
# In your application Makefile
include $(ROOT_DIR)/public_libs/Anjay/port/anjay_port.mak
```

#### Option B: Using CMake

Add the port directory to your CMakeLists.txt:

```cmake
# Set paths
set(ANJAY_DIR ${CMAKE_SOURCE_DIR}/public_libs/Anjay)
set(SDK_INC_DIR ${CMAKE_SOURCE_DIR}/sc_lib/A7672E_FASE_1603_V201_DS_OPENSDK/inc)

# Add port subdirectory
add_subdirectory(${ANJAY_DIR}/port anjay_port)

# Link to your application
target_link_libraries(your_app PRIVATE anjay_port)
```

### 4. Use in Application

In your `sc_application.c` or dedicated task:

```c
#include "anjay_main.h"

// Option 1: Call directly (blocks until stopped)
void userSpace_Main(void *arg) {
    // ... other init ...
    anjay_client_start();
}

// Option 2: Run in separate task (non-blocking)
void userSpace_Main(void *arg) {
    // ... other init ...
    anjay_client_start_task();
}
```

## DTLS Support

DTLS is handled by Anjay's native mbedtls integration (`AVS_COMMONS_WITH_MBEDTLS`). The mbedtls source is located at `Third_Party/mbedtls/` and is compiled as part of the build.

### Supported Security Modes

- **NoSec** (`ANJAY_SECURITY_NOSEC`) — No encryption, for testing only
- **PSK** (`ANJAY_SECURITY_PSK`) — Pre-Shared Key authentication via DTLS 1.2

### NoSec Mode (Testing Only)

For initial testing without encryption:

```c
#define BOOTSTRAP_SECURITY_MODE     ANJAY_SECURITY_NOSEC
```

And use `coap://` instead of `coaps://` in the server URI.

## API Reference

### anjay_client_start()

```c
int anjay_client_start(void);
```

Initializes and starts the LWM2M client. This function blocks and runs the event loop until `anjay_client_stop()` is called.

**Returns:** 0 on success, -1 on failure

### anjay_client_start_task()

```c
int anjay_client_start_task(void);
```

Creates a dedicated task for the LWM2M client. The client runs in the background.

**Returns:** 0 on success, -1 on failure

### anjay_client_stop()

```c
void anjay_client_stop(void);
```

Signals the client to stop. The event loop will exit on the next iteration.

### anjay_client_is_running()

```c
bool anjay_client_is_running(void);
```

**Returns:** true if the client is running, false otherwise

## Troubleshooting

### Build Errors

1. **"avs_commons not found"**: Initialize the git submodule
2. **"Multiple definitions"**: Ensure only one network implementation is compiled
3. **Missing headers**: Check include paths in CMakeLists.txt

### Runtime Issues

1. **DNS resolution fails**: Check network registration and APN settings
2. **Connection timeout**: Verify server URI and port
3. **PSK mismatch**: Ensure identity and key match server configuration

### Memory Issues

Adjust buffer sizes in `anjay_simcom_config.h`:

```c
#define ANJAY_IN_BUFFER_SIZE        2000  // Reduce if low on RAM
#define ANJAY_OUT_BUFFER_SIZE       2000
#define ANJAY_MSG_CACHE_SIZE        1000
```

## References

- [Anjay Documentation](https://avsystem.github.io/Anjay-doc/)
- [LWM2M Specification](https://www.openmobilealliance.org/release/LightweightM2M/)
- [SIMCOM OpenSDK Documentation](Doc/software/)
