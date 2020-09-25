/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2020 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS products only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/* Firmware Upgrade HomeKit Custom Service
 */
#ifndef _HAP_FW_UPGRADE_H_
#define _HAP_FW_UPGRADE_H_
#include <hap.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Custom UUID for Firmware Upgrade Service */
#define HAP_SERV_CUSTOM_UUID_FW_UPG         "d57034d8-3736-11e8-b467-0ed5f89f718b"

/** Custom UUID for the Write-Only Firmware Upgrade  URL */
#define HAP_CHAR_CUSTOM_UUID_FW_UPG_URL     "d57039e2-3736-11e8-b467-0ed5f89f718b"

/** Custom UUID for the Read-Only Firmware Upgrade Status */
#define HAP_CHAR_CUSTOM_UUID_FW_UPG_STATUS  "d5703b5e-3736-11e8-b467-0ed5f89f718b"

typedef struct {
    char * server_cert_pem; /*!< Server verification, PEM format as string */
} hap_fw_upgrade_config_t;

/** Firmware Upgrade Status
 *
 * These are the valid values for HAP_CHAR_CUSTOM_UUID_FW_UPG_STATUS
 */
typedef enum {
    /** FW Upgrade Failed */
    FW_UPG_STATUS_FAIL = -1,
    /** FW Upgrade Idle */
    FW_UPG_STATUS_IDLE = 0,
    /** FW Upgrade in Progress */
    FW_UPG_STATUS_UPGRADING = 1,
    /** FW Upgrade Successful */
    FW_UPG_STATUS_SUCCESS = 2,
} hap_fw_upgrade_status_t;

/** Create Firmware Upgrade Service
 *
 * This creates the custom Firmware Upgrade HomeKit Service with appropriate characteristics.
 * Add this service to the accessory, to enable the HTTP Client based Firmware Upgrade.
 * Host the FW image binary on a webserver and provide the URL as write value for
 * \ref HAP_CHAR_CUSTOM_UUID_FW_UPG_URL. The status will be reported on
 * \ref HAP_CHAR_CUSTOM_UUID_FW_UPG_STATUS
 *
 * Please refer the top level README.md for more details.
 * ESP32 OTA details: https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/ota.html
 * ESP32 API Reference: https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/esp_https_ota.html
 * @param[in] ota_config Pointer to a \ref hap_fw_upgrade_config_t structure, for using Secure HTTP (HTTPS).
 *
 * @return Service Object pointer on success
 * @return NULL on failure
 */
hap_serv_t * hap_serv_fw_upgrade_create(hap_fw_upgrade_config_t *ota_config);

#ifdef __cplusplus
}
#endif

#endif /* _HAP_FW_UPGRADE_H_ */
