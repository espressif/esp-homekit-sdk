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
#include <string.h>
#include <esp_log.h>
#include <hap.h>
#include <hap_fw_upgrade.h>
#include <esp_https_ota.h>
#include <esp_idf_version.h>

#define FW_UPG_TASK_PRIORITY    1
#define FW_UPG_STACKSIZE        6 * 1024
#define FW_UPG_TASK_NAME        "hap_fw_upgrade"

static const char *TAG = "HAP FW Upgrade";

static hap_fw_upgrade_status_t fw_upgrade_status = FW_UPG_STATUS_IDLE;
static hap_char_t *fw_upgrade_status_char;

static void remove_escape_char(char *url)
{
    char *target_url = url;
    while (*url) {
        if (*url == '\\') {
            url++;
        }
        *target_url++ = *url++;
    }
    *target_url = '\0';
}

static void fw_upgrade_thread_entry(void *data)
{
    esp_http_client_config_t *client_config = (esp_http_client_config_t *)data;
    remove_escape_char((char *)client_config->url);
    ESP_LOGI(TAG, "Fetching FW image from %s", client_config->url);
    fw_upgrade_status = FW_UPG_STATUS_UPGRADING;
    hap_val_t val = {.i = fw_upgrade_status};
    hap_char_update_val(fw_upgrade_status_char, &val);
    esp_err_t ret;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_https_ota_config_t ota_config = {
        .http_config = client_config,
    };
    ret = esp_https_ota(&ota_config);
#else
    ret = esp_https_ota(client_config);
#endif
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "FW Upgrade Successful");
        fw_upgrade_status = FW_UPG_STATUS_SUCCESS;
    } else {
        ESP_LOGE(TAG, "FW Upgrade Failed");
        fw_upgrade_status = FW_UPG_STATUS_FAIL;
    }
    free((char *)client_config->url);
    val.i = fw_upgrade_status;
    hap_char_update_val(fw_upgrade_status_char, &val);
    if (fw_upgrade_status == FW_UPG_STATUS_SUCCESS) {
        ESP_LOGI(TAG, "Rebooting into new firmware");
        /* Wait for 5 seconds, so that there is enough time for the status
         * to reflect on the controller
         */
        vTaskDelay((5 * 1000) / portTICK_PERIOD_MS);
        /* Restart and boot into the new firmware */
        hap_reboot_accessory();
    }
    /* Updating the value of the variable here, so that the next upgrade attempt (if any) can start.
     * However, no need to update the value, as we want to retain the lates status for 
     * controllers to read.
     */
    fw_upgrade_status = FW_UPG_STATUS_IDLE;
    vTaskDelete(NULL);
}

static int hap_fw_upgrade_write(hap_write_data_t write_data[], int count,
        void *serv_priv, void *write_priv)
{
    int i, ret = HAP_SUCCESS;
    hap_write_data_t *write;
    for (i = 0; i < count; i++) {
        write = &write_data[i];
        if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_CUSTOM_UUID_FW_UPG_URL)) {
            /* If the Upgrade Status is not idle, it means that a FW Upgrade attempt
             * is already in progress. Report appropriate status in such a case and do
             * not proceed
             */
            if (fw_upgrade_status != FW_UPG_STATUS_IDLE) {
                *(write->status) = HAP_STATUS_RES_BUSY;
                ret = HAP_FAIL;
            } else {
                if (!serv_priv) {
                    *(write->status) = HAP_STATUS_OO_RES;
                    ret = HAP_FAIL;
                    continue;
                }
                esp_http_client_config_t *client_config = (esp_http_client_config_t *)serv_priv;
                client_config->url = strndup(write->val.s, strlen(write->val.s)+1);
                if (xTaskCreate(fw_upgrade_thread_entry, FW_UPG_TASK_NAME, FW_UPG_STACKSIZE,
                    client_config, FW_UPG_TASK_PRIORITY, NULL) == pdTRUE) {
                    *(write->status) = HAP_STATUS_SUCCESS;
                } else {
                    *(write->status) = HAP_STATUS_OO_RES;
                    ret = HAP_FAIL;
                }
            }
        } else {
            *(write->status) = HAP_STATUS_RES_ABSENT;
            ret = HAP_FAIL;
        }
    }
    return ret;
}

hap_serv_t * hap_serv_fw_upgrade_create(hap_fw_upgrade_config_t *ota_config)
{
    hap_serv_t *hs = hap_serv_create(HAP_SERV_CUSTOM_UUID_FW_UPG);
    if (!hs) {
        return NULL;
    }
    hap_char_t *hc = hap_char_string_create(HAP_CHAR_CUSTOM_UUID_FW_UPG_URL, HAP_CHAR_PERM_PW, NULL);
    int ret = hap_serv_add_char(hs, hc);
    fw_upgrade_status_char = hap_char_int_create(HAP_CHAR_CUSTOM_UUID_FW_UPG_STATUS, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_EV, 0);
    ret = hap_serv_add_char(hs, fw_upgrade_status_char);
    if (ret != HAP_SUCCESS) {
        hap_serv_delete(hs);
        return NULL;
    }
    hap_char_add_description(hc, "FW Upgrade URL");
    hap_char_add_description(fw_upgrade_status_char, "FW Upgrade Status");
    hap_serv_set_write_cb(hs, hap_fw_upgrade_write);
    esp_http_client_config_t *client_config = calloc(1, sizeof(esp_http_client_config_t));
    if (!client_config) {
        hap_serv_delete(hs);
        return NULL;
    }
    if(ota_config->server_cert_pem) {
        client_config->cert_pem = strdup(ota_config->server_cert_pem);
        if(!client_config->cert_pem) {
            free(client_config);
            hap_serv_delete(hs);
            return NULL;
        }
    } else {
        ESP_LOGW(TAG, "Server certificate not provided in OTA config.");
    }
    hap_serv_set_priv(hs, client_config);

    /* Mark the service as hidden as it need not be controlled directly by users */
    hap_serv_mark_hidden(hs);
    return hs;
}


