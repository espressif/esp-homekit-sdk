/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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

/* HomeKit Emulator Example
*/

#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_wifi.h>

#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include <hap_fw_upgrade.h>

#include <hap_bct_http_handlers.h>

#include <iot_button.h>

#include <app_wifi.h>
#include <app_hap_setup_payload.h>

#include "emulator.h"

static const char *TAG = "HAP Emulator";

/*  Required for server verification during OTA, PEM format as string  */
char server_cert[] = {};

#define HAP_ACC_TASK_PRIORITY  1
#define HAP_ACC_TASK_STACKSIZE 4 * 1024
#define HAP_ACC_TASK_NAME      "hap_emulator"

/* Reset network credentials if button is pressed for more than 3 seconds and then released */
#define RESET_NETWORK_BUTTON_TIMEOUT        3

/* Reset to factory if button is pressed and held for more than 10 seconds */
#define RESET_TO_FACTORY_BUTTON_TIMEOUT     10

/* The button "Boot" will be used as the Reset button for the example */
#define RESET_GPIO  GPIO_NUM_0
/**
 * @brief The network reset button callback handler.
 * Useful for testing the Wi-Fi re-configuration feature of WAC2
 */
static void reset_network_handler(void* arg)
{
    hap_reset_network();
}
/**
 * @brief The factory reset button callback handler.
 */
static void reset_to_factory_handler(void* arg)
{
    hap_reset_to_factory();
}

/**
 * The Reset button  GPIO initialisation function.
 * Same button will be used for resetting Wi-Fi network as well as for reset to factory based on
 * the time for which the button is pressed.
 */
static void reset_key_init(uint32_t key_gpio_pin)
{
    button_handle_t handle = iot_button_create(key_gpio_pin, BUTTON_ACTIVE_LOW);
    iot_button_add_on_release_cb(handle, RESET_NETWORK_BUTTON_TIMEOUT, reset_network_handler, NULL);
    iot_button_add_on_press_cb(handle, RESET_TO_FACTORY_BUTTON_TIMEOUT, reset_to_factory_handler, NULL);
}

/*
 * An optional HomeKit Event handler which can be used to track HomeKit
 * specific events.
 */
void emulator_hap_event_handler(hap_event_t event, void *data)
{
    switch(event) {
        case HAP_EVENT_PAIRING_STARTED :
            ESP_LOGI(TAG, "Pairing Started");
            break;
        case HAP_EVENT_PAIRING_ABORTED :
            ESP_LOGI(TAG, "Pairing Aborted");
            break;
        case HAP_EVENT_CTRL_PAIRED :
            ESP_LOGI(TAG, "Controller %s Paired. Controller count: %d",
                        (char *)data, hap_get_paired_controller_count());
            break;
        case HAP_EVENT_CTRL_UNPAIRED :
            ESP_LOGI(TAG, "Controller %s Removed. Controller count: %d",
                        (char *)data, hap_get_paired_controller_count());
            break;
        case HAP_EVENT_CTRL_CONNECTED :
            ESP_LOGI(TAG, "Controller %s Connected", (char *)data);
            break;
        case HAP_EVENT_CTRL_DISCONNECTED :
            ESP_LOGI(TAG, "Controller %s Disconnected", (char *)data);
            break;
        default:
            ESP_LOGW(TAG, "Unknown HomeKit event: %d", event);
            break;
    }
}

static void hap_emulator_system_event_handler(void* arg, esp_event_base_t event_base, int event_id, void* event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("Got IP:" IPSTR "\n", IP2STR(&event->ip_info.ip));
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("Failed to connect to network/disconnected.\n");
    }
}

static hap_emulator_profile_t *profile;
/* Mandatory identify routine for the accessory.
 * In a real accessory, something like LED blink should be implemented
 * got visual identification
 */
static int emulator_identify(hap_acc_t *ha)
{
    ESP_LOGI(TAG, "%s identified", profile->name);
    printf("%s identified\n", profile->name);
    return HAP_SUCCESS;
}


