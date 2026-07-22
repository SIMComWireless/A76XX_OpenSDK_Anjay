# Anjay LwM2M Porting to SIMCOM A7672E SDK - Summary

## Completed Changes

### 1. Created Minimal mbedtls Configuration Header

**File**: `APP/sc_app/code/Third_Party/mbedtls/include/mbedtls/anjay_mbedtls_config.h`

Created an mbedtls configuration optimized for embedded platforms, enabling only the minimum feature set required for DTLS + PSK:

- AES, CCM, GCM cipher algorithms
- SHA-256 hash algorithm
- CTR_DRBG random number generator
- DTLS 1.2 protocol support
- PSK key exchange
- Disabled unnecessary features (X.509, RSA, ECC, etc.)

### 2. Modified port/CMakeLists.txt

**File**: `APP/sc_app/code/Anjay/port/CMakeLists.txt`

Key changes:

- Removed custom TLS implementation (`tls_impl.c`)
- Added mbedtls source file compilation
- Set `AVS_COMMONS_WITH_MBEDTLS` compile definition
- Configured mbedtls header include paths
- Specified `MBEDTLS_CONFIG_FILE="mbedtls/anjay_mbedtls_config.h"`

### 3. Cleared tls_impl.c

**File**: `APP/sc_app/code/Anjay/port/src/tls_impl.c`

Emptied the file to contain only comment documentation. With Anjay's native mbedtls integration, all DTLS functionality is handled by Anjay's `avs_commons/net/src/ssl/mbedtls` module.

### 4. Modified APP/sc_app/CMakeLists.txt

**File**: `APP/sc_app/CMakeLists.txt`

Key changes:

- Added mbedtls library compilation (static library `mbedtls_local`)
- Configured mbedtls headers and compile definitions
- Set variables required by `FindMbedTLS` (`MBEDTLS_FOUND`, `MBEDTLS_INCLUDE_DIR`, etc.)
- Configured Anjay build options:
  - `DTLS_BACKEND=mbedtls`
  - `WITH_MBEDTLS=ON`
  - `WITH_PSK=ON`
  - `WITH_PKI=OFF`
  - `WITH_POSIX_AVS_SOCKET=OFF`
- Linked mbedtls library to the main application

### 5. Modified mbedtls config.h

**File**: `APP/sc_app/code/Third_Party/mbedtls/include/mbedtls/config.h`

Added support for custom configuration file:

- If `MBEDTLS_CONFIG_FILE` is defined, include that file
- Otherwise, use the default configuration

### 6. Modified anjay_main.c

**File**: `APP/sc_app/code/Anjay/port/src/anjay_main.c`

Removed manual SSL initialization calls:

- Removed `_avs_net_initialize_global_ssl_state()` call
- Removed `_avs_net_cleanup_global_ssl_state()` call
- Removed `_avs_net_initialize_global_compat_state()` call
- Removed `_avs_net_cleanup_global_compat_state()` call

These functions are now automatically called by the Anjay library during initialization.

### 7. Modified avs_commons_config.h

**File**: `APP/sc_app/code/Anjay/example_configs/embedded_lwm2m12/avsystem/commons/avs_commons_config.h`

Disabled POSIX socket implementation:

- Changed `#define AVS_COMMONS_NET_WITH_POSIX_AVS_SOCKET` to `/* #undef AVS_COMMONS_NET_WITH_POSIX_AVS_SOCKET */`

## Architecture Notes

### DTLS Implementation

Uses Anjay's native mbedtls integration (`AVS_COMMONS_WITH_MBEDTLS`):

- Anjay internally uses mbedtls for DTLS 1.2
- PSK authentication mode
- AES-128-CCM / AES-128-GCM cipher suites

### Network Implementation

Uses custom network implementation (`AVS_COMMONS_NET_WITH_POSIX_AVS_SOCKET` disabled):

- Uses SIMCOM SDK's socket API
- Custom DNS resolution (`sAPI_TcpipGetaddrinfo`)
- Adapts to SIMCOM SDK's lwIP TCP/IP stack

### Bootstrap Flow

1. Client connects to Bootstrap server (`coaps://lwm2m-test.avsystem.io:5694`)
2. Authenticates using PSK (identity: `862095060018816`, key: `123456789`)
3. Bootstrap server provisions LwM2M Server configuration
4. Client connects to the provisioned LwM2M Server

## Build and Test

### Build Steps

```bash
cd APP
build.bat
```

### Expected Results

1. Build succeeds, producing `anjay_simcom.bin`
2. Flash to the A7672E module
3. Serial log output shows:
   - mbedtls initialization success
   - DTLS handshake start
   - Connection to Bootstrap server
   - LwM2M Server configuration received

## Notes

1. **Memory Usage**: The minimal mbedtls configuration reduces ROM/RAM usage
2. **PSK Credentials**: Modify `anjay_simcom_config.h` according to your actual Bootstrap server configuration
3. **Debugging**: Control log verbosity via `ANJAY_DEBUG_ENABLED` and `ANJAY_LOG_LEVEL`
4. **Network**: Ensure the SIMCOM module has registered on the network and obtained an IP address

## Next Steps

1. Build and compile
2. Flash to hardware
3. Monitor serial output
4. Verify Bootstrap connection
5. Verify LwM2M functionality
