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
/* HomeKit Sensor Example with custom data/tlv8 characteristics
*/
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>

#include <iot_button.h>

#include <app_wifi.h>
#include <app_hap_setup_payload.h>

static const char *TAG = "HAP Sensor";

#define SENSOR_TASK_PRIORITY  1
#define SENSOR_TASK_STACKSIZE 4 * 1024
#define SENSOR_TASK_NAME      "hap_sensor"

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


/* Mandatory identify routine for the accessory.
 * In a real accessory, something like LED blink should be implemented
 * got visual identification
 */
static int sensor_identify(hap_acc_t *ha)
{
    ESP_LOGI(TAG, "Sensor identified");
    return HAP_SUCCESS;
}

/* A dummy callback for handling a read on the "Temperature" characteristic of Sensor.
 * In an actual accessory, this should read from the hardware
 */
static int sensor_read(hap_char_t *hc, hap_status_t *status_code, void *serv_priv, void *read_priv)
{
    if (!strcmp(hap_char_get_type_uuid(hc), HAP_CHAR_UUID_CURRENT_TEMPERATURE)) {
        ESP_LOGI(TAG, "Received Read for Current Temperature");
        const hap_val_t *cur_val = hap_char_get_val(hc);
        ESP_LOGI(TAG, "Current Value: %f", cur_val->f);
        /* We are setting a fummy value here, which is 1 more than the current value.
         * If the value overflows max limit of 100, we reset it to 0
         * TODO: Read from actual hardware
         */
        hap_val_t new_val;
        new_val.f = cur_val->f + 1.0;
        if (new_val.f > 100) {
            new_val.f = 0;
        }
        ESP_LOGI(TAG, "Updated Value: %f", new_val.f);
        hap_char_update_val(hc, &new_val);
        *status_code = HAP_STATUS_SUCCESS;
        return HAP_SUCCESS;
    } else {
        *status_code = HAP_STATUS_RES_ABSENT;
    }
    return HAP_FAIL;
}

/** Buffers to hold data/tlv8 values. The HomeKit core does not maintain a copy of
 * the values. It will just keep the pointers.
 */
uint8_t mydata[128];
uint8_t mytlv8[128];

/** Custom UUIDs for the custom service and characteristics */
#define APP_CUSTOM_SERV_UUID        "8517ab60-73bd-11e8-adc0-fa7ae01bbebc"
#define APP_CUSTOM_CHAR_DATA_UUID   "8517ade0-73bd-11e8-adc0-fa7ae01bbebc"
#define APP_CUSTOM_CHAR_TLV8_UUID   "8517af8e-73bd-11e8-adc0-fa7ae01bbebc"

static void hex_dbg_print(char *name, unsigned char *buf, int buf_len)
{
	int i;
	printf("%s: ", name);
	for (i = 0; i < buf_len; i++) {
		if (i % 16 == 0)
			printf("\r\n");
		printf("%02x ", buf[i]);
	}
	printf("\r\n");
}

/* Read routine for the custom data/tlv8 characteristics */
static int custom_serv_read(hap_char_t *hc, hap_status_t *status, void *serv_priv, void *read_priv)
{
    int ret = HAP_SUCCESS;
    if (!strcmp(hap_char_get_type_uuid(hc), APP_CUSTOM_CHAR_DATA_UUID)) {
        const hap_val_t *cur_val = hap_char_get_val(hc);
        hap_data_val_t data;
        /* Changing the first byte of existing data, just to demonstrate a value change on read */
        mydata[0] = 0xaa;
        data.buf = mydata;
        data.buflen = cur_val->d.buflen;
        hap_val_t new_val = {
            .d = data,
        };
        hex_dbg_print("Read data", data.buf, data.buflen);
        hap_char_update_val(hc, &new_val);
        *status = HAP_STATUS_SUCCESS;
    } else if (!strcmp(hap_char_get_type_uuid(hc), APP_CUSTOM_CHAR_TLV8_UUID)) {
        const hap_val_t *cur_val = hap_char_get_val(hc);
        hap_tlv8_val_t tlv8;
        /* Changing the last byte (if applicable) of existing tlv8, just to demonstrate a value change on read */
        if (cur_val->t.buflen) {
            mytlv8[cur_val->t.buflen - 1] = 0xcc;
        }
        tlv8.buf = mytlv8;
        tlv8.buflen = cur_val->t.buflen;
        hap_val_t new_val = {
            .t = tlv8,
        };
        hex_dbg_print("Read tlv8", tlv8.buf, tlv8.buflen);
        hap_char_update_val(hc, &new_val);
        *status = HAP_STATUS_SUCCESS;
    } else {
        *status = HAP_STATUS_RES_ABSENT;
        ret = HAP_FAIL;
    }
    return ret;
}

