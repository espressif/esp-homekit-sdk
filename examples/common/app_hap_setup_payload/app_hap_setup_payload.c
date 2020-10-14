/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <esp_err.h>
#include <esp_log.h>
#include <hap.h>
#include <qrcode.h>

static const char *TAG = "app_hap_setup_payload";

#define QRCODE_BASE_URL     "https://espressif.github.io/esp-homekit-sdk/qrcode.html"

esp_err_t app_hap_setup_payload(char *setup_code, char *setup_id, bool wac_support, hap_cid_t cid)
{
    char *setup_payload =  esp_hap_get_setup_payload(setup_code, setup_id, wac_support, cid);
    if (setup_payload) {
        ESP_LOGI(TAG, "-----QR Code for HomeKit-----");
        ESP_LOGI(TAG, "Scan this QR code from the Home app on iOS");
        qrcode_display(setup_payload);
        ESP_LOGI(TAG, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s", QRCODE_BASE_URL, setup_payload);
        free(setup_payload);
        return ESP_OK;
    }
    return ESP_FAIL;
}
