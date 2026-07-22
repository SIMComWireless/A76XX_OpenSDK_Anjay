# CMakeLists.txt for Anjay LwM2M client on SIMCOM A7672E
#
# 此文件会被复制到 SDK/SIMCOM_SDK_SET/ 进行编译

cmake_minimum_required(VERSION 3.10)

# ============================================================================
# Anjay 源码目录
# ============================================================================

set(ANJAY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/code/Anjay)
set(ANJAY_PORT_DIR ${ANJAY_DIR}/port)

# ============================================================================
# mbedtls 源码目录
# ============================================================================

set(MBEDTLS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/code/Third_Party/mbedtls)

# ============================================================================
# SDK 头文件路径
# ============================================================================

set(SDK_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../sc_lib/A7672E_FASE_1603_V201_DS_OPENSDK/inc)

# ============================================================================
# 首先编译 mbedtls 库
# ============================================================================

# mbedtls 源文件（精简集，仅包含 DTLS+PSK 所需）
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

# 创建 mbedtls 静态库
add_library(mbedtls_local STATIC ${MBEDTLS_SOURCES})

# 设置 mbedtls 头文件包含路径
target_include_directories(mbedtls_local PUBLIC
    ${MBEDTLS_SRC_DIR}/include
    ${MBEDTLS_SRC_DIR}/library
)

# 设置 mbedtls 编译定义（使用我们的精简配置）
# 使用 PUBLIC 确保所有链接 mbedtls_local 的目标（包括 avs_commons）都使用自定义配置
target_compile_definitions(mbedtls_local PUBLIC
    MBEDTLS_CONFIG_FILE="mbedtls/anjay_mbedtls_config.h"
)

# 设置 mbedtls 编译选项
target_compile_options(mbedtls_local PRIVATE
    -Wno-missing-declarations
    -Wno-unused-parameter
    -Wno-missing-prototypes
)

# 禁用安装规则（避免导出问题）
set_target_properties(mbedtls_local PROPERTIES
    EXCLUDE_FROM_ALL TRUE
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mbedtls_output
    OUTPUT_NAME mbedtls
)

# ============================================================================
# 设置 FindMbedTLS 需要的变量
# ============================================================================

# 设置 mbedtls 相关变量，供 avs_commons 的 FindMbedTLS 使用
# 注意：使用实际文件路径而非目标名，因为 avs_commons 把这些值直接传给链接器
set(MBEDTLS_FOUND TRUE CACHE BOOL "" FORCE)
set(MBEDTLS_INCLUDE_DIR ${MBEDTLS_SRC_DIR}/include CACHE PATH "" FORCE)
set(MBEDTLS_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/mbedtls_output/libmbedtls.a CACHE STRING "" FORCE)
set(MBEDTLS_CRYPTO_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/mbedtls_output/libmbedtls.a CACHE STRING "" FORCE)
set(MBEDTLS_X509_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/mbedtls_output/libmbedtls.a CACHE STRING "" FORCE)
set(MBEDTLS_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/mbedtls_output/libmbedtls.a CACHE STRING "" FORCE)
set(MBEDTLS_VERSION "2.23.0" CACHE STRING "" FORCE)
set(MBEDTLS_VERSION_MAJOR 2 CACHE STRING "" FORCE)
set(MBEDTLS_VERSION_MINOR 23 CACHE STRING "" FORCE)
set(MBEDTLS_VERSION_PATCH 0 CACHE STRING "" FORCE)

# ============================================================================
# 配置文件路径（port/config 目录）
# ============================================================================

set(ANJAY_PORT_CONFIG_DIR ${ANJAY_PORT_DIR}/config)

# ============================================================================
# Anjay 构建选项
# ============================================================================

# 禁用 Anjay 示例和测试
set(WITH_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WITH_TESTS OFF CACHE BOOL "" FORCE)

# 使用 mbedtls 作为 DTLS 后端
set(DTLS_BACKEND "mbedtls" CACHE STRING "" FORCE)

# 启用 PSK 支持
set(WITH_PSK ON CACHE BOOL "" FORCE)

# 禁用 PKI（PSK 模式不需要证书）
set(WITH_PKI OFF CACHE BOOL "" FORCE)

# 设置 mbedtls 使用静态库
set(MBEDTLS_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)

# 启用 mbedtls 支持
set(WITH_MBEDTLS ON CACHE BOOL "" FORCE)

# 禁用 POSIX socket实现（使用自定义网络实现）
set(WITH_POSIX_AVS_SOCKET OFF CACHE BOOL "" FORCE)

# 启用线程支持，但使用 atomic_spinlock 后端（不依赖 pthread）
set(WITH_AVS_COMPAT_THREADING ON CACHE BOOL "" FORCE)
set(WITH_AVS_COMPAT_THREADING_PTHREAD OFF CACHE BOOL "" FORCE)
set(WITH_AVS_COMPAT_THREADING_ATOMIC_SPINLOCK ON CACHE BOOL "" FORCE)

# 禁用安装规则（避免导出问题）
set(CMAKE_SKIP_INSTALL_RULES ON CACHE BOOL "" FORCE)

# ============================================================================
# 构建 Anjay 库
# ============================================================================

# 传递 mbedtls 自定义配置给 avs_commons 子目录
# FindMbedTLS.cmake 创建的 IMPORTED 目标不是 GLOBAL 的，
# 父目录无法通过 set_target_properties 设置属性，
# 所以通过变量传递，在 avs_net_mbedtls 目标创建时直接添加编译定义
set(MBEDTLS_CONFIG_FILE_VALUE "\"mbedtls/anjay_mbedtls_config.h\"")

# 添加 Anjay 子目录
add_subdirectory(${ANJAY_DIR} anjay_build)

# ============================================================================
# 构建 Port 层
# ============================================================================

# 设置 Anjay 目录变量供 port 层使用
set(ANJAY_DIR ${ANJAY_DIR})
set(SDK_INC_DIR ${SDK_INC_DIR})

# 添加 Port 子目录
add_subdirectory(${ANJAY_PORT_DIR} anjay_port_build)

# ============================================================================
# 主应用程序库（SDK 期望 sc_app 是一个静态库）
# ============================================================================

# 确保 avs_commons 输出目录存在（avs_commons 会将库输出到 anjay_build/deps/avs_commons/output/lib/）
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/anjay_build/deps/avs_commons/output/lib)

# 创建INTERFACE库（无源文件），聚合所有依赖供SDK的userspace目标链接
add_library(sc_app INTERFACE)

# 允许多重定义（解决 avs_commons 库重复链接问题）
target_link_options(sc_app INTERFACE
    --allow-multiple-definition
)

# 传递配置文件目录
target_include_directories(sc_app INTERFACE
    ${ANJAY_PORT_CONFIG_DIR}
)

# 传递所有库依赖
# 注意：mbedtls_local 使用生成器表达式，因为INTERFACE库传递到父目录时目标名无法正确解析
target_link_libraries(sc_app INTERFACE
    anjay_port
    anjay
    avs_coap
    $<TARGET_FILE:mbedtls_local>
)

# ============================================================================
# 链接 SDK 库
# ============================================================================

# SDK 预编译库路径
set(SDK_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../sc_lib/A7672E_FASE_1603_V201_DS_OPENSDK/lib)

# SDK库已由SDK的CMakeLists.txt自动链接，不需要在这里重复链接

# INTERFACE库不需要输出文件配置（SDK通过target_link_libraries引用此目标）
