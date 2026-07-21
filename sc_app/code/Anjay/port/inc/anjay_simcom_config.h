/**
 * @file anjay_simcom_config.h
 * @brief Anjay LWM2M client configuration for SIMCOM A7672E platform
 */

#ifndef ANJAY_SIMCOM_CONFIG_H
#define ANJAY_SIMCOM_CONFIG_H

/*===========================================================================
 * Bootstrap Server Configuration
 * Fill in your Bootstrap server details here
 *===========================================================================*/

/* Bootstrap Server URI (e.g., "coaps://bootstrap.example.com:5684") */
#ifndef BOOTSTRAP_SERVER_URI
#define BOOTSTRAP_SERVER_URI        "coaps://lwm2m-test.avsystem.io:5694"
#endif

/* Security Mode: ANJAY_SECURITY_NOSEC, ANJAY_SECURITY_PSK,
 * ANJAY_SECURITY_CERTIFICATE, or ANJAY_SECURITY_RPK */
#ifndef BOOTSTRAP_SECURITY_MODE
#define BOOTSTRAP_SECURITY_MODE     ANJAY_SECURITY_PSK
#endif

/* PSK Identity (for PSK mode) */
#ifndef BOOTSTRAP_PSK_IDENTITY
#define BOOTSTRAP_PSK_IDENTITY      "862095060018816"
#endif

/* PSK Key (for PSK mode, hex string) */
#ifndef BOOTSTRAP_PSK_KEY
#define BOOTSTRAP_PSK_KEY           "123456789"
#endif

/* Server Certificate (for Certificate mode) */
#ifndef BOOTSTRAP_SERVER_CERT
#define BOOTSTRAP_SERVER_CERT       NULL
#endif
#ifndef BOOTSTRAP_SERVER_CERT_SIZE
#define BOOTSTRAP_SERVER_CERT_SIZE  0
#endif

/* Client Certificate (for Certificate mode) */
#ifndef BOOTSTRAP_CLIENT_CERT
#define BOOTSTRAP_CLIENT_CERT       NULL
#endif
#ifndef BOOTSTRAP_CLIENT_CERT_SIZE
#define BOOTSTRAP_CLIENT_CERT_SIZE  0
#endif

/* Client Private Key (for Certificate mode) */
#ifndef BOOTSTRAP_CLIENT_KEY
#define BOOTSTRAP_CLIENT_KEY        NULL
#endif
#ifndef BOOTSTRAP_CLIENT_KEY_SIZE
#define BOOTSTRAP_CLIENT_KEY_SIZE   0
#endif

/*===========================================================================
 * LWM2M Client Configuration
 *===========================================================================*/

/* Endpoint name - must be unique for each device */
#ifndef ANJAY_ENDPOINT_NAME
#define ANJAY_ENDPOINT_NAME         "simcom_a7672e_device"
#endif

/* Buffer sizes (adjust based on available RAM) */
#ifndef ANJAY_IN_BUFFER_SIZE
#define ANJAY_IN_BUFFER_SIZE        4000
#endif

#ifndef ANJAY_OUT_BUFFER_SIZE
#define ANJAY_OUT_BUFFER_SIZE       4000
#endif

#ifndef ANJAY_MSG_CACHE_SIZE
#define ANJAY_MSG_CACHE_SIZE        4000
#endif

/* Lifetime in seconds (server will expect updates within this period) */
#ifndef ANJAY_LIFETIME
#define ANJAY_LIFETIME              60
#endif

/*===========================================================================
 * Network Configuration
 *===========================================================================*/

/* Socket receive timeout in seconds */
#ifndef SOCKET_RECV_TIMEOUT_SEC
#define SOCKET_RECV_TIMEOUT_SEC     30
#endif

/* Maximum MTU for CoAP messages */
#ifndef ANJAY_MAX_MTU
#define ANJAY_MAX_MTU               1464
#endif

/*===========================================================================
 * Debug Configuration
 *===========================================================================*/

/* Enable/disable debug logging */
#ifndef ANJAY_DEBUG_ENABLED
#define ANJAY_DEBUG_ENABLED         1
#endif

/* Log level: 0=TRACE, 1=DEBUG, 2=INFO, 3=WARNING, 4=ERROR */
#ifndef ANJAY_LOG_LEVEL
#define ANJAY_LOG_LEVEL             2
#endif

#endif /* ANJAY_SIMCOM_CONFIG_H */
