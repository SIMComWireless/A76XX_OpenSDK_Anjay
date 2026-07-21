/**
 * @file platform_impl.c
 * @brief Platform implementation for Anjay on SIMCOM A7672E
 *
 * Provides logging and utility functions
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "simcom_platform.h"
#include "anjay_simcom_config.h"

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
void simcom_log_set_level(simcom_log_level_t level) {
    g_log_level = level;
}

void simcom_log(simcom_log_level_t level, const char *module,
                const char *file, int line, const char *msg, ...) {
    if (level < g_log_level) {
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

    printf("[%s] %s (%s:%d): %s\r\n",
           log_level_str[level], module, filename, line, buf);
}