static void emulator_thread_entry(void *p)
{
    hap_acc_t *accessory;
    hap_serv_t *service;
    hap_set_debug_level(HAP_DEBUG_LEVEL_ERR);
    esp_log_level_set("wifi", 0);
    
    /* Configure HomeKit core to make the Accessory name (and thus the WAC SSID) unique,
     * instead of the default configuration wherein only the WAC SSID is made unique.
     */
    hap_cfg_t hap_cfg;
    hap_get_config(&hap_cfg);
    hap_cfg.unique_param = UNIQUE_NAME;
    hap_set_config(&hap_cfg);

    /* Initialize the MFi core */
    hap_init(HAP_TRANSPORT_WIFI);
    /* Retrieve the profile before reboot */
    profile = emulator_get_profile();

    /* Initialise the mandatory parameters for Accessory which will be added as
     * the mandatory services internally
     */
    hap_acc_cfg_t cfg = {
        .name = profile->name,
        .manufacturer = "Espressif",
        .model = "MD-01",
        .serial_num = "001122334455",
        .fw_rev = "0.9.0",
        .hw_rev = NULL,
        .pv = "1.1.0",
        .identify_routine = emulator_identify,
        .cid = profile->cid,
    };
    /* Create accessory object */
    accessory = hap_acc_create(&cfg);

    /* Add a dummy Product Data */
    uint8_t product_data[] = {'E','S','P','3','2','H','A','P'};
    hap_acc_add_product_data(accessory, product_data, sizeof(product_data));

    /* Create the appropriate Service. Include the "name" since this is a user visible service  */
    service = profile->serv_create();
    hap_serv_add_char(service, hap_char_name_create(profile->name));

    /* Set the write callback for the service */
    hap_serv_set_write_cb(service, emulator_write);

    /* Add the Service to the Accessory Object */
    hap_acc_add_serv(accessory, service);

    /* Create the Firmware Upgrade HomeKit Custom Service.
     * Please refer the FW Upgrade documentation under components/homekit/extras/include/hap_fw_upgrade.h
     * and the top level README for more information.
     */
#ifndef CONFIG_IDF_TARGET_ESP8266
    hap_fw_upgrade_config_t ota_config = {
        .server_cert_pem = server_cert,
    };
    service = hap_serv_fw_upgrade_create(&ota_config);

    /* Add the service to the Accessory Object */
    hap_acc_add_serv(accessory, service);
#endif

    /* Add the Accessory to the HomeKit Database */
    hap_add_accessory(accessory);

    ESP_LOGI(TAG, "Starting %s", profile->name);

    printf("\n---------------\n");
    char setup_code[11] = {0};
    size_t setup_code_size = sizeof(setup_code);
    if (hap_factory_keystore_get("hap_setup", "setup_code", (uint8_t *)setup_code, &setup_code_size) == HAP_SUCCESS) {
        printf("Setup Code : %s\n", setup_code);
    }

    int method;
    size_t size = sizeof(method);
    /* Methods -> 1: Hardware Authentication, 2: Software Authentication */
    if(hap_factory_keystore_get("auth", "method", (uint8_t *)&method, &size) == HAP_SUCCESS) {
        if (method == 1) {
            hap_enable_mfi_auth(HAP_MFI_AUTH_HW);
            printf("H/W Authentication\n");
        } else if (method == 2) {
            hap_enable_mfi_auth(HAP_MFI_AUTH_SW);
            printf("S/W Authentication\n");
        }
    }

    /* Register a common button for reset Wi-Fi network and reset to factory.
     * The Wi-Fi reset will especially be useful to test the Network Re-configuration
     * feature of WAC2. If the network configuration of a paired accessory is reset, as per
     * WAC2, the iOS can reconfigure it automatically.
     */
    reset_key_init(RESET_GPIO);
    
    /* Fetches the brightness, hue, saturation values before power cycle characteristics from NVS */
    emulator_get_characteristics();

    /* Register an event handler for HomeKit specific events */
    hap_register_event_handler(emulator_hap_event_handler);

    /* Query the controller count (just for information) */
    ESP_LOGI(TAG, "Accessory is paired with %d controllers",
                hap_get_paired_controller_count());

    /* For production accessories, the setup code shouldn't be programmed on to
     * the device. Instead, the setup info, derived from the setup code must
     * be used. Use the factory_nvs_gen utility to generate this data and then
     * flash it into the factory NVS partition.
     *
     * By default, the setup ID and setup info will be read from the factory_nvs
     * Flash partition and so, is not required to set here explicitly.
     *
     * However, for testing purpose, this can be overridden by using hap_set_setup_code()
     * and hap_set_setup_id() APIs, as has been done here.
     */
#ifdef CONFIG_EXAMPLE_USE_HARDCODED_SETUP_CODE
    /* Unique Setup code of the format xxx-xx-xxx. Default: 111-22-333 */
    hap_set_setup_code(CONFIG_EXAMPLE_SETUP_CODE);
    /* Unique four character Setup Id. Default: ES32 */
    hap_set_setup_id(CONFIG_EXAMPLE_SETUP_ID);
#ifdef CONFIG_APP_WIFI_USE_WAC_PROVISIONING
    app_hap_setup_payload(CONFIG_EXAMPLE_SETUP_CODE, CONFIG_EXAMPLE_SETUP_ID, true, cfg.cid);
#else
    app_hap_setup_payload(CONFIG_EXAMPLE_SETUP_CODE, CONFIG_EXAMPLE_SETUP_ID, false, cfg.cid);
#endif
#endif

    /* Initialize Wi-Fi */
    app_wifi_init();

    /* Register system event handler */
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &hap_emulator_system_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &hap_emulator_system_event_handler, NULL);

    /* After all the initializations are done, start the HAP core */
    if (hap_start() != ESP_OK) {
        printf("Failed to start HAP\n");
    }
    
    /* Start Wi-Fi */
    app_wifi_start(0);

    /* state of HAP_CHAR_UUID_ON - Turned ON or OFF before reboot */
    if(!emulator_get_state()) {
        hap_val_t val;
        val.b = true;
        hap_serv_t *hs = hap_acc_get_first_serv(hap_get_first_acc());
        while(hs && !hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_ON))
            hs = hap_serv_get_next(hs);
        if(hs)
            hap_char_update_val(hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_ON), &val);    
    }
    
    /* Registering BCT handlers as this is a test accesory */
    hap_bct_register_http_handlers();

    emulator_show_profiles();
    printf("Accessory has booted up as a %s.\n", profile->name);
    printf("Enter \'help\' for options.\n");

    /* Initializes the console input sequence */ 
    console_init();

    /* The task ends here. The read/write callbacks will be invoked by the HAP Framework */
    vTaskDelete(NULL);
}

void app_main()
{
    xTaskCreate(emulator_thread_entry, HAP_ACC_TASK_NAME, HAP_ACC_TASK_STACKSIZE, NULL, HAP_ACC_TASK_PRIORITY, NULL);
}

