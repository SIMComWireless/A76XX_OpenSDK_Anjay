/**
 * @file anjay_main.h
 * @brief Main LWM2M client API for SIMCOM A7672E
 */

#ifndef ANJAY_MAIN_H
#define ANJAY_MAIN_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize and start the Anjay LWM2M client
 *
 * This function initializes the network, creates the Anjay instance,
 * sets up Bootstrap, and enters the event loop. This is a blocking call.
 *
 * @return 0 on success, -1 on failure
 */
int anjay_client_start(void);

/**
 * @brief Start Anjay client in a dedicated task
 *
 * Creates a background task that runs the LWM2M client.
 * This is a non-blocking call.
 *
 * @return 0 on success, -1 on failure
 */
int anjay_client_start_task(void);

/**
 * @brief Stop the Anjay LWM2M client
 *
 * Signals the event loop to exit. If running in a task, the task will
 * exit on the next loop iteration.
 */
void anjay_client_stop(void);

/**
 * @brief Check if the Anjay client is running
 * @return true if running, false otherwise
 */
bool anjay_client_is_running(void);

#ifdef __cplusplus
}
#endif

#endif /* ANJAY_MAIN_H */
