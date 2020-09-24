/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
#include <esp_err.h>
#include <hap.h>

/** Show the HAP Setup payload
 *
 * This shows the setup paylod in a QR code
 *
 * @param[in] setup_code NULL terminated setup code. Eg. "111-22-333"
 * @param[in] setup_id NULL terminated setup id. Eg. "ES32"
 * @param[in] wac_support Boolean indicating if WAC provisioning is supported.
 * @param[in] cid Accessory category identifier.
 *
 * @return ESP_OK on success.
 * @return ESP_FAIL on failure.
 */
esp_err_t app_hap_setup_payload(char *setup_code, char *setup_id, bool wac_support, hap_cid_t cid);
