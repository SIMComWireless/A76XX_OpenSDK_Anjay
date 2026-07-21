/**
 * @file tls_impl.c
 * @brief TLS/DTLS 实现说明
 *
 * 当使用 AVS_COMMONS_WITH_MBEDTLS 配置时，Anjay 库内部使用 mbedtls
 * 实现完整的 DTLS 功能，包括：
 * - DTLS 1.2 协议
 * - PSK 认证模式
 * - AES-128-CCM / AES-128-GCM 密码套件
 *
 * 本文件不再需要自定义 TLS 实现。所有 DTLS 功能由 Anjay 的
 * avs_commons/net/src/ssl/mbedtls 模块处理。
 *
 * 如果需要自定义 TLS 实现（例如使用硬件加密引擎），
 * 请改用 AVS_COMMONS_WITH_CUSTOM_TLS 配置。
 */

/*
 * 此文件故意留空。
 * Anjay 使用 mbedtls 原生 DTLS 实现。
 */
