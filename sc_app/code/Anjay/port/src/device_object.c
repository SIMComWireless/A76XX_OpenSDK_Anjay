/*
 * Device Object (/3) for SIMCOM A7672E
 *
 * LwM2M Device Object implementation using SIMCOM SDK APIs.
 * Provides device manufacturer, model, serial number, firmware version, etc.
 */

#include "device_object.h"

#include <anjay/anjay.h>
#include <anjay/dm.h>
#include <anjay/io.h>
#include <avsystem/commons/avs_defs.h>
#include <avsystem/commons/avs_list.h>
#include <avsystem/commons/avs_memory.h>

#include <string.h>

#include "simcom_platform.h"
#include "simcom_system.h"
#include "simcom_os.h"
#include "sc_power.h"

#ifndef UINT16_MAX
#define UINT16_MAX 0xFFFF
#endif

typedef struct {
    const anjay_dm_object_def_t *def_ptr;
    anjay_t *anjay;

    /* Cached device info (read once at init) */
    char manufacturer[64];
    char model_number[64];
    char serial_number[32]; /* IMEI */
    char firmware_version[64];
    char hardware_version[64];
    char software_version[64];
} device_object_t;

static device_object_t g_dev;

/* Resource IDs */
#define RID_MANUFACTURER        0
#define RID_MODEL_NUMBER        1
#define RID_SERIAL_NUMBER       2
#define RID_FIRMWARE_VERSION    3
#define RID_REBOOT              4
#define RID_DEVICE_TYPE         17
#define RID_HARDWARE_VERSION    18
#define RID_SOFTWARE_VERSION    19
#define RID_SUPPORTED_BINDING   16

static void device_info_init(void)
{
    SIMComVersion ver;
    RFVersion rf_ver;
    memset(&ver, 0, sizeof(ver));
    memset(&rf_ver, 0, sizeof(rf_ver));

    /* Get SIMCOM version info */
    sAPI_SysGetVersion(&ver);

    if (ver.manufacture_namestr) {
        strncpy(g_dev.manufacturer, ver.manufacture_namestr,
                sizeof(g_dev.manufacturer) - 1);
    } else {
        strcpy(g_dev.manufacturer, "SIMCom");
    }

    if (ver.Module_modelstr) {
        strncpy(g_dev.model_number, ver.Module_modelstr,
                sizeof(g_dev.model_number) - 1);
    } else {
        strcpy(g_dev.model_number, "A7672E");
    }

    if (ver.Revision) {
        strncpy(g_dev.firmware_version, ver.Revision,
                sizeof(g_dev.firmware_version) - 1);
    } else {
        strcpy(g_dev.firmware_version, "unknown");
    }

    if (ver.SDK_Version) {
        strncpy(g_dev.software_version, ver.SDK_Version,
                sizeof(g_dev.software_version) - 1);
    } else {
        strcpy(g_dev.software_version, "unknown");
    }

    /* Get IMEI as serial number */
    if (sAPI_SysGetImei(g_dev.serial_number) != 0) {
        strcpy(g_dev.serial_number, "unknown");
    }

    /* Get RF version as hardware version */
    sAPI_SysGetRFVersion(&rf_ver);
    if (rf_ver.rfversion1[0] != '\0') {
        strncpy(g_dev.hardware_version, rf_ver.rfversion1,
                sizeof(g_dev.hardware_version) - 1);
    } else {
        strcpy(g_dev.hardware_version, "unknown");
    }
}

/* --- Anjay DM handlers --- */

static int device_list_resources(anjay_t *anjay,
                                 const anjay_dm_object_def_t *const *obj_ptr,
                                 anjay_iid_t iid,
                                 anjay_dm_resource_list_ctx_t *ctx)
{
    (void) anjay;
    (void) obj_ptr;
    (void) iid;

    anjay_dm_emit_res(ctx, RID_MANUFACTURER,
                      ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_MODEL_NUMBER,
                      ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_SERIAL_NUMBER,
                      ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_FIRMWARE_VERSION,
                      ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_SUPPORTED_BINDING,
                      ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_REBOOT,
                      ANJAY_DM_RES_E, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_DEVICE_TYPE,
                      ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_HARDWARE_VERSION,
                      ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_SOFTWARE_VERSION,
                      ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    return 0;
}

static int device_resource_read(anjay_t *anjay,
                                const anjay_dm_object_def_t *const *obj_ptr,
                                anjay_iid_t iid,
                                anjay_rid_t rid,
                                anjay_riid_t riid,
                                anjay_output_ctx_t *ctx)
{
    (void) anjay;
    (void) obj_ptr;
    (void) iid;

    switch (rid) {
    case RID_MANUFACTURER:
        return anjay_ret_string(ctx, g_dev.manufacturer);
    case RID_MODEL_NUMBER:
        return anjay_ret_string(ctx, g_dev.model_number);
    case RID_SERIAL_NUMBER:
        return anjay_ret_string(ctx, g_dev.serial_number);
    case RID_FIRMWARE_VERSION:
        return anjay_ret_string(ctx, g_dev.firmware_version);
    case RID_SUPPORTED_BINDING:
        return anjay_ret_string(ctx, "U");
    case RID_DEVICE_TYPE:
        return anjay_ret_string(ctx, "A7672E");
    case RID_HARDWARE_VERSION:
        return anjay_ret_string(ctx, g_dev.hardware_version);
    case RID_SOFTWARE_VERSION:
        return anjay_ret_string(ctx, g_dev.software_version);
    default:
        return ANJAY_ERR_METHOD_NOT_ALLOWED;
    }
}

static int device_resource_execute(anjay_t *anjay,
                                   const anjay_dm_object_def_t *const *obj_ptr,
                                   anjay_iid_t iid,
                                   anjay_rid_t rid,
                                   anjay_execute_ctx_t *arg_ctx)
{
    (void) anjay;
    (void) obj_ptr;
    (void) iid;
    (void) arg_ctx;

    if (rid == RID_REBOOT) {
        simcom_uart_log_write("[DEV] Reboot requested by server\r\n");
        /* Give UART time to flush before reboot */
        sAPI_TaskSleep(200);
        sAPI_SysReset();
        return 0;
    }
    return ANJAY_ERR_METHOD_NOT_ALLOWED;
}

static int device_list_instances(anjay_t *anjay,
                                 const anjay_dm_object_def_t *const *def_ptr,
                                 anjay_dm_list_ctx_t *ctx)
{
    (void) anjay;
    (void) def_ptr;
    anjay_dm_emit(ctx, 0);
    return 0;
}

static const anjay_dm_object_def_t DEVICE_OBJ_DEF = {
    .oid = 3,
    .handlers = {
        .list_instances = device_list_instances,
        .list_resources = device_list_resources,
        .resource_read = device_resource_read,
        .resource_execute = device_resource_execute,
        .transaction_begin = anjay_dm_transaction_NOOP,
        .transaction_validate = anjay_dm_transaction_NOOP,
        .transaction_commit = anjay_dm_transaction_NOOP,
        .transaction_rollback = anjay_dm_transaction_NOOP,
    }
};

const anjay_dm_object_def_t **device_object_def_ptr(void)
{
    if (!g_dev.def_ptr) {
        g_dev.def_ptr = &DEVICE_OBJ_DEF;
        device_info_init();
    }
    return &g_dev.def_ptr;
}

void device_object_cleanup(void)
{
    memset(&g_dev, 0, sizeof(g_dev));
}
