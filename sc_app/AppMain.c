/**
  ******************************************************************************
  * @file    AppMain.c
  * @author  SIMCom OpenSDK Team
  * @brief   Source code for SIMCom OpenSDK application, void userSpace_Main(void * arg) is the app entry for OpenSDK application,customer should start application from this call.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 SIMCom Wireless.
  * All rights reserved.
  *
  ******************************************************************************
  */

/*----------------------------------------------------------------------
*  Include files
*----------------------------------------------------------------------*/
#include "include.h"

#include "userspaceConfig.h"
#include "api_map.h"
#include "simcom_os.h"
#include "anjay_main.h"
#include "simcom_platform.h"
#include "simcom_ntp_client.h"

/*----------------------------------------------------------------------
*  Function Definitions
*----------------------------------------------------------------------*/

/**
  * @brief  Wait for SIM card to be ready.
  * @retval 0 on success, -1 on failure
  */
static int wait_for_sim_ready(void)
{
    UINT8 cpin_state = SC_SIM_UNKNOW;
    int retry_count = 0;
    const int max_retries = 30; /* ~30 seconds */

    simcom_uart_log_write("[NET] Checking SIM card status...\r\n");
    while (retry_count < max_retries) {
        SC_simcard_err_e err = sAPI_SimcardPinGet(&cpin_state);
        if (err == SC_SIM_RETURN_SUCCESS && cpin_state == SC_SIM_READY) {
            simcom_uart_log_write("[NET] SIM card ready.\r\n");
            return 0;
        }
        if (cpin_state == SC_SIM_PIN || cpin_state == SC_SIM_PUK) {
            simcom_uart_log_write("[NET] SIM requires PIN/PUK, cannot proceed.\r\n");
            return -1;
        }
        if (cpin_state == SC_SIM_BLOCKED || cpin_state == SC_SIM_REMOVED
                || cpin_state == SC_SIM_CRASH
                || cpin_state == SC_SIM_NOINSRETED) {
            simcom_uart_log_write("[NET] SIM not available (state=%d), retrying...\r\n",
                   cpin_state);
        }
        retry_count++;
        sAPI_TaskSleep(200);
    }

    simcom_uart_log_write("[NET] SIM card timeout.\r\n");
    return -1;
}

/**
  * @brief  Wait for network registration (PS domain).
  * @retval 0 on success, -1 on timeout
  */
static int wait_for_network_reg(void)
{
    SCcpsiParm cpsi;
    int cgreg = 0;
    int retry_count = 0;
    const int max_retries = 120; /* ~2 minutes */

    simcom_uart_log_write("[NET] Waiting for network registration...\r\n");
    while (retry_count < max_retries) {
        /* Check PS registration via CGREG */
        if (sAPI_NetworkGetCgreg(&cgreg) == SC_NET_SUCCESS) {
            if (cgreg == SC_PS_NW_REG_STA_REG_HPLMN
                    || cgreg == SC_PS_NW_REG_STA_REG_ROAMING) {
                simcom_uart_log_write("[NET] Network registered (CGREG=%d).\r\n", cgreg);

                /* Print cell info via CPSI */
                if (sAPI_NetworkGetCpsi(&cpsi) == SC_NET_SUCCESS) {
                    simcom_uart_log_write("[NET] Network mode: %s, MCC-MNC: %s\r\n",
                           cpsi.networkmode, cpsi.Mnc_Mcc);
                    simcom_uart_log_write("[NET] LAC=0x%04X, CellID=0x%04X\r\n",
                           cpsi.LAC, cpsi.CellID);
                }
                return 0;
            }
        }
        retry_count++;
        sAPI_TaskSleep(200);
    }

    simcom_uart_log_write("[NET] Network registration timeout.\r\n");
    return -1;
}

/**
  * @brief  Activate PDP context for TCP/IP.
  * @retval 0 on success, -1 on failure
  */
static int activate_pdp(void)
{
    INT32 ret;

    simcom_uart_log_write("[NET] Activating PDP context...\r\n");
    ret = sAPI_TcpipPdpActive(1, 1); /* cid=1, channel=1 */
    if (ret == SC_TCPIP_SUCCESS) {
        simcom_uart_log_write("[NET] PDP context activated.\r\n");
        return 0;
    }

    simcom_uart_log_write("[NET] PDP activation failed (%ld).\r\n", (long)ret);
    return -1;
}

/**
  * @brief  Sync time via NTP (blocking, ~10s timeout).
  * @retval 0 on success, -1 on failure
  *
  * NOTE: Currently not called — PSK mode doesn't need accurate time.
  * Enable this for certificate-based DTLS.
  */
