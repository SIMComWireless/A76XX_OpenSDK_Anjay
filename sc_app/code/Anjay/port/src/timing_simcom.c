/**
 * @file timing_simcom.c
 * @brief Custom mbedtls timing implementation for SIMCOM A7672E platform
 *
 * Implements mbedtls_timing_set_delay() and mbedtls_timing_get_delay() using
 * the SIMCOM SDK's sAPI_GetTicks() function. This replaces the standard
 * timing.c which requires Unix/Windows APIs unavailable on ARM bare-metal.
 */

#include "mbedtls/timing.h"
#include "simcom_os.h"

volatile int mbedtls_timing_alarmed = 0;

void mbedtls_timing_set_delay(void *data, uint32_t int_ms, uint32_t fin_ms) {
    mbedtls_timing_delay_context *ctx =
            (mbedtls_timing_delay_context *)data;

    ctx->int_ms = int_ms;
    ctx->fin_ms = fin_ms;

    if (fin_ms != 0) {
        ctx->timer.start_ms = sAPI_GetTicks();
    }
}

int mbedtls_timing_get_delay(void *data) {
    mbedtls_timing_delay_context *ctx =
            (mbedtls_timing_delay_context *)data;

    if (ctx->fin_ms == 0) {
        return -1;
    }

    uint32_t elapsed_ms = sAPI_GetTicks() - ctx->timer.start_ms;

    if (elapsed_ms >= ctx->fin_ms) {
        return 2;
    }

    if (elapsed_ms >= ctx->int_ms) {
        return 1;
    }

    return 0;
}

unsigned long mbedtls_timing_get_timer(struct mbedtls_timing_hr_time *val,
                                       int reset) {
    if (reset) {
        val->start_ms = sAPI_GetTicks();
        return 0;
    }

    return (unsigned long)(sAPI_GetTicks() - val->start_ms);
}

void mbedtls_set_alarm(int seconds) {
    (void)seconds;
    /* Not used by avs_commons mbedtls integration */
}
