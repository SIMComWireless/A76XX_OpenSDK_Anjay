# Anjay LwM2M Client for SIMCOM A7672E

An open-source [LwM2M](https://www.openmobilealliance.org/release/LightweightM2M/) client for SIMCOM A7672E/A7672SA cellular modules, based on the [Anjay](https://github.com/AVSystem/Anjay) library and the SIMCOM OpenSDK.

## Features

- LwM2M 1.0/1.1 client with Bootstrap support
- PSK and NoSec security modes
- Built-in LwM2M Objects:
  - **Security (/0)** and **Server (/1)** — managed by Anjay core
  - **Device (/3)** — module info (manufacturer, model, IMEI, firmware version) and Reboot
  - **Connectivity Statistics (/7)** — TX/RX bytes and packet counts via SIMCOM SDK
- RTOS-based event loop running in a dedicated task
- UDP/IPv4 transport over cellular PPP link

## Prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| SIMCOM OpenSDK | A7672E_FASE_1603_V201 | Install to `SDK/SIMCOM_SDK_SET/` |
| GNUmake | Bundled with OpenSDK | `GNUmake.exe` in SDK root |
| Ninja | Bundled with OpenSDK | Used as build backend |
| ARM GCC | 9.2.1 (arm-none-eabi) | Bundled with OpenSDK |

## Project Structure

```
APP/
├── build.bat                       # Build script (copies sources to SDK, invokes GNUmake)
├── CMakeLists.txt                  # Top-level CMake for the application
├── .config                         # Build feature config (cJSON, modules, etc.)
├── output/                         # Build output (.zip firmware packages)
└── sc_app/
    ├── AppMain.c                   # Entry point: userSpace_Main()
    ├── include.h                   # Common SIMCOM SDK headers
    └── code/Anjay/
        ├── src/                    # Anjay core library source
        ├── deps/                   # Dependencies (avs_commons, avs_coap)
        └── port/                   # SIMCOM platform adaptation layer
            ├── config/             # Anjay/avs_commons build configuration headers
            ├── inc/
            │   ├── anjay_main.h           # Client start/stop API
            │   ├── anjay_simcom_config.h  # Server URI, PSK, endpoint name, buffer sizes
            │   ├── device_object.h        # Device Object (/3) interface
            │   ├── conn_stats_object.h    # Connectivity Statistics Object (/7) interface
            │   └── simcom_platform.h      # UART logging and platform utilities
            └── src/
                ├── anjay_main.c           # LwM2M client entry, event loop, Bootstrap setup
                ├── device_object.c        # Device Object implementation
                ├── conn_stats_object.c    # Connectivity Statistics implementation
                ├── net_impl.c             # UDP/TCP socket layer (lwIP POSIX API)
                ├── platform_impl.c        # Logging and utility functions
                └── timing_simcom.c        # Timing abstraction (sAPI_GetTicks)
```

## Build

### Setup

After cloning this repository, open `build.bat` and modify the SDK path on this line:

```bat
set EATSDKDIR=E:\OpenSDK\Anjay\SDK\SIMCOM_SDK_SET
```

Change it to the actual path where your SIMCOM OpenSDK is installed. This is the only modification needed before building.

### Quick Start

```cmd
cd APP
build.bat
```

The script will:

1. Copy `sc_app/` and `CMakeLists.txt` into the SDK directory
2. Run `GNUmake clean` followed by `gnumake <module>`
3. Copy the output `.zip` firmware package to `APP/output/`

### Pre-select a Module

To skip the interactive module selection prompt:

```cmd
set SELECTED_MODULE=A7672E_FASE_1603_V201_DS_OPENSDK
build.bat
```

### Build Output

The firmware `.zip` file is placed in `APP/output/`. Flash it to the module using SIMCOM's standard download tool.

## Configuration

Edit `sc_app/code/Anjay/port/inc/anjay_simcom_config.h`:

| Define | Default | Description |
|--------|---------|-------------|
| `BOOTSTRAP_SERVER_URI` | `coaps://lwm2m-test.avsystem.io:5694` | Bootstrap server URI |
| `BOOTSTRAP_SECURITY_MODE` | `ANJAY_SECURITY_PSK` | Security mode (NoSec, PSK, Certificate) |
| `BOOTSTRAP_PSK_IDENTITY` | `"862095060018816"` | PSK identity |
| `BOOTSTRAP_PSK_KEY` | `"123456789"` | PSK key (hex string) |
| `ANJAY_ENDPOINT_NAME` | `"XIOT_Client01"` | Device endpoint name (must be unique) |
| `ANJAY_LIFETIME` | `60` | Registration lifetime in seconds |
| `ANJAY_IN_BUFFER_SIZE` | `4000` | CoAP input buffer size |
| `ANJAY_OUT_BUFFER_SIZE` | `4000` | CoAP output buffer size |

## LwM2M Objects

### Device Object (/3)

Reports module information retrieved from SIMCOM SDK APIs (`sAPI_SysGetVersion`, `sAPI_SysGetImei`, `sAPI_SysGetRFVersion`).

| RID | Resource | Access | Description |
|-----|----------|--------|-------------|
| 0 | Manufacturer | R | Module manufacturer |
| 1 | Model Number | R | Module model string |
| 2 | Serial Number | R | IMEI |
| 3 | Firmware Version | R | Firmware revision |
| 4 | Reboot | E | Triggers `sAPI_SysReset()` |
| 16 | Binding Mode | R | Always "U" (UDP) |
| 17 | Device Type | R | Module model identifier |
| 18 | Hardware Version | R | Hardware revision |
| 19 | Software Version | R | SDK version |

### Connectivity Statistics Object (/7)

Reports network traffic statistics from `sAPI_GetStatisticsData()`. All values are deltas since the last reset.

| RID | Resource | Access | Description |
|-----|----------|--------|-------------|
| 2 | TX Data | R | Bytes transmitted |
| 3 | RX Data | R | Bytes received |
| 6 | Start/Reset | E | Snapshot current counters as new baseline |
| 10 | TX Packets | R | Packets transmitted (vendor extension) |
| 11 | RX Packets | R | Packets received (vendor extension) |
| 12 | TX Dropped | R | Dropped TX packets (vendor extension) |
| 13 | RX Dropped | R | Dropped RX packets (vendor extension) |

## Application Flow

```
userSpace_Main()
  ├── ApiMapInit()                  # SIMCOM SDK API init
  ├── simcom_uart_log_init()        # UART logging
  ├── sAPI_NetworkInit()            # Network subsystem
  ├── wait_for_sim_ready()          # Wait for SIM card
  ├── wait_for_network_reg()        # Wait for PS registration
  ├── activate_pdp()                # Activate PDP context
  ├── sync_ntp_time()               # NTP time sync
  └── anjay_client_start_task()     # Start LwM2M client task
        ├── anjay_new()             # Create Anjay instance
        ├── Install Security (/0) and Server (/1) objects
        ├── Install Device (/3) and Conn Stats (/7) objects
        ├── anjay_event_loop_run()  # Main event loop
        └── Cleanup on exit
```

## Adding Custom LwM2M Objects

1. Create `port/src/my_object.c` and `port/inc/my_object.h`
2. Define `anjay_dm_object_def_t` with required handlers
3. Export `my_object_def_ptr()` returning `const anjay_dm_object_def_t **`
4. Register in `anjay_main.c` via `anjay_register_object(g_anjay, my_object_def_ptr())`
5. Add the source file to `port/CMakeLists.txt` under `PORT_SOURCES`

## License

This project uses the following open-source components:

- [Anjay](https://github.com/AVSystem/Anjay) — Apache 2.0 License
- [avs_commons](https://github.com/AVSystem/avs_commons) — Apache 2.0 License
- [avs_coap](https://github.com/AVSystem/avs_coap) — Apache 2.0 License

SIMCOM OpenSDK is provided by [SIMCom Wireless](https://www.simcom.com/) and is subject to their license terms.
