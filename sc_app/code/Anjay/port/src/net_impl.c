/**
 * @file net_impl.c
 * @brief Network layer implementation for Anjay on SIMCOM A7672E platform
 *
 * Uses the SDK's POSIX-compatible socket API (lwIP-based)
 */

#include <string.h>
#include <stdlib.h>

/* SIMCOM SDK headers */
#include "scfw_socket.h"
#include "scfw_socket_define.h"
#include "scfw_netdb.h"

/* Anjay headers */
#include <avsystem/commons/avs_socket_v_table.h>
#include <avsystem/commons/avs_log.h>

/* Platform config */
#include "anjay_simcom_config.h"

/* Ensure custom network implementation is used */
#ifdef AVS_COMMONS_NET_WITH_POSIX_AVS_SOCKET
#    error "Custom implementation conflicts with AVS_COMMONS_NET_WITH_POSIX_AVS_SOCKET"
#endif

#define LOG(level, ...) avs_log(simcom_net, level, __VA_ARGS__)

/*===========================================================================
 * Forward declarations for Anjay platform functions
 *===========================================================================*/
avs_error_t _avs_net_initialize_global_compat_state(void);
void _avs_net_cleanup_global_compat_state(void);
avs_error_t _avs_net_create_udp_socket(avs_net_socket_t **socket,
                                       const void *socket_configuration);
avs_error_t _avs_net_create_tcp_socket(avs_net_socket_t **socket,
                                       const void *socket_configuration);

/*===========================================================================
 * Socket implementation structure
 *===========================================================================*/
typedef struct {
    const avs_net_socket_v_table_t *operations;
    int fd;                         /* Socket file descriptor */
    int socktype;                   /* SOCK_DGRAM or SOCK_STREAM */
    avs_time_duration_t recv_timeout;
    char remote_host[256];          /* Remote host name */
    char remote_port[16];           /* Remote port string */
} net_socket_impl_t;

/*===========================================================================
 * Global state management
 *===========================================================================*/
avs_error_t _avs_net_initialize_global_compat_state(void) {
    LOG(AVS_LOG_INFO, "Network global state initialized");
    return AVS_OK;
}

void _avs_net_cleanup_global_compat_state(void) {
    LOG(AVS_LOG_INFO, "Network global state cleaned up");
}

/*===========================================================================
 * DNS resolution helper
 *===========================================================================*/
static avs_error_t resolve_address(const char *host, const char *port,
                                   int socktype, struct sockaddr_in *addr_out,
                                   socklen_t *addrlen_out) {
    /* Use SDK's DNS resolution */
    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = socktype
    };
    struct addrinfo *res = NULL;

    int err = sAPI_TcpipGetaddrinfo(host, port, &hints, &res);
    if (err != 0 || !res) {
        LOG(AVS_LOG_ERROR, "DNS resolution failed for %s:%s (err=%d)", host, port, err);
        return avs_errno(AVS_EADDRNOTAVAIL);
    }

    memcpy(addr_out, res->ai_addr, res->ai_addrlen);
    *addrlen_out = res->ai_addrlen;
    sAPI_TcpipFreeaddrinfo(res);

    return AVS_OK;
}

/*===========================================================================
 * Socket vtable operations
 *===========================================================================*/
static avs_error_t
net_connect(avs_net_socket_t *sock_, const char *host, const char *port) {
    net_socket_impl_t *sock = (net_socket_impl_t *)sock_;

    LOG(AVS_LOG_DEBUG, "net_connect: %s:%s (fd=%d)", host, port, sock->fd);

    /* Resolve address */
    struct sockaddr_in addr;
    socklen_t addrlen;
    avs_error_t err = resolve_address(host, port, sock->socktype, &addr, &addrlen);
    if (avs_is_err(err)) {
        return err;
    }

    /* Create socket if not already created */
    if (sock->fd < 0) {
        sock->fd = socket(AF_INET, sock->socktype, 0);
        if (sock->fd < 0) {
            LOG(AVS_LOG_ERROR, "socket() failed");
            return avs_errno(AVS_UNKNOWN_ERROR);
        }
    }

    /* Connect */
    if (connect(sock->fd, (struct sockaddr *)&addr, addrlen) != 0) {
        LOG(AVS_LOG_ERROR, "connect() failed");
        close(sock->fd);
        sock->fd = -1;
        return avs_errno(AVS_ECONNREFUSED);
    }

    /* Store remote info */
    strncpy(sock->remote_host, host, sizeof(sock->remote_host) - 1);
    strncpy(sock->remote_port, port, sizeof(sock->remote_port) - 1);

    return AVS_OK;
}

static avs_error_t
net_send(avs_net_socket_t *sock_, const void *buffer, size_t buffer_length) {
    net_socket_impl_t *sock = (net_socket_impl_t *)sock_;

    if (sock->fd < 0) {
        return avs_errno(AVS_EBADF);
    }

    ssize_t written = send(sock->fd, buffer, buffer_length, 0);
    if (written >= 0 && (size_t)written == buffer_length) {
        return AVS_OK;
    }

    LOG(AVS_LOG_ERROR, "send() failed or partial write");
    return avs_errno(AVS_EIO);
}

