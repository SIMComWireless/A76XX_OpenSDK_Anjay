/*
 * Connectivity Statistics Object (/7) for SIMCOM A7672E
 *
 * LwM2M Connectivity Statistics Object implementation.
 * Uses sAPI_GetStatisticsData() to report TX/RX byte counts.
 */

#include "conn_stats_object.h"

#include <anjay/anjay.h>
#include <anjay/dm.h>
#include <anjay/io.h>
#include <avsystem/commons/avs_defs.h>
#include <avsystem/commons/avs_list.h>
#include <avsystem/commons/avs_memory.h>

#include <string.h>

#include "simcom_network.h"
#include "simcom_platform.h"

#ifndef UINT16_MAX
#define UINT16_MAX 0xFFFF
#endif

/* Resource IDs (standard /7) */
#define RID_TX_DATA             2
#define RID_RX_DATA             3
#define RID_START_RESET         6

/* Resource IDs (vendor extension for packet statistics) */
#define RID_TX_PACKETS          10
#define RID_RX_PACKETS          11
#define RID_TX_DROPPED          12
#define RID_RX_DROPPED          13

typedef struct {
    const anjay_dm_object_def_t *def_ptr;
    anjay_t *anjay;

    /* Snapshot at reset time (baseline for delta calculation) */
    SCstatisticsData baseline;
} conn_stats_object_t;

static conn_stats_object_t g_conn_stats;

/**
 * Get current statistics from SIMCOM SDK.
 * Returns 0 on success, -1 on error.
 */
static int get_statistics(SCstatisticsData *out)
{
    memset(out, 0, sizeof(*out));
    int ret = sAPI_GetStatisticsData(out);
    simcom_uart_log_write("[CONN_STATS] sAPI_GetStatisticsData ret=%d\r\n", ret);
    simcom_uart_log_write("[CONN_STATS] Rx_Bytes=%llu Tx_Bytes=%llu\r\n",
                          out->Rx_Bytes, out->Tx_Bytes);
    simcom_uart_log_write("[CONN_STATS] TX_Packets=%lu RX_Packets=%lu\r\n",
                          out->TX_Packets, out->RX_Packets);
    simcom_uart_log_write("[CONN_STATS] Tx_Dropped=%lu Rx_Dropped=%lu\r\n",
                          out->Tx_Dropped_Packets, out->Rx_Dropped_Packets);
    return ret == 0 ? 0 : -1;
}

/* --- Anjay DM handlers --- */

static int conn_stats_list_instances(anjay_t *anjay,
                                     const anjay_dm_object_def_t *const *def_ptr,
                                     anjay_dm_list_ctx_t *ctx)
{
    (void) anjay;
    (void) def_ptr;
    anjay_dm_emit(ctx, 0);
    return 0;
}

static int conn_stats_list_resources(anjay_t *anjay,
                                     const anjay_dm_object_def_t *const *obj_ptr,
                                     anjay_iid_t iid,
                                     anjay_dm_resource_list_ctx_t *ctx)
{
    (void) anjay;
    (void) obj_ptr;
    (void) iid;

    anjay_dm_emit_res(ctx, RID_TX_DATA,
                      ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_RX_DATA,
                      ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_TX_PACKETS,
                      ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_RX_PACKETS,
                      ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_TX_DROPPED,
                      ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_RX_DROPPED,
                      ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_START_RESET,
                      ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT);
    return 0;
}

static int conn_stats_resource_read(anjay_t *anjay,
                                    const anjay_dm_object_def_t *const *obj_ptr,
                                    anjay_iid_t iid,
                                    anjay_rid_t rid,
                                    anjay_riid_t riid,
                                    anjay_output_ctx_t *ctx)
{
    (void) anjay;
    (void) obj_ptr;
    (void) iid;

    SCstatisticsData stats;

    switch (rid) {
    case RID_TX_DATA:
        if (get_statistics(&stats) != 0) return ANJAY_ERR_INTERNAL;
        return anjay_ret_i64(ctx,
            (int64_t)(stats.Tx_Bytes - g_conn_stats.baseline.Tx_Bytes));
    case RID_RX_DATA:
        if (get_statistics(&stats) != 0) return ANJAY_ERR_INTERNAL;
        return anjay_ret_i64(ctx,
            (int64_t)(stats.Rx_Bytes - g_conn_stats.baseline.Rx_Bytes));
    case RID_TX_PACKETS:
        if (get_statistics(&stats) != 0) return ANJAY_ERR_INTERNAL;
        return anjay_ret_i64(ctx,
            (int64_t)(stats.TX_Packets - g_conn_stats.baseline.TX_Packets));
    case RID_RX_PACKETS:
        if (get_statistics(&stats) != 0) return ANJAY_ERR_INTERNAL;
        return anjay_ret_i64(ctx,
            (int64_t)(stats.RX_Packets - g_conn_stats.baseline.RX_Packets));
    case RID_TX_DROPPED:
        if (get_statistics(&stats) != 0) return ANJAY_ERR_INTERNAL;
        return anjay_ret_i64(ctx,
            (int64_t)(stats.Tx_Dropped_Packets - g_conn_stats.baseline.Tx_Dropped_Packets));
    case RID_RX_DROPPED:
        if (get_statistics(&stats) != 0) return ANJAY_ERR_INTERNAL;
        return anjay_ret_i64(ctx,
            (int64_t)(stats.Rx_Dropped_Packets - g_conn_stats.baseline.Rx_Dropped_Packets));
    default:
        return ANJAY_ERR_METHOD_NOT_ALLOWED;
    }
}

static int conn_stats_resource_execute(anjay_t *anjay,
                                       const anjay_dm_object_def_t *const *obj_ptr,
                                       anjay_iid_t iid,
                                       anjay_rid_t rid,
                                       anjay_execute_ctx_t *arg_ctx)
{
    (void) anjay;
    (void) obj_ptr;
    (void) iid;
    (void) arg_ctx;

    if (rid == RID_START_RESET) {
        /* Reset counters: snapshot current values as new baseline */
        SCstatisticsData stats;
        if (get_statistics(&stats) != 0) {
            return ANJAY_ERR_INTERNAL;
        }
        g_conn_stats.baseline = stats;
        simcom_uart_log_write("[CONN_STATS] Counters reset\r\n");
        return 0;
    }
    return ANJAY_ERR_METHOD_NOT_ALLOWED;
}

static const anjay_dm_object_def_t CONN_STATS_OBJ_DEF = {
    .oid = 7,
    .handlers = {
        .list_instances = conn_stats_list_instances,
        .list_resources = conn_stats_list_resources,
        .resource_read = conn_stats_resource_read,
        .resource_execute = conn_stats_resource_execute,
        .transaction_begin = anjay_dm_transaction_NOOP,
        .transaction_validate = anjay_dm_transaction_NOOP,
        .transaction_commit = anjay_dm_transaction_NOOP,
        .transaction_rollback = anjay_dm_transaction_NOOP,
    }
};

const anjay_dm_object_def_t **conn_stats_object_def_ptr(void)
{
    if (!g_conn_stats.def_ptr) {
        g_conn_stats.def_ptr = &CONN_STATS_OBJ_DEF;

        /* Snapshot initial counters as baseline */
        SCstatisticsData stats;
        if (get_statistics(&stats) == 0) {
            g_conn_stats.baseline = stats;
        }
    }
    return &g_conn_stats.def_ptr;
}

void conn_stats_object_cleanup(void)
{
    memset(&g_conn_stats, 0, sizeof(g_conn_stats));
}
