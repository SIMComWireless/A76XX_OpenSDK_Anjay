/**
 * @file anjay_main.c
 * @brief Main LWM2M client implementation for SIMCOM A7672E
 *
 * This file contains the Anjay client initialization and main event loop.
 * It is designed to be called from the SDK's userSpace_Main() or from
 * a dedicated task.
 */

#include <string.h>
#include <stdbool.h>

/* SIMCOM SDK headers */
#include "simcom_os.h"
#include "simcom_debug.h"
#include "scfw_socket.h"

/* Anjay headers */
#include <anjay/anjay.h>
#include <anjay/security.h>
#include <anjay/server.h>
#include <anjay/bootstrap.h>
#include <avsystem/commons/avs_log.h>

/* Platform headers */
#include "simcom_platform.h"
#include "anjay_simcom_config.h"

#define LOG_MODULE "anjay_main"
#define LOG(level, ...) SIMCOM_LOG_##level(LOG_MODULE, __VA_ARGS__)

/*===========================================================================
 * Global variables
 *===========================================================================*/
static anjay_t *g_anjay = NULL;
static bool g_anjay_running = false;

/*===========================================================================
 * Security object setup (Bootstrap mode)
 *===========================================================================*/
static int setup_security_object(anjay_t *anjay) {
    if (anjay_security_object_install(anjay)) {
        LOG(ERROR, "Failed to install Security object");
        return -1;
    }

    /*
     * For Bootstrap mode, we add a Bootstrap Security instance.
     * The Bootstrap server will provision the actual server credentials.
     */
    anjay_security_instance_t security_instance = {
        .ssid = 0,  /* Bootstrap server uses SSID 0 */
        .server_uri = BOOTSTRAP_SERVER_URI,
        .security_mode = BOOTSTRAP_SECURITY_MODE,
        .public_cert_or_psk_identity = (const uint8_t *)BOOTSTRAP_PSK_IDENTITY,
        .public_cert_or_psk_identity_size = strlen(BOOTSTRAP_PSK_IDENTITY),
        .private_cert_or_psk_key = (const uint8_t *)BOOTSTRAP_PSK_KEY,
        .private_cert_or_psk_key_size = strlen(BOOTSTRAP_PSK_KEY),
        .is_bootstrap = true  /* Mark as Bootstrap server */
    };

    anjay_iid_t security_instance_id = ANJAY_ID_INVALID;
    if (anjay_security_object_add_instance(anjay, &security_instance,
                                           &security_instance_id)) {
        LOG(ERROR, "Failed to add Security instance");
        return -1;
    }

    LOG(INFO, "Bootstrap Security instance added (SSID=%d, URI=%s)",
        security_instance_id, BOOTSTRAP_SERVER_URI);

    return 0;
}

/*===========================================================================
 * Bootstrap object setup
 *===========================================================================*/
static int setup_bootstrap_object(anjay_t *anjay) {
    if (anjay_bootstrap_object_install(anjay)) {
        LOG(ERROR, "Failed to install Bootstrap object");
        return -1;
    }

    LOG(INFO, "Bootstrap object installed");
    return 0;
}

/*===========================================================================
 * Main event loop
 *===========================================================================*/
static int anjay_event_loop(anjay_t *anjay) {
    LOG(INFO, "Entering Anjay event loop");

    while (g_anjay_running) {
        /* Obtain all network data sources */
        AVS_LIST(avs_net_socket_t *const) sockets = anjay_get_sockets(anjay);

        /* Prepare to select() on them */
        size_t numsocks = AVS_LIST_SIZE(sockets);

        if (numsocks == 0) {
            /* No sockets yet, just run scheduler and wait */
            anjay_sched_run(anjay);
            simcom_platform_sleep_ms(100);
            continue;
        }

        /* Use select() for socket events (SDK supports select) */
        fd_set readfds;
        int maxfd = 0;
        FD_ZERO(&readfds);

        AVS_LIST(avs_net_socket_t *const) sock;
        AVS_LIST_FOREACH(sock, sockets) {
            const int *fd_ptr = (const int *)avs_net_socket_get_system(*sock);
            if (fd_ptr && *fd_ptr >= 0) {
                FD_SET(*fd_ptr, &readfds);
                if (*fd_ptr > maxfd) {
                    maxfd = *fd_ptr;
                }
            }
        }

        /* Determine wait time from scheduler */
        const int max_wait_time_ms = 1000;
        int wait_ms = anjay_sched_calculate_wait_time_ms(anjay, max_wait_time_ms);

        struct timeval tv = {
            .tv_sec = wait_ms / 1000,
            .tv_usec = (wait_ms % 1000) * 1000
        };

        /* Wait for events */
        int ret = select(maxfd + 1, &readfds, NULL, NULL, &tv);
        if (ret > 0) {
            AVS_LIST(avs_net_socket_t *const) socket = NULL;
            AVS_LIST_FOREACH(socket, sockets) {
                const int *fd_ptr =
                        (const int *)avs_net_socket_get_system(*socket);
                if (fd_ptr && *fd_ptr >= 0 && FD_ISSET(*fd_ptr, &readfds)) {
                    if (anjay_serve(anjay, *socket)) {
                        LOG(ERROR, "anjay_serve failed");
                    }
                }
            }
        }

        /* Run scheduled tasks */
        anjay_sched_run(anjay);
    }

    LOG(INFO, "Exiting Anjay event loop");
    return 0;
}