static avs_error_t net_receive(avs_net_socket_t *sock_,
                               size_t *out_bytes_received,
                               void *buffer,
                               size_t buffer_length) {
    net_socket_impl_t *sock = (net_socket_impl_t *)sock_;

    if (sock->fd < 0) {
        return avs_errno(AVS_EBADF);
    }

    /* Use select() for timeout since SDK supports it */
    fd_set readfds;
    struct timeval tv;
    int64_t timeout_ms;

    if (avs_time_duration_to_scalar(&timeout_ms, AVS_TIME_MS, sock->recv_timeout)) {
        timeout_ms = -1;
    } else if (timeout_ms < 0) {
        timeout_ms = 0;
    }

    FD_ZERO(&readfds);
    FD_SET(sock->fd, &readfds);

    if (timeout_ms >= 0) {
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
    }

    int ret = select(sock->fd + 1, &readfds, NULL, NULL,
                     (timeout_ms >= 0) ? &tv : NULL);
    if (ret == 0) {
        return avs_errno(AVS_ETIMEDOUT);
    } else if (ret < 0) {
        return avs_errno(AVS_EIO);
    }

    /* Receive data */
    ssize_t bytes_received = recv(sock->fd, buffer, buffer_length, 0);
    if (bytes_received < 0) {
        LOG(AVS_LOG_ERROR, "recv() failed");
        return avs_errno(AVS_EIO);
    }

    *out_bytes_received = (size_t)bytes_received;

    /* For UDP, check if buffer was too small */
    if (buffer_length > 0 && sock->socktype == SOCK_DGRAM
            && (size_t)bytes_received == buffer_length) {
        return avs_errno(AVS_EMSGSIZE);
    }

    return AVS_OK;
}

static avs_error_t net_bind(avs_net_socket_t *sock_, const char *address,
                            const char *port) {
    net_socket_impl_t *sock = (net_socket_impl_t *)sock_;

    LOG(AVS_LOG_DEBUG, "net_bind: %s:%s", address ? address : "any", port);

    /* Create socket if needed */
    if (sock->fd < 0) {
        sock->fd = socket(AF_INET, sock->socktype, 0);
        if (sock->fd < 0) {
            return avs_errno(AVS_UNKNOWN_ERROR);
        }
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));

    if (address && address[0]) {
        addr.sin_addr.s_addr = inet_addr(address);
    } else {
        addr.sin_addr.s_addr = INADDR_ANY;
    }

    if (bind(sock->fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        LOG(AVS_LOG_ERROR, "bind() failed");
        return avs_errno(AVS_EADDRINUSE);
    }

    return AVS_OK;
}

static avs_error_t net_close(avs_net_socket_t *sock_) {
    net_socket_impl_t *sock = (net_socket_impl_t *)sock_;
    avs_error_t err = AVS_OK;

    if (sock->fd >= 0) {
        if (close(sock->fd) != 0) {
            err = avs_errno(AVS_EIO);
        }
        sock->fd = -1;
    }

    return err;
}

static avs_error_t net_shutdown(avs_net_socket_t *sock_) {
    net_socket_impl_t *sock = (net_socket_impl_t *)sock_;

    if (sock->fd >= 0) {
        shutdown(sock->fd, SHUT_RDWR);
    }
    return AVS_OK;
}

static avs_error_t net_cleanup(avs_net_socket_t **sock_ptr) {
    avs_error_t err = AVS_OK;

    if (sock_ptr && *sock_ptr) {
        err = net_close(*sock_ptr);
        avs_free(*sock_ptr);
        *sock_ptr = NULL;
    }

    return err;
}

static const void *net_system_socket(avs_net_socket_t *sock_) {
    net_socket_impl_t *sock = (net_socket_impl_t *)sock_;
    return &sock->fd;
}

static avs_error_t net_get_remote_host(avs_net_socket_t *sock_,
                                       char *out_buffer,
                                       size_t out_buffer_size) {
    net_socket_impl_t *sock = (net_socket_impl_t *)sock_;

    if (sock->remote_host[0] == '\0') {
        return avs_errno(AVS_ENOENT);
    }

    strncpy(out_buffer, sock->remote_host, out_buffer_size);
    return AVS_OK;
}

static avs_error_t net_get_remote_hostname(avs_net_socket_t *sock_,
                                           char *out_buffer,
                                           size_t out_buffer_size) {
    return net_get_remote_host(sock_, out_buffer, out_buffer_size);
}

