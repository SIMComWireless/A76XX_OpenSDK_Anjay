/**
 * @file platform_impl.c
 * @brief Platform implementation for Anjay on SIMCOM A7672E
 *
 * Provides logging, timing, threading, and utility functions
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "simcom_platform.h"
#include "simcom_debug.h"
#include "simcom_uart.h"
#include "simcom_ntp_client.h"
#include "anjay_simcom_config.h"

#include <avsystem/commons/avs_time.h>
#include <avsystem/commons/avs_mutex.h>
#include <avsystem/commons/avs_condvar.h>
#include <avsystem/commons/avs_init_once.h>
#include <avsystem/commons/avs_log.h>

/*===========================================================================
 * UART log ring buffer + writer task
 *
 * sAPI_UartWrite is blocking. To avoid stalling callers when the log
 * volume is high, log data is pushed into a ring buffer and a dedicated
 * task drains it to SC_UART asynchronously.
 *===========================================================================*/

#define LOG_RING_SIZE   2048
#define LOG_RING_MASK   (LOG_RING_SIZE - 1)

static uint8_t  *g_log_ring = NULL;          /* dynamically allocated */
static volatile uint32_t g_log_head = 0;     /* producer writes here */
static volatile uint32_t g_log_tail = 0;     /* consumer reads from here */

static sTaskRef  g_uart_log_task = NULL;
static sMutexRef g_log_ring_mutex = NULL;
static sFlagRef  g_log_data_flag = NULL;

#define UART_LOG_TASK_STACK_SIZE  2048
static uint8_t *g_uart_log_task_stack = NULL; /* dynamically allocated */

static uint32_t ring_free(void) {
    return LOG_RING_SIZE - (g_log_head - g_log_tail);
}

static void ring_push(const uint8_t *data, uint32_t len) {
    if (!g_log_ring || !g_log_ring_mutex) return; /* not initialized yet */
    sAPI_MutexLock(g_log_ring_mutex, SC_SUSPEND);
    uint32_t free_space = ring_free();
    if (len > free_space) {
        len = free_space;   /* drop oldest data on overflow */
    }
    uint32_t pos = g_log_head & LOG_RING_MASK;
    uint32_t first = LOG_RING_SIZE - pos;
    if (first > len) first = len;
    memcpy(&g_log_ring[pos], data, first);
    if (len > first) {
        memcpy(&g_log_ring[0], data + first, len - first);
    }
    g_log_head += len;
    sAPI_MutexUnLock(g_log_ring_mutex);
    sAPI_FlagSet(g_log_data_flag, 1, SC_FLAG_OR);
}

static uint32_t ring_pop(uint8_t *dst, uint32_t max_len) {
    uint32_t avail = g_log_head - g_log_tail;
    if (avail == 0) return 0;
    if (avail > max_len) avail = max_len;
    uint32_t pos = g_log_tail & LOG_RING_MASK;
    uint32_t first = LOG_RING_SIZE - pos;
    if (first > avail) first = avail;
    memcpy(dst, &g_log_ring[pos], first);
    if (avail > first) {
        memcpy(dst + first, &g_log_ring[0], avail - first);
    }
    g_log_tail += avail;
    return avail;
}

static void uart_log_writer_task(void *arg) {
    (void)arg;
    uint8_t buf[256];
    UINT32 flag;
    while (1) {
        /* Wait for data to appear in the ring */
        sAPI_FlagWait(g_log_data_flag, 1, SC_FLAG_OR_CLEAR,
                      &flag, SC_SUSPEND);
        uint32_t n;
        while ((n = ring_pop(buf, sizeof(buf))) > 0) {
            sAPI_UartWrite(SC_UART, buf, (UINT32)n);
        }
    }
}

/**
 * @brief Initialize SC_UART (115200 8N1) and start the async log writer task.
 */
