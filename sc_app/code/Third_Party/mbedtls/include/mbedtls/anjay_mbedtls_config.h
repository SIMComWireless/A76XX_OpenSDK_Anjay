/**
 * @file anjay_mbedtls_config.h
 * @brief 精简的 mbedtls 配置，用于 Anjay LwM2M 客户端在 SIMCOM A7672E 平台
 *
 * 仅启用 DTLS + PSK 所需的最小功能集，以节省 ROM/RAM 空间。
 */

#ifndef ANJAY_MBEDTLS_CONFIG_H
#define ANJAY_MBEDTLS_CONFIG_H

#include <limits.h>
#include <stdlib.h>

/* ============================================================================
 * 基础加密算法
 * ============================================================================ */

/* AES 块加密 - DTLS 必需 */
#define MBEDTLS_AES_C

/* CCM 模式 - AES-CCM 密码套件需要 */
#define MBEDTLS_CCM_C

/* GCM 模式 - AES-GCM 密码套件需要 */
#define MBEDTLS_GCM_C

/* 通用加密层 */
#define MBEDTLS_CIPHER_C

/* CTR_DRBG 随机数生成器 */
#define MBEDTLS_CTR_DRBG_C

/* 熵源 */
#define MBEDTLS_ENTROPY_C

/* SHA-256 哈希 - DTLS 握手需要 */
#define MBEDTLS_SHA256_C

/* 消息摘要层 */
#define MBEDTLS_MD_C

/* ASN1 解析（最小集） */
#define MBEDTLS_ASN1_PARSE_C

/* ============================================================================
 * DTLS/TLS 协议支持
 * ============================================================================ */

/* DTLS 协议支持 - LwM2M CoAP 必需 */
#define MBEDTLS_SSL_PROTO_DTLS

/* TLS 1.2 协议（DTLS 1.2 基于此） */
#define MBEDTLS_SSL_PROTO_TLS1_2

/* SSL 客户端功能 */
#define MBEDTLS_SSL_CLI_C

/* SSL/TLS 核心层 */
#define MBEDTLS_SSL_TLS_C

/* 发送所有告警消息 */
#define MBEDTLS_SSL_ALL_ALERT_MESSAGES

/* 最大分片长度支持 */
#define MBEDTLS_SSL_MAX_FRAGMENT_LENGTH

/* 导出密钥块支持 */
#define MBEDTLS_SSL_EXPORT_KEYS

/* ============================================================================
 * PSK 认证
 * ============================================================================ */

/* PSK 密钥交换 - Bootstrap 安全模式需要 */
#define MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

/* ============================================================================
 * DTLS 安全特性
 * ============================================================================ */

/* 防重放攻击保护 */
#define MBEDTLS_SSL_DTLS_ANTI_REPLAY

/* HelloVerifyRequest 支持 */
#define MBEDTLS_SSL_DTLS_HELLO_VERIFY

/* 错误 MAC 限制 */
#define MBEDTLS_SSL_DTLS_BADMAC_LIMIT

/* ============================================================================
 * 平台适配
 * ============================================================================ */

/* 允许内联汇编优化 */
#define MBEDTLS_HAVE_ASM

/* 使用自定义 timing 实现（SIMCOM 平台没有 Unix/Windows API） */
#define MBEDTLS_TIMING_ALT

/* 使用标准库内存分配 */
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_CALLOC_MACRO   calloc
#define MBEDTLS_PLATFORM_FREE_MACRO     free

/* 禁用平台熵源（嵌入式平台没有 /dev/urandom 或 Windows CryptoAPI） */
#define MBEDTLS_NO_PLATFORM_ENTROPY

/* 使用自定义硬件熵源（需实现 mbedtls_hardware_poll） */
#define MBEDTLS_ENTROPY_HARDWARE_ALT

/* 禁用时间日期功能（嵌入式平台可能没有 RTC） */
/* #undef MBEDTLS_HAVE_TIME */
/* #undef MBEDTLS_HAVE_TIME_DATE */

/* ============================================================================
 * 禁用不需要的功能以节省空间
 * ============================================================================ */

/* 禁用文件系统 I/O */
#undef MBEDTLS_FS_IO

/* 禁用网络层（使用 SIMCOM SDK 的 socket API） */
#undef MBEDTLS_NET_C

/* 禁用 SSL 服务器功能 */
#undef MBEDTLS_SSL_SRV_C

/* 禁用 X.509 证书解析（PSK 模式不需要） */
#undef MBEDTLS_X509_CRT_PARSE_C
#undef MBEDTLS_X509_C
#undef MBEDTLS_CERTS_C

/* 禁用公钥加密（PSK 模式不需要） */
#undef MBEDTLS_PK_C
#undef MBEDTLS_PK_PARSE_C
#undef MBEDTLS_RSA_C
#undef MBEDTLS_ECP_C
#undef MBEDTLS_ECDH_C
#undef MBEDTLS_ECDSA_C
#undef MBEDTLS_DHM_C

/* 禁用其他不需要的功能 */
#undef MBEDTLS_PSA_CRYPTO_C
#undef MBEDTLS_SELF_TEST
#undef MBEDTLS_VERSION_FEATURES

#endif /* ANJAY_MBEDTLS_CONFIG_H */