static int sync_ntp_time(void)
{
    SIM_MSG_T ntp_result = {SC_SRV_NONE, -1, 0, NULL};
    sMsgQRef ntp_msgq = NULL;
    SCsysTime_t currUtcTime;
    UINT32 ret;

    simcom_uart_log_write("[NET] Syncing time via NTP...\r\n");

    /* Create message queue for async NTP result */
    if (sAPI_MsgQCreate(&ntp_msgq, "ntp_msgq", sizeof(SIM_MSG_T), 4, SC_FIFO)
            != SC_SUCCESS) {
        simcom_uart_log_write("[NET] NTP msgQ create failed.\r\n");
        return -1;
    }

    /* Step 1: Set NTP server and timezone (UTC+8 = 32) */
    ret = sAPI_NtpUpdate(SC_NTP_OP_SET, "ntp.aliyun.com", 32, NULL);
    if (ret != SC_NTP_OK) {
        simcom_uart_log_write("[NET] NTP set failed (%d).\r\n", ret);
        sAPI_MsgQDelete(ntp_msgq);
        return -1;
    }

    /* Step 2: Execute NTP sync (async, result via message queue) */
    ret = sAPI_NtpUpdate(SC_NTP_OP_EXC, NULL, 0, ntp_msgq);
    if (ret != SC_NTP_OK) {
        simcom_uart_log_write("[NET] NTP exec failed (%d).\r\n", ret);
        sAPI_MsgQDelete(ntp_msgq);
        return -1;
    }

    /* Step 3: Wait for NTP result */
    while (1) {
        if (sAPI_MsgQRecv(ntp_msgq, &ntp_result, SC_SUSPEND) != SC_SUCCESS) {
            simcom_uart_log_write("[NET] NTP msgQ recv failed.\r\n");
            break;
        }
        if (ntp_result.msg_id != SC_SRV_NTP) {
            continue; /* not our message, keep waiting */
        }
        if (ntp_result.arg1 == SC_NTP_OK) {
            memset(&currUtcTime, 0, sizeof(currUtcTime));
            sAPI_GetSysLocalTime(&currUtcTime);
            simcom_uart_log_write("[NET] NTP synced: %d-%02d-%02d %02d:%02d:%02d\r\n",
                   currUtcTime.tm_year, currUtcTime.tm_mon, currUtcTime.tm_mday,
                   currUtcTime.tm_hour, currUtcTime.tm_min, currUtcTime.tm_sec);
            sAPI_MsgQDelete(ntp_msgq);
            return 0;
        }
        simcom_uart_log_write("[NET] NTP sync failed (code=%d).\r\n", ntp_result.arg1);
        break;
    }

    sAPI_MsgQDelete(ntp_msgq);
    return -1;
}

/**
  * @brief  OpenSDK app entry.
  * @param  Pointer arg
  * @note   This is the app entry,like main(),all functions must start from here!!!!!!
  * @retval void
  */
void userSpace_Main(void *arg)
{
    ApiMapInit(arg);

    /* Initialize UART1 for log output (115200 8N1 + async ring buffer) */
    simcom_uart_log_init();

    /* Initialize network subsystem */
    sAPI_NetworkInit();

    /* Wait for SIM card */
    if (wait_for_sim_ready() != 0) {
        simcom_uart_log_write("[NET] SIM card not available, cannot start.\r\n");
        return;
    }

    /* Wait for network registration */
    if (wait_for_network_reg() != 0) {
        simcom_uart_log_write("[NET] Network registration failed, cannot start.\r\n");
        return;
    }

    /* Activate PDP context */
    if (activate_pdp() != 0) {
        simcom_uart_log_write("[NET] PDP activation failed, cannot start.\r\n");
        return;
    }

    /* Sync time via NTP (needed for certificate-based DTLS) */
    sync_ntp_time();

    simcom_uart_log_write("[NET] Network ready, starting Anjay LwM2M client...\r\n");

    /* Start Anjay LwM2M client in a dedicated task */
    anjay_client_start_task();

}

void abort(void)
{
    simcom_uart_log_write("abort!!!\r\n");
    while (1);
}

#define _appRegTable_attr_ __attribute__((unused, section(".userSpaceRegTable")))

#define appMainStackSize (1024*10)

#ifndef APP_VERSION
#define APP_VERSION "Anjay_SDK_1.0.0"
#endif
userSpaceEntry_t userSpaceEntry _appRegTable_attr_ = {NULL, NULL, appMainStackSize, 30, "userSpaceMain", userSpace_Main, APP_VERSION };