void simcom_uart_log_init(void) {
    if (g_log_ring) return; /* already initialized */

    /* Configure UART1 */
    SCuartConfiguration uartConfig;
    uartConfig.BaudRate  = SC_UART_BAUD_115200;
    uartConfig.DataBits  = SC_UART_WORD_LEN_8;
    uartConfig.ParityBit = SC_UART_NO_PARITY_BITS;
    uartConfig.StopBits  = SC_UART_ONE_STOP_BIT;
    if (sAPI_UartSetConfig(SC_UART, &uartConfig) == SC_UART_RETURN_CODE_ERROR) {
        sAPI_Debug("[LOG] UART init failed!");
    }

    /* Allocate ring buffer on heap to save .bss */
    g_log_ring = (uint8_t *)malloc(LOG_RING_SIZE);
    if (!g_log_ring) {
        sAPI_Debug("[LOG] ring alloc failed!");
        return;
    }
    memset(g_log_ring, 0, LOG_RING_SIZE);

    /* Create ring buffer sync primitives */
    sAPI_MutexCreate(&g_log_ring_mutex, SC_FIFO);
    sAPI_FlagCreate(&g_log_data_flag);

    /* Allocate task stack on heap */
    g_uart_log_task_stack = (uint8_t *)malloc(UART_LOG_TASK_STACK_SIZE);
    if (!g_uart_log_task_stack) {
        sAPI_Debug("[LOG] task stack alloc failed!");
        return;
    }

    /* Start the writer task */
    sAPI_TaskCreate(&g_uart_log_task,
                    g_uart_log_task_stack,
                    UART_LOG_TASK_STACK_SIZE,
                    201,   /* high priority so logs drain promptly */
                    "uart_log",
                    uart_log_writer_task,
                    NULL);
}

/**
 * @brief Non-blocking log write — pushes into ring buffer.
 *        The writer task will drain it to SC_UART.
 */
void simcom_uart_log_write(const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0) {
        ring_push((const uint8_t *)buf, (uint32_t)len);
    }
}

/*===========================================================================
 * Global log level
 *===========================================================================*/
static simcom_log_level_t g_log_level = SIMCOM_LOG_INFO;

/*===========================================================================
 * Log level strings
 *===========================================================================*/
static const char *log_level_str[] = {
    "TRACE",
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR"
};

/*===========================================================================
 * Logging implementation
 *===========================================================================*/

/**
 * @brief Get timestamp string from the SIMCOM local system time.
 * @param buf   Output buffer (at least 32 bytes)
 * @param size  Buffer size
 */
static void get_timestamp_str(char *buf, size_t size) {
    SCsysTime_t currUtcTime;
    sAPI_GetSysLocalTime(&currUtcTime);

    /* Keep a millisecond suffix from the tick counter, while using the
     * SIMCOM system-local-time API for the wall-clock fields. */
    uint32_t ms = sAPI_GetTicks() % 1000;
    snprintf(buf, size, "%02d/%02d/%02d,%02d:%02d:%02d.%03u",
             currUtcTime.tm_year % 100,
             currUtcTime.tm_mon + 1,
             currUtcTime.tm_mday,
             currUtcTime.tm_hour,
             currUtcTime.tm_min,
             currUtcTime.tm_sec,
             ms);
}

void simcom_log_set_level(simcom_log_level_t level) {
    g_log_level = level;
}

void simcom_log(simcom_log_level_t level, const char *module,
                const char *file, int line, const char *msg, ...) {
    if (level < g_log_level || level < SIMCOM_LOG_TRACE
            || level > SIMCOM_LOG_ERROR) {
        return;
    }

    char buf[256];
    va_list args;
    va_start(args, msg);
    vsnprintf(buf, sizeof(buf), msg, args);
    va_end(args);

    /* Extract just the filename from the path */
    const char *filename = file;
    const char *p = file;
    while (*p) {
        if (*p == '/' || *p == '\\') {
            filename = p + 1;
        }
        p++;
    }

    char time_buf[32];
    get_timestamp_str(time_buf, sizeof(time_buf));

    simcom_uart_log_write("[%s] [%s] %s (%s:%d): %s\r\n",
                   time_buf, log_level_str[level], module, filename, line, buf);
}

/*===========================================================================
 * avs_log handler — routes Anjay internal logs to UART via ring buffer
 *===========================================================================*/

static const char *avs_log_level_str(avs_log_level_t level) {
    switch (level) {
    case AVS_LOG_TRACE:   return "TRACE";
    case AVS_LOG_DEBUG:   return "DEBUG";
    case AVS_LOG_INFO:    return "INFO ";
    case AVS_LOG_WARNING: return "WARN ";
    case AVS_LOG_ERROR:   return "ERROR";
    default:              return "?????";
    }
}

static void simcom_avs_log_handler(avs_log_level_t level,
                                   const char *module,
                                   const char *message) {
    char time_buf[32];
    get_timestamp_str(time_buf, sizeof(time_buf));

    simcom_uart_log_write("[%s] [%s] %s: %s\r\n",
                   time_buf,
                   avs_log_level_str(level),
                   module ? module : "?",
                   message ? message : "(null)");
}

void simcom_platform_log_init(void) {
    avs_log_set_handler(simcom_avs_log_handler);
    avs_log_set_default_level(AVS_LOG_DEBUG);
}

