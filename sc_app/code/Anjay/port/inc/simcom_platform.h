/**
 * @file simcom_platform.h
 * @brief Platform abstraction for Anjay on SIMCOM A7672E
 *
 * Provides threading, timing, and memory functions required by Anjay
 */

#ifndef SIMCOM_PLATFORM_H
#define SIMCOM_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* SIMCOM SDK headers */
#include "simcom_os.h"
#include "simcom_debug.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * Memory allocation wrappers
 *===========================================================================*/

/**
 * @brief Allocate memory block
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
static inline void *simcom_platform_malloc(size_t size) {
    return malloc(size);
}

/**
 * @brief Allocate zeroed memory block
 * @param count Number of elements
 * @param size Size of each element
 * @return Pointer to allocated memory, or NULL on failure
 */
static inline void *simcom_platform_calloc(size_t count, size_t size) {
    return calloc(count, size);
}

/**
 * @brief Reallocate memory block
 * @param ptr Pointer to previously allocated memory (or NULL)
 * @param size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 */
static inline void *simcom_platform_realloc(void *ptr, size_t size) {
    return realloc(ptr, size);
}

/**
 * @brief Free memory block
 * @param ptr Pointer to memory to free (or NULL)
 */
static inline void simcom_platform_free(void *ptr) {
    free(ptr);
}

/*===========================================================================
 * Timing functions
 *===========================================================================*/

/**
 * @brief Get system tick count in milliseconds
 * @return Current tick count
 */
static inline uint32_t simcom_platform_get_tick_ms(void) {
    return (uint32_t)(sAPI_GetTicks() * portTICK_PERIOD_MS);
}

/**
 * @brief Sleep for specified milliseconds
 * @param ms Time to sleep in milliseconds
 */
static inline void simcom_platform_sleep_ms(uint32_t ms) {
    sAPI_TaskSleep(ms / portTICK_PERIOD_MS);
}

/*===========================================================================
 * Thread creation helper
 *===========================================================================*/

/**
 * @brief Thread function type
 */
typedef void (*simcom_thread_func_t)(void *arg);

/**
 * @brief Thread handle
 */
typedef struct {
    sTaskRef task_ref;
    char *stack_ptr;
    uint32_t stack_size;
} simcom_thread_t;

/**
 * @brief Create and start a new thread
 * @param thread Pointer to thread handle
 * @param name Thread name (for debugging)
 * @param stack_size Stack size in bytes
 * @param priority Thread priority (150-250 recommended)
 * @param func Thread function
 * @param arg Argument passed to thread function
 * @return 0 on success, -1 on failure
 */
static inline int simcom_thread_create(simcom_thread_t *thread,
                                       const char *name,
                                       uint32_t stack_size,
                                       int priority,
                                       simcom_thread_func_t func,
                                       void *arg) {
    if (!thread || !func || stack_size < 1024) {
        return -1;
    }

    /* Allocate stack */
    thread->stack_size = stack_size;
    thread->stack_ptr = (char *)malloc(stack_size);
    if (!thread->stack_ptr) {
        return -1;
    }

    /* Create task */
    thread->task_ref = NULL;
    int ret = sAPI_TaskCreate(&thread->task_ref,
                              thread->stack_ptr,
                              stack_size,
                              priority,
                              (char *)name,
                              (void (*)(void *))func,
                              arg);
    if (ret != 0) {
        free(thread->stack_ptr);
        thread->stack_ptr = NULL;
        return -1;
    }

    return 0;
}

/**
 * @brief Delete a thread
 * @param thread Pointer to thread handle
 *
 * NOTE: The thread function must have returned before calling this.
 * Ensure the thread's loop has exited before deletion.
 */
static inline void simcom_thread_delete(simcom_thread_t *thread) {
    if (thread && thread->task_ref) {
        sAPI_TaskDelete(thread->task_ref);
        thread->task_ref = NULL;
    }
    if (thread && thread->stack_ptr) {
        free(thread->stack_ptr);
        thread->stack_ptr = NULL;
    }
}

/*===========================================================================
 * Logging helpers
 *===========================================================================*/

/**
 * @brief Log levels matching Anjay's log levels
 */
typedef enum {
    SIMCOM_LOG_TRACE = 0,
    SIMCOM_LOG_DEBUG = 1,
    SIMCOM_LOG_INFO  = 2,
    SIMCOM_LOG_WARN  = 3,
    SIMCOM_LOG_ERROR = 4
} simcom_log_level_t;

/**
 * @brief Set minimum log level
 * @param level Minimum level to display
 */
void simcom_log_set_level(simcom_log_level_t level);

/**
 * @brief Log a message
 * @param level Log level
 * @param module Module name
 * @param file Source file name
 * @param line Line number
 * @param msg Format string
 * @param ... Format arguments
 */
void simcom_log(simcom_log_level_t level, const char *module,
                const char *file, int line, const char *msg, ...);

/* Log macros */
#define SIMCOM_LOG_TRACE(module, ...) \
    simcom_log(SIMCOM_LOG_TRACE, module, __FILE__, __LINE__, __VA_ARGS__)
#define SIMCOM_LOG_DEBUG(module, ...) \
    simcom_log(SIMCOM_LOG_DEBUG, module, __FILE__, __LINE__, __VA_ARGS__)
#define SIMCOM_LOG_INFO(module, ...) \
    simcom_log(SIMCOM_LOG_INFO, module, __FILE__, __LINE__, __VA_ARGS__)
#define SIMCOM_LOG_WARN(module, ...) \
    simcom_log(SIMCOM_LOG_WARN, module, __FILE__, __LINE__, __VA_ARGS__)
#define SIMCOM_LOG_ERROR(module, ...) \
    simcom_log(SIMCOM_LOG_ERROR, module, __FILE__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* SIMCOM_PLATFORM_H */
