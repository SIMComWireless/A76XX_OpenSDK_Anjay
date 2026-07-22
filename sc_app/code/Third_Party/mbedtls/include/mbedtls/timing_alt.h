/**
 * @file timing_alt.h
 * @brief Alternative timing structures for MBEDTLS_TIMING_ALT on SIMCOM platform
 *
 * Used instead of the default opaque struct when MBEDTLS_TIMING_ALT is defined.
 * The SIMCOM platform doesn't support the standard timing.c (requires
 * Unix/Windows APIs), so we provide a lightweight implementation using
 * sAPI_GetTicks().
 */

#ifndef TIMING_ALT_H
#define TIMING_ALT_H

#include <stdint.h>

struct mbedtls_timing_hr_time {
    uint32_t start_ms;  /* Starting timestamp in milliseconds */
};

typedef struct mbedtls_timing_delay_context {
    struct mbedtls_timing_hr_time timer;
    uint32_t int_ms;    /* Intermediate delay threshold */
    uint32_t fin_ms;    /* Final delay threshold */
} mbedtls_timing_delay_context;

#endif /* TIMING_ALT_H */