/*===========================================================================
 * Public API
 *===========================================================================*/

/**
 * @brief Initialize and start the Anjay LWM2M client
 *
 * This function initializes the network, creates the Anjay instance,
 * sets up Bootstrap, and enters the event loop.
 *
 * @return 0 on success, -1 on failure
 */
int anjay_client_start(void) {
    LOG(INFO, "Starting Anjay LWM2M client...");
    LOG(INFO, "Endpoint: %s", ANJAY_ENDPOINT_NAME);
    LOG(INFO, "Bootstrap URI: %s", BOOTSTRAP_SERVER_URI);

    /* Note: Anjay with native mbedtls integration handles initialization automatically */

    /* Configure Anjay */
    const anjay_configuration_t config = {
        .endpoint_name = ANJAY_ENDPOINT_NAME,
        .in_buffer_size = ANJAY_IN_BUFFER_SIZE,
        .out_buffer_size = ANJAY_OUT_BUFFER_SIZE,
        .msg_cache_size = ANJAY_MSG_CACHE_SIZE
    };

    /* Create Anjay instance */
    g_anjay = anjay_new(&config);
    if (!g_anjay) {
        LOG(ERROR, "Failed to create Anjay instance");
        return -1;
    }

    /* Setup Security object with Bootstrap server */
    if (setup_security_object(g_anjay)) {
        anjay_delete(g_anjay);
        g_anjay = NULL;
        return -1;
    }

    /* Setup Bootstrap object */
    if (setup_bootstrap_object(g_anjay)) {
        anjay_delete(g_anjay);
        g_anjay = NULL;
        return -1;
    }

    LOG(INFO, "Anjay initialized successfully, starting event loop");

    /* Enter event loop */
    g_anjay_running = true;
    int result = anjay_event_loop(g_anjay);

    /* Cleanup */
    g_anjay_running = false;
    anjay_delete(g_anjay);
    g_anjay = NULL;

    /* Note: Anjay with native mbedtls integration handles SSL cleanup automatically */

    LOG(INFO, "Anjay client stopped");
    return result;
}

/**
 * @brief Stop the Anjay LWM2M client
 *
 * This function signals the event loop to exit.
 */
void anjay_client_stop(void) {
    LOG(INFO, "Stopping Anjay client...");
    g_anjay_running = false;
}

/**
 * @brief Check if the Anjay client is running
 * @return true if running, false otherwise
 */
bool anjay_client_is_running(void) {
    return g_anjay_running && (g_anjay != NULL);
}

/*===========================================================================
 * Task entry point (for running in a separate task)
 *===========================================================================*/

/* Task stack and handle */
#define ANJAY_TASK_STACK_SIZE (1024 * 16)  /* 16KB stack */
static char g_anjay_task_stack[ANJAY_TASK_STACK_SIZE];
static sTaskRef g_anjay_task_ref = NULL;

/**
 * @brief Anjay task entry function
 * @param arg Task argument (unused)
 */
static void anjay_task_entry(void *arg) {
    (void)arg;

    /* Wait for system initialization */
    sAPI_TaskSleep(2000);

    /* Start the client */
    anjay_client_start();

    /* Task should not reach here unless client stops */
    LOG(INFO, "Anjay task exiting");
}

/**
 * @brief Start Anjay client in a dedicated task
 * @return 0 on success, -1 on failure
 */
int anjay_client_start_task(void) {
    LOG(INFO, "Creating Anjay task (stack=%d bytes)", ANJAY_TASK_STACK_SIZE);

    int ret = sAPI_TaskCreate(&g_anjay_task_ref,
                              g_anjay_task_stack,
                              ANJAY_TASK_STACK_SIZE,
                              200,  /* Priority 200 (lower than system tasks) */
                              "anjay_lwm2m",
                              anjay_task_entry,
                              NULL);
    if (ret != 0) {
        LOG(ERROR, "Failed to create Anjay task");
        return -1;
    }

    LOG(INFO, "Anjay task created successfully");
    return 0;
}
