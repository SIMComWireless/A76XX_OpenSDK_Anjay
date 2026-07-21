# CMakeLists.txt for Anjay port layer on SIMCOM A7672E
#
# 使用 Anjay 原生 mbedtls 集成实现 DTLS 支持

cmake_minimum_required(VERSION 3.10)

# ============================================================================
# mbedtls 源码路径
# ============================================================================

set(MBEDTLS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../Third_Party/mbedtls)

# ============================================================================
# mbedtls 源文件（精简集，仅包含 DTLS+PSK 所需）
# ============================================================================

set(MBEDTLS_SOURCES
    # 基础加密
    ${MBEDTLS_SRC_DIR}/library/aes.c
    ${MBEDTLS_SRC_DIR}/library/cipher.c
    ${MBEDTLS_SRC_DIR}/library/cipher_wrap.c
    ${MBEDTLS_SRC_DIR}/library/ccm.c
    ${MBEDTLS_SRC_DIR}/library/gcm.c
    ${MBEDTLS_SRC_DIR}/library/ctr_drbg.c
    ${MBEDTLS_SRC_DIR}/library/entropy.c
    ${MBEDTLS_SRC_DIR}/library/entropy_poll.c
    ${MBEDTLS_SRC_DIR}/library/sha256.c
    ${MBEDTLS_SRC_DIR}/library/md.c
    ${MBEDTLS_SRC_DIR}/library/bignum.c

    # ASN1 支持
    ${MBEDTLS_SRC_DIR}/library/asn1parse.c

    # SSL/TLS/DTLS 核心
    ${MBEDTLS_SRC_DIR}/library/ssl_tls.c
    ${MBEDTLS_SRC_DIR}/library/ssl_cli.c
    ${MBEDTLS_SRC_DIR}/library/ssl_msg.c
    ${MBEDTLS_SRC_DIR}/library/ssl_ciphersuites.c
    ${MBEDTLS_SRC_DIR}/library/ssl_cookie.c
    ${MBEDTLS_SRC_DIR}/library/debug.c

    # 平台支持
    ${MBEDTLS_SRC_DIR}/library/platform.c
    ${MBEDTLS_SRC_DIR}/library/platform_util.c
    ${MBEDTLS_SRC_DIR}/library/error.c
    ${MBEDTLS_SRC_DIR}/library/timing.c
)

# ============================================================================
# Port 层源文件（移除 tls_impl.c，由 Anjay 原生 mbedtls 处理 DTLS）
# ============================================================================

set(PORT_SOURCES
    src/net_impl.c
    src/platform_impl.c
    src/anjay_main.c
)

# ============================================================================
# Port 层头文件包含路径
# ============================================================================

set(PORT_INCLUDES
    inc
    config
    ${ANJAY_DIR}/include_public
    ${ANJAY_DIR}/deps/avs_commons/include_public
    ${ANJAY_DIR}/deps/avs_coap/include_public
    ${SDK_INC_DIR}
    ${MBEDTLS_SRC_DIR}/include
    ${MBEDTLS_SRC_DIR}/library
)

# ============================================================================
# 创建 anjay_port 静态库（包含 mbedtls）
# ============================================================================

add_library(anjay_port STATIC ${PORT_SOURCES})

target_include_directories(anjay_port PUBLIC ${PORT_INCLUDES})

# 链接 mbedtls 库
target_link_libraries(anjay_port PUBLIC mbedtls_local)

# ============================================================================
# 链接依赖
# ============================================================================

target_link_libraries(anjay_port PUBLIC
    anjay
    avs_coap
    ${AVS_COMMONS_LIBRARIES}
)

# ============================================================================
# 编译定义
# ============================================================================

target_compile_definitions(anjay_port PUBLIC
    # 使用 Anjay 原生 mbedtls 集成
    AVS_COMMONS_WITH_MBEDTLS
    SIMCOM_SDK=ON
    # 指定 mbedtls 配置文件
    MBEDTLS_CONFIG_FILE="mbedtls/anjay_mbedtls_config.h"
)

# ============================================================================
# 编译选项
# ============================================================================

target_compile_options(anjay_port PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    # mbedtls 警告抑制
    -Wno-missing-declarations
    -Wno-missing-prototypes
)