/* Write routine for the custom data/tlv8 characteristics */
static int custom_serv_write(hap_write_data_t write_data[], int count,
        void *serv_priv, void *write_priv)
{
    int i, ret = HAP_SUCCESS;
    hap_write_data_t *write;
    for (i = 0; i < count; i++) {
        write = &write_data[i];
        if (!strcmp(hap_char_get_type_uuid(write->hc), APP_CUSTOM_CHAR_DATA_UUID)) {
            hex_dbg_print("Write data", write->val.d.buf, write->val.d.buflen);
            /* Copy the received value to mydata and update */
            memcpy(mydata, write->val.d.buf, write->val.d.buflen);
            hap_val_t val;
            val.d.buf = mydata;
            val.d.buflen = write->val.d.buflen;
            hap_char_update_val(write->hc, &val);
        } else if (!strcmp(hap_char_get_type_uuid(write->hc), APP_CUSTOM_CHAR_TLV8_UUID)) {
            hex_dbg_print("Write tlv8", write->val.t.buf, write->val.t.buflen);
            /* Copy the received value to mytlv8 and update */
            memcpy(mytlv8, write->val.t.buf, write->val.t.buflen); 
            hap_val_t val;
            val.t.buf = mytlv8;
            val.t.buflen = write->val.t.buflen;
            hap_char_update_val(write->hc, &val);
        } else {
            *(write->status) = HAP_STATUS_RES_ABSENT;
            ret = HAP_FAIL;
        }
    }
    return ret;
}
/* Create a custom service with data/tlv8 characteristics */
hap_serv_t *custom_data_tlv8_serv_create()
{
    hap_serv_t *hs = hap_serv_create(APP_CUSTOM_SERV_UUID);
    /* Initialising the characteristic with some dummy data */
    uint8_t tmp[] = {0x0a, 0x0b, 0x0c, 0x0d};
    hap_data_val_t d;
    memcpy(mydata, tmp, sizeof(tmp));
    d.buf = mydata;
    d.buflen = sizeof(tmp);
    hap_char_t *data = hap_char_data_create(APP_CUSTOM_CHAR_DATA_UUID, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_PW, &d);
    hap_serv_add_char(hs, data);

    /* Initialising characteristic with NULL TLV8 */
    hap_char_t *tlv8 = hap_char_tlv8_create(APP_CUSTOM_CHAR_TLV8_UUID, HAP_CHAR_PERM_PR | HAP_CHAR_PERM_PW, NULL);
    hap_serv_add_char(hs, tlv8);

    hap_serv_set_read_cb(hs, custom_serv_read);
    hap_serv_set_write_cb(hs, custom_serv_write);
    return hs;
}

/*The main thread for handling the Sensor Accessory */
static void sensor_thread_entry(void *p)
{
    hap_acc_t *accessory;
    hap_serv_t *service;

    /* Initialize the HAP core */
    hap_init(HAP_TRANSPORT_WIFI);

    /* Initialise the mandatory parameters for Accessory which will be added as
     * the mandatory services internally
     */
    hap_acc_cfg_t cfg = {
        .name = "Esp-Temperature-Sensor",
        .manufacturer = "Espressif",
        .model = "EspSensor-T01",
        .serial_num = "001122334455",
        .fw_rev = "0.9.0",
        .hw_rev = NULL,
        .pv = "1.1.0",
        .identify_routine = sensor_identify,
        .cid = HAP_CID_SENSOR,
    };
    /* Create accessory object */
    accessory = hap_acc_create(&cfg);

    /* Add a dummy Product Data */
    uint8_t product_data[] = {'E','S','P','3','2','H','A','P'};
    hap_acc_add_product_data(accessory, product_data, sizeof(product_data));

    /* Create the Sensor Service. Include the "name" since this is a user visible service  */
    service = hap_serv_temperature_sensor_create(10);
    hap_serv_add_char(service, hap_char_name_create("My Temperature Sensor"));

    /* Set the read callback for the service */
    hap_serv_set_read_cb(service, sensor_read);

    /* Add the Sensor Service to the Accessory Object */
    hap_acc_add_serv(accessory, service);

    service = custom_data_tlv8_serv_create();
    hap_acc_add_serv(accessory, service);

    /* Add the Accessory to the HomeKit Database */
    hap_add_accessory(accessory);

    /* Use the setup_payload_gen tool to get the QR code for Accessory Setup.
     * The payload below is for a Sensor with setup code 111-22-333 and setup id ES32
     */
    ESP_LOGI(TAG, "Use setup payload: \"X-HM://00AHJA6JHES32\" for Accessory Setup");

    /* Register a common button for reset Wi-Fi network and reset to factory.
     */
    reset_key_init(RESET_GPIO);

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

    /* Enable Hardware MFi authentication (applicable only for MFi variant of SDK) */
    hap_enable_mfi_auth(HAP_MFI_AUTH_HW);

    /* Initialize Wi-Fi */
    app_wifi_init();

    /* After all the initializations are done, start the HAP core */
    hap_start();
    /* Start Wi-Fi */
    app_wifi_start(portMAX_DELAY);
    /* The task ends here. The read/write callbacks will be invoked by the HAP Framework */
    vTaskDelete(NULL);

}

void app_main()
{
    /* Create the application thread */
    xTaskCreate(sensor_thread_entry, SENSOR_TASK_NAME, SENSOR_TASK_STACKSIZE,
                NULL, SENSOR_TASK_PRIORITY, NULL);
}