/*===========================================================================
 * Hardware entropy source for mbedtls (MBEDTLS_ENTROPY_HARDWARE_ALT)
 *
 * Uses timer jitter from sAPI_GetTicks() to gather entropy.
 * This is sufficient for DTLS PSK handshakes on embedded platforms
 * without a dedicated hardware RNG.
 *===========================================================================*/

int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len,
                          size_t *olen)
{
    (void)data;
    uint32_t seed = 0;
    size_t i;

    for (i = 0; i < len; i++) {
        uint32_t accumulator = 0;
        int j;

        /* Collect 8 samples of timer jitter per output byte */
        for (j = 0; j < 8; j++) {
            uint32_t t1 = sAPI_GetTicks();
            volatile int k;
            for (k = 0; k < (j * 3 + 1); k++) {
                /* busy wait for variable delay */
            }
            uint32_t t2 = sAPI_GetTicks();
            accumulator ^= (t2 - t1) << (j * 4);
        }

        /* Mix with running seed */
        seed = seed * 6364136223846793005ULL + accumulator;
        output[i] = (unsigned char)(seed ^ (seed >> 8)
                                    ^ (seed >> 16) ^ (seed >> 24));
    }

    *olen = len;
    return 0;
}

/*===========================================================================
 * Time implementation for Anjay (avs_time)
 *
 * avs_time_real_now()    — wall-clock time via sAPI_Gettimeofday()
 * avs_time_monotonic_now() — monotonic ticks via sAPI_GetTicks()
 *===========================================================================*/

avs_time_real_t avs_time_real_now(void) {
    sTimeval tv;
    avs_time_real_t result;

    if (sAPI_Gettimeofday(&tv, NULL) == 0) {
        result.since_real_epoch.seconds = (int64_t)tv.tv_sec;
        result.since_real_epoch.nanoseconds = (int32_t)(tv.tv_usec * 1000);
    } else {
        /* Fallback: use ticks as seconds (not accurate for real time) */
        result.since_real_epoch.seconds = (int64_t)((uint64_t)sAPI_GetTicks() * portTICK_PERIOD_MS / 1000);
        result.since_real_epoch.nanoseconds = 0;
    }

    return result;
}

avs_time_monotonic_t avs_time_monotonic_now(void) {
    uint32_t ticks = sAPI_GetTicks();
    /* Use 64-bit to avoid overflow after ~49 days with 1ms tick */
    uint64_t ms = (uint64_t)ticks * portTICK_PERIOD_MS;
    avs_time_monotonic_t result;

    result.since_monotonic_epoch.seconds = (int64_t)(ms / 1000);
    result.since_monotonic_epoch.nanoseconds = (int32_t)((ms % 1000) * 1000000);

    return result;
}

/*===========================================================================
 * Mutex implementation using SIMCOM sAPI_Mutex*
 *===========================================================================*/

struct avs_mutex {
    sMutexRef handle;
};

int avs_mutex_create(avs_mutex_t **out_mutex) {
    if (!out_mutex) {
        return -1;
    }

    avs_mutex_t *mutex = (avs_mutex_t *)calloc(1, sizeof(avs_mutex_t));
    if (!mutex) {
        return -1;
    }

    if (sAPI_MutexCreate(&mutex->handle, SC_FIFO) != SC_SUCCESS) {
        free(mutex);
        return -1;
    }

    *out_mutex = mutex;
    return 0;
}

int avs_mutex_lock(avs_mutex_t *mutex) {
    if (!mutex || !mutex->handle) {
        return -1;
    }
    return (sAPI_MutexLock(mutex->handle, SC_SUSPEND) == SC_SUCCESS) ? 0 : -1;
}

int avs_mutex_try_lock(avs_mutex_t *mutex) {
    if (!mutex || !mutex->handle) {
        return -1;
    }
    SC_STATUS status = sAPI_MutexLock(mutex->handle, SC_NO_SUSPEND);
    if (status == SC_SUCCESS) {
        return 0;  /* locked */
    }
    return 1;  /* already locked */
}

int avs_mutex_unlock(avs_mutex_t *mutex) {
    if (!mutex || !mutex->handle) {
        return -1;
    }
    return (sAPI_MutexUnLock(mutex->handle) == SC_SUCCESS) ? 0 : -1;
}

void avs_mutex_cleanup(avs_mutex_t **mutex) {
    if (mutex && *mutex) {
        if ((*mutex)->handle) {
            sAPI_MutexDelete((*mutex)->handle);
        }
        free(*mutex);
        *mutex = NULL;
    }
}

