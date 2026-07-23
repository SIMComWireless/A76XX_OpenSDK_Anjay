/**
 * @file anjay_mbedtls_config.h
 * @brief Minimal mbedtls configuration for Anjay LwM2M client on SIMCOM A7672E platform
 *
 * Only enables the minimum feature set required for DTLS + PSK to save ROM/RAM space.
 */

#ifndef ANJAY_MBEDTLS_CONFIG_H
#define ANJAY_MBEDTLS_CONFIG_H

#include <limits.h>
#include <stdlib.h>

/* ============================================================================
 * Core Cryptographic Algorithms
 * ============================================================================ */

/* AES block cipher - required for DTLS */
#define MBEDTLS_AES_C

/* CCM mode - required for AES-CCM cipher suites */
#define MBEDTLS_CCM_C

/* GCM mode - required for AES-GCM cipher suites (disabled, PSK mode only needs CCM) */
/* #undef MBEDTLS_GCM_C */

/* Generic cipher layer */
#define MBEDTLS_CIPHER_C

/* CTR_DRBG random number generator */
#define MBEDTLS_CTR_DRBG_C

/* Entropy source */
#define MBEDTLS_ENTROPY_C

/* SHA-256 hash - required for DTLS handshake */
#define MBEDTLS_SHA256_C

/* Message digest layer */
#define MBEDTLS_MD_C

/* ASN1 parsing (minimal set) */
#define MBEDTLS_ASN1_PARSE_C

/* ============================================================================
 * DTLS/TLS Protocol Support
 * ============================================================================ */

/* DTLS protocol support - required for LwM2M CoAP */
#define MBEDTLS_SSL_PROTO_DTLS

/* TLS 1.2 protocol (DTLS 1.2 is based on this) */
#define MBEDTLS_SSL_PROTO_TLS1_2

/* SSL client functionality */
#define MBEDTLS_SSL_CLI_C

/* SSL/TLS core layer */
#define MBEDTLS_SSL_TLS_C

/* Send all alert messages (not essential, saves space when disabled) */
/* #undef MBEDTLS_SSL_ALL_ALERT_MESSAGES */

/* Maximum fragment length support (not essential, saves space when disabled) */
/* #undef MBEDTLS_SSL_MAX_FRAGMENT_LENGTH */

/* Export key block support (not essential, saves space when disabled) */
/* #undef MBEDTLS_SSL_EXPORT_KEYS */

/* ============================================================================
 * PSK Authentication
 * ============================================================================ */

/* PSK key exchange - required for Bootstrap security mode */
#define MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

/* ============================================================================
 * DTLS Security Features
 * ============================================================================ */

/* Anti-replay protection */
#define MBEDTLS_SSL_DTLS_ANTI_REPLAY

/* HelloVerifyRequest support */
#define MBEDTLS_SSL_DTLS_HELLO_VERIFY

/* Bad MAC limit */
#define MBEDTLS_SSL_DTLS_BADMAC_LIMIT

/* ============================================================================
 * Platform Adaptation
 * ============================================================================ */

/* Allow inline assembly optimizations */
#define MBEDTLS_HAVE_ASM

/* Use custom timing implementation (SIMCOM platform has no Unix/Windows API) */
#define MBEDTLS_TIMING_ALT

/* Use standard library memory allocation */
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_CALLOC_MACRO   calloc
#define MBEDTLS_PLATFORM_FREE_MACRO     free

/* Disable platform entropy source (embedded platform has no /dev/urandom or Windows CryptoAPI) */
#define MBEDTLS_NO_PLATFORM_ENTROPY

/* Use custom hardware entropy source (must implement mbedtls_hardware_poll) */
#define MBEDTLS_ENTROPY_HARDWARE_ALT

/* Disable time/date functions (embedded platform may have no RTC) */
/* #undef MBEDTLS_HAVE_TIME */
/* #undef MBEDTLS_HAVE_TIME_DATE */

/* ============================================================================
 * Disable Unnecessary Features to Save Space
 * ============================================================================ */

/* Disable file system I/O */
#undef MBEDTLS_FS_IO

/* Disable network layer (using SIMCOM SDK socket API) */
#undef MBEDTLS_NET_C

/* Disable SSL server functionality */
#undef MBEDTLS_SSL_SRV_C

/* Disable X.509 certificate parsing (not needed for PSK mode) */
#undef MBEDTLS_X509_CRT_PARSE_C
#undef MBEDTLS_X509_C
#undef MBEDTLS_CERTS_C

/* Disable public key cryptography (not needed for PSK mode) */
#undef MBEDTLS_PK_C
#undef MBEDTLS_PK_PARSE_C
#undef MBEDTLS_RSA_C
#undef MBEDTLS_ECP_C
#undef MBEDTLS_ECDH_C
#undef MBEDTLS_ECDSA_C
#undef MBEDTLS_DHM_C

/* Disable other unnecessary features */
#undef MBEDTLS_PSA_CRYPTO_C
#undef MBEDTLS_SELF_TEST
#undef MBEDTLS_VERSION_FEATURES

#endif /* ANJAY_MBEDTLS_CONFIG_H */