static avs_error_t net_get_remote_port(avs_net_socket_t *sock_,
                                       char *out_buffer,
                                       size_t out_buffer_size) {
    net_socket_impl_t *sock = (net_socket_impl_t *)sock_;

    if (sock->remote_port[0] == '\0') {
        return avs_errno(AVS_ENOENT);
    }

    strncpy(out_buffer, sock->remote_port, out_buffer_size);
    return AVS_OK;
}

static avs_error_t net_get_local_port(avs_net_socket_t *sock_,
                                      char *out_buffer,
                                      size_t out_buffer_size) {
    net_socket_impl_t *sock = (net_socket_impl_t *)sock_;

    if (sock->fd < 0) {
        return avs_errno(AVS_EBADF);
    }

    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if (getsockname(sock->fd, (struct sockaddr *)&addr, &addrlen) != 0) {
        return avs_errno(AVS_EIO);
    }

    snprintf(out_buffer, out_buffer_size, "%d", ntohs(addr.sin_port));
    return AVS_OK;
}

static avs_error_t net_get_opt(avs_net_socket_t *sock_,
                               avs_net_socket_opt_key_t option_key,
                               avs_net_socket_opt_value_t *out_option_value) {
    net_socket_impl_t *sock = (net_socket_impl_t *)sock_;

    switch (option_key) {
    case AVS_NET_SOCKET_OPT_RECV_TIMEOUT:
        out_option_value->recv_timeout = sock->recv_timeout;
        return AVS_OK;

    case AVS_NET_SOCKET_OPT_STATE:
        if (sock->fd < 0) {
            out_option_value->state = AVS_NET_SOCKET_STATE_CLOSED;
        } else {
            out_option_value->state = AVS_NET_SOCKET_STATE_CONNECTED;
        }
        return AVS_OK;

    case AVS_NET_SOCKET_OPT_INNER_MTU:
        out_option_value->mtu = ANJAY_MAX_MTU;
        return AVS_OK;

    case AVS_NET_SOCKET_HAS_BUFFERED_DATA:
        out_option_value->flag = false;
        return AVS_OK;

    default:
        return avs_errno(AVS_ENOTSUP);
    }
}

static avs_error_t net_set_opt(avs_net_socket_t *sock_,
                               avs_net_socket_opt_key_t option_key,
                               avs_net_socket_opt_value_t option_value) {
    net_socket_impl_t *sock = (net_socket_impl_t *)sock_;

    switch (option_key) {
    case AVS_NET_SOCKET_OPT_RECV_TIMEOUT:
        sock->recv_timeout = option_value.recv_timeout;
        return AVS_OK;

    default:
        return avs_errno(AVS_ENOTSUP);
    }
}

/*===========================================================================
 * Vtable definition
 *===========================================================================*/
static const avs_net_socket_v_table_t NET_SOCKET_VTABLE = {
    .connect = net_connect,
    .send = net_send,
    .receive = net_receive,
    .bind = net_bind,
    .close = net_close,
    .shutdown = net_shutdown,
    .cleanup = net_cleanup,
    .get_system_socket = net_system_socket,
    .get_remote_host = net_get_remote_host,
    .get_remote_hostname = net_get_remote_hostname,
    .get_remote_port = net_get_remote_port,
    .get_local_port = net_get_local_port,
    .get_opt = net_get_opt,
    .set_opt = net_set_opt
};

/*===========================================================================
 * Socket creation functions
 *===========================================================================*/
static avs_error_t
net_create_socket(avs_net_socket_t **socket_ptr,
                  const avs_net_socket_configuration_t *configuration,
                  int socktype) {
    if (!socket_ptr || *socket_ptr) {
        return avs_errno(AVS_EINVAL);
    }

    (void)configuration; /* Configuration not used for basic implementation */

    net_socket_impl_t *socket =
            (net_socket_impl_t *)avs_calloc(1, sizeof(net_socket_impl_t));
    if (!socket) {
        return avs_errno(AVS_ENOMEM);
    }

    socket->operations = &NET_SOCKET_VTABLE;
    socket->socktype = socktype;
    socket->fd = -1;
    socket->recv_timeout = avs_time_duration_from_scalar(
            SOCKET_RECV_TIMEOUT_SEC, AVS_TIME_S);
    socket->remote_host[0] = '\0';
    socket->remote_port[0] = '\0';

    *socket_ptr = (avs_net_socket_t *)socket;
    LOG(AVS_LOG_DEBUG, "Created %s socket", (socktype == SOCK_DGRAM) ? "UDP" : "TCP");

    return AVS_OK;
}

avs_error_t _avs_net_create_udp_socket(avs_net_socket_t **socket_ptr,
                                       const void *configuration) {
    return net_create_socket(
            socket_ptr,
            (const avs_net_socket_configuration_t *)configuration,
            SOCK_DGRAM);
}

avs_error_t _avs_net_create_tcp_socket(avs_net_socket_t **socket_ptr,
                                       const void *configuration) {
    return net_create_socket(
            socket_ptr,
            (const avs_net_socket_configuration_t *)configuration,
            SOCK_STREAM);
}