/*===========================================================================
 * Condition variable implementation using SIMCOM sAPI_Semaphore*
 *
 * FreeRTOS doesn't have native condvars. We simulate with a binary
 * semaphore: wait() acquires the semaphore (blocks), notify_all()
 * releases it (wakes waiter).
 *
 * LIMITATION: Binary semaphore means notify_all() only wakes ONE waiter.
 * This is acceptable for Anjay's typical single-waiter usage in avs_sched.
 * For true multi-waiter support, a counting semaphore + waiter counter
 * would be needed.
 *===========================================================================*/

struct avs_condvar {
    sSemaRef sem;
};

int avs_condvar_create(avs_condvar_t **out_condvar) {
    if (!out_condvar) {
        return -1;
    }

    avs_condvar_t *cv = (avs_condvar_t *)calloc(1, sizeof(avs_condvar_t));
    if (!cv) {
        return -1;
    }

    /* Binary semaphore: initial count = 0 (blocks on acquire) */
    if (sAPI_SemaphoreCreate(&cv->sem, SC_FIFO, 0) != SC_SUCCESS) {
        free(cv);
        return -1;
    }

    *out_condvar = cv;
    return 0;
}

int avs_condvar_notify_all(avs_condvar_t *condvar) {
    if (!condvar || !condvar->sem) {
        return -1;
    }
    return (sAPI_SemaphoreRelease(condvar->sem) == SC_SUCCESS) ? 0 : -1;
}

int avs_condvar_wait(avs_condvar_t *condvar,
                     avs_mutex_t *mutex,
                     avs_time_monotonic_t deadline) {
    if (!condvar || !condvar->sem || !mutex) {
        return -1;
    }

    /* Unlock mutex before waiting */
    avs_mutex_unlock(mutex);

    UINT32 timeout;
    if (!avs_time_monotonic_valid(deadline)) {
        timeout = SC_SUSPEND;  /* wait forever */
    } else {
        avs_time_duration_t remaining = avs_time_monotonic_diff(
                deadline, avs_time_monotonic_now());
        int64_t remaining_ms = 0;
        if (avs_time_duration_to_scalar(&remaining_ms, AVS_TIME_MS, remaining)
                || remaining_ms < 0) {
            timeout = SC_NO_SUSPEND;
        } else if (remaining_ms > (int64_t)0xFFFFFFFF / portTICK_PERIOD_MS) {
            timeout = SC_SUSPEND;
        } else {
            timeout = (UINT32)(remaining_ms / portTICK_PERIOD_MS);
        }
    }

    int result;
    SC_STATUS status = sAPI_SemaphoreAcquire(condvar->sem, timeout);
    if (status == SC_SUCCESS) {
        result = 0;  /* event occurred */
    } else {
        result = 1;  /* timeout */
    }

    /* Re-lock mutex after waiting */
    avs_mutex_lock(mutex);
    return result;
}

void avs_condvar_cleanup(avs_condvar_t **condvar) {
    if (condvar && *condvar) {
        if ((*condvar)->sem) {
            sAPI_SemaphoreDelete((*condvar)->sem);
        }
        free(*condvar);
        *condvar = NULL;
    }
}

/*===========================================================================
 * Init-once implementation
 *
 * Uses a static mutex to ensure func() is never called in parallel and
 * only once across all threads (required by avs_init_once contract).
 *===========================================================================*/

struct avs_init_once_handle {
    int done;
};

static sMutexRef g_init_once_mutex = NULL;

int avs_init_once(volatile avs_init_once_handle_t *handle,
                  avs_init_once_func_t *func,
                  void *func_arg) {
    if (!handle || !func) {
        return -1;
    }

    /* Fast path: already initialized */
    if (*handle && (*handle)->done) {
        return 0;
    }

    /* Lazy-init the global mutex (safe: called during early init) */
    if (!g_init_once_mutex) {
        if (sAPI_MutexCreate(&g_init_once_mutex, SC_FIFO) != SC_SUCCESS) {
            return -1;
        }
    }

    sAPI_MutexLock(g_init_once_mutex, SC_SUSPEND);

    /* Double-check under lock */
    if (*handle && (*handle)->done) {
        sAPI_MutexUnLock(g_init_once_mutex);
        return 0;
    }

    if (!*handle) {
        struct avs_init_once_handle *h =
                (struct avs_init_once_handle *)calloc(1, sizeof(*h));
        if (!h) {
            sAPI_MutexUnLock(g_init_once_mutex);
            return -1;
        }
        *handle = h;
    }

    int ret = func(func_arg);
    if (ret == 0) {
        (*handle)->done = 1;
    }

    sAPI_MutexUnLock(g_init_once_mutex);
    return ret;
}
