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

/* HomeKit led strip Example
*/

#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include <hap_fw_upgrade.h>

#include <iot_button.h>

#include <app_wifi.h>
#include <app_hap_setup_payload.h>

#include <driver/rmt.h>
#include "neopixel.c"


/* Comment out the below line to disable Firmware Upgrades */
#define CONFIG_FIRMWARE_SERVICE

static const char *TAG = "HAP led_strip";

#define	NEOPIXEL_PORT	18
#define	NR_LED		7
//#define	NR_LED		3
//#define	NEOPIXEL_WS2812
#define	NEOPIXEL_SK6812
#define	NEOPIXEL_RMT_CHANNEL    RMT_CHANNEL_2

#define LED_STRIP_TASK_PRIORITY  1
#define LED_STRIP_TASK_STACKSIZE 4 * 1024
#define LED_STRIP_BRIDGE_TASK_NAME      "hap_led_strip"
#define LED_STRIP_LEDS_TASK_NAME        "leds"

#define NUM_BRIDGED_ACCESSORIES 3

#define NUM_LED_SEGMENTS NUM_BRIDGED_ACCESSORIES

/* Reset network credentials if button is pressed for more than 3 seconds and then released */
#define RESET_NETWORK_BUTTON_TIMEOUT        3

/* Reset to factory if button is pressed and held for more than 10 seconds */
#define RESET_TO_FACTORY_BUTTON_TIMEOUT     10



/* The button "Boot" will be used as the Reset button for the example */
#define RESET_GPIO  GPIO_NUM_0


#define DEG_TO_RAD(X) (M_PI*(X)/180)

int segment_center[] = {0,3,6};

uint32_t segment_hue[NUM_LED_SEGMENTS];
uint32_t segment_saturation[NUM_LED_SEGMENTS];
uint32_t segment_intensity[NUM_LED_SEGMENTS];
bool segment_on[NUM_LED_SEGMENTS];
uint32_t segment_intensity_off[NUM_LED_SEGMENTS];

bool leds_changed = false;


#include "led_strip.h"


pixel_settings_t px;
	uint32_t		pixels[NR_LED];
	int		i;
	int		rc;




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

void hsi2rgbw(float H, float S, float I, int rgbw[]) {
  int r, g, b, w;
  float cos_h, cos_1047_h;
//   H = fmod(H,360); // cycle H around to 0-360 degrees
  H = 3.14159*H/(float)180; // Convert to radians.
  S = S / 100;
  I = I / 100;
  S = S>0?(S<1?S:1):0; // clamp S and I to interval [0,1]
  I = I>0?(I<1?I:1):0;
  
  if(H < 2.09439) {
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667-H);
    r = S*255*I/3*(1+cos_h/cos_1047_h);
    g = S*255*I/3*(1+(1-cos_h/cos_1047_h));
    b = 0;
    w = 255*(1-S)*I;
  } else if(H < 4.188787) {
    H = H - 2.09439;
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667-H);
    g = S*255*I/3*(1+cos_h/cos_1047_h);
    b = S*255*I/3*(1+(1-cos_h/cos_1047_h));
    r = 0;
    w = 255*(1-S)*I;
  } else {
    H = H - 4.188787;
    cos_h = cos(H);
    cos_1047_h = cos(1.047196667-H);
    b = S*255*I/3*(1+cos_h/cos_1047_h);
    r = S*255*I/3*(1+(1-cos_h/cos_1047_h));
    g = 0;
    w = 255*(1-S)*I;
  }
  
  rgbw[0]=r;
  rgbw[1]=g;
  rgbw[2]=b;
  rgbw[3]=w;

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

/* Mandatory identify routine for the accessory (bridge)
 * In a real accessory, something like LED blink should be implemented
 * got visual identification
 */
static int bridge_identify(hap_acc_t *ha)
{
    ESP_LOGI(TAG, "Bridge identified");
    return HAP_SUCCESS;
}

/* Mandatory identify routine for the bridged accessory
 * In a real bridge, the actual accessory must be sent some request to
 * identify itself visually
 */
static int accessory_identify(hap_acc_t *ha)
{
    hap_serv_t *hs = hap_acc_get_serv_by_uuid(ha, HAP_SERV_UUID_LIGHTBULB);
    hap_char_t *hc = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_NAME);
    const hap_val_t *val = hap_char_get_val(hc);
    char *name = val->s;

    ESP_LOGI(TAG, "Bridged Accessory %s identified", name);
    return HAP_SUCCESS;
}

// /* Callback for handling writes on the Light Bulb Service
//  */
// static int led_strip_write(hap_write_data_t write_data[], int count,
//         void *serv_priv, void *write_priv)
// {
//     int i, ret = HAP_SUCCESS;
//     hap_write_data_t *write;
//     for (i = 0; i < count; i++) {
//         write = &write_data[i];
//         /* Setting a default error value */
//         *(write->status) = HAP_STATUS_VAL_INVALID;
//         if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ON)) {
//             ESP_LOGI(TAG, "Received Write for Light %s", write->val.b ? "On" : "Off");
//             if (led_strip_set_on(write->val.b) == 0) {
//                 *(write->status) = HAP_STATUS_SUCCESS;
//             }
//         } else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_BRIGHTNESS)) {
//             ESP_LOGI(TAG, "Received Write for Light Brightness %d", write->val.i);
//             if (led_strip_set_brightness(write->val.i) == 0) {
//                 *(write->status) = HAP_STATUS_SUCCESS;
//             }
//         } else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_HUE)) {
//             ESP_LOGI(TAG, "Received Write for Light Hue %f", write->val.f);
//             if (led_strip_set_hue(write->val.f) == 0) {
//                 *(write->status) = HAP_STATUS_SUCCESS;
//             }
//         } else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_SATURATION)) {
//             ESP_LOGI(TAG, "Received Write for Light Saturation %f", write->val.f);
//             if (led_strip_set_saturation(write->val.f) == 0) {
//                 *(write->status) = HAP_STATUS_SUCCESS;
//             }
//         } else {
//             *(write->status) = HAP_STATUS_RES_ABSENT;
//         }
//         /* If the characteristic write was successful, update it in hap core
//          */
//         if (*(write->status) == HAP_STATUS_SUCCESS) {
//             hap_char_update_val(write->hc, &(write->val));
//         } else {
//             /* Else, set the return value appropriately to report error */
//             ret = HAP_FAIL;
//         }
//     }
//     return ret;
// }

// /* Callback for handling writes on the Light Bulb Service
//  */
// static int led_strip_write1(hap_write_data_t write_data[], int count,
//         void *serv_priv, void *write_priv)
// {
//     int i, ret = HAP_SUCCESS;
//     hap_write_data_t *write;
//     for (i = 0; i < count; i++) {
//         write = &write_data[i];
//         /* Setting a default error value */
//         *(write->status) = HAP_STATUS_VAL_INVALID;
//         if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ON)) {
//             ESP_LOGI(TAG, "Received Write for Light %s", write->val.b ? "On" : "Off");
//             if (led_strip_set_on(write->val.b) == 0) {
//                 *(write->status) = HAP_STATUS_SUCCESS;
//             }
//         } else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_BRIGHTNESS)) {
//             ESP_LOGI(TAG, "Received Write for Light Brightness %d", write->val.i);
//             if (led_strip_set_brightness(write->val.i) == 0) {
//                 *(write->status) = HAP_STATUS_SUCCESS;
//             }
//         } else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_HUE)) {
//             ESP_LOGI(TAG, "Received Write for Light Hue %f", write->val.f);
//             if (led_strip_set_hue(write->val.f) == 0) {
//                 *(write->status) = HAP_STATUS_SUCCESS;
//             }
//         } else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_SATURATION)) {
//             ESP_LOGI(TAG, "Received Write for Light Saturation %f", write->val.f);
//             if (led_strip_set_saturation(write->val.f) == 0) {
//                 *(write->status) = HAP_STATUS_SUCCESS;
//             }
//         } else {
//             *(write->status) = HAP_STATUS_RES_ABSENT;
//         }
//         /* If the characteristic write was successful, update it in hap core
//          */
//         if (*(write->status) == HAP_STATUS_SUCCESS) {
//             hap_char_update_val(write->hc, &(write->val));
//         } else {
//             /* Else, set the return value appropriately to report error */
//             ret = HAP_FAIL;
//         }
//     }
//     return ret;
// }

/*The main thread for handling the Light Bulb Accessory */
// static void led_strip_thread_entry(void *arg)
// {
//     hap_acc_t *accessory;
//     hap_serv_t *service;

//     /* Initialise the mandatory parameters for Accessory which will be added as
//      * the mandatory services internally
//      */
//     // char num_c[5];
//     // sprintf(num_c, "%d", num);
//     // char name[20] = "Led-Strip-";
//     // strcat(name, num_c);
//     hap_acc_cfg_t cfg = {
//         .name = "Led-Strip0",
//         .manufacturer = "Espressif",
//         .model = "EspLight01",
//         .serial_num = "0",
//         .fw_rev = "0.9.0",
//         .hw_rev = "1.0",
//         .pv = "1.1.0",
//         .identify_routine = light_identify,
//         .cid = HAP_CID_LIGHTING,
//     };

//     /* Create accessory object */
//     accessory = hap_acc_create(&cfg);
//     if (!accessory) {
//         ESP_LOGE(TAG, "Failed to create accessory");
//         goto light_err;
//     }

//     /* Add a dummy Product Data */
//     uint8_t product_data[] = {'E','S','P','3','2','H','A','P'};
//     hap_acc_add_product_data(accessory, product_data, sizeof(product_data));

//     /* Create the Light Bulb Service. Include the "name" since this is a user visible service  */
//     service = hap_serv_lightbulb_create(true);
//     if (!service) {
//         ESP_LOGE(TAG, "Failed to create lightbulb Service");
//         goto light_err;
//     }

//     /* Add the optional characteristic to the Light Bulb Service */
//     int ret = hap_serv_add_char(service, hap_char_name_create("My Light"));
//     ret |= hap_serv_add_char(service, hap_char_brightness_create(50));
//     ret |= hap_serv_add_char(service, hap_char_hue_create(180));
//     ret |= hap_serv_add_char(service, hap_char_saturation_create(100));
    
//     if (ret != HAP_SUCCESS) {
//         ESP_LOGE(TAG, "Failed to add optional characteristics to led_strip");
//         goto light_err;
//     }
//     /* Set the write callback for the service */
//     hap_serv_set_write_cb(service, led_strip_write1);
    
//     /* Add the Light Bulb Service to the Accessory Object */
//     hap_acc_add_serv(accessory, service);

// #ifdef CONFIG_FIRMWARE_SERVICE
//     /*  Required for server verification during OTA, PEM format as string  */
//     static char server_cert[] = {};
//     hap_fw_upgrade_config_t ota_config = {
//         .server_cert_pem = server_cert,
//     };
//     /* Create and add the Firmware Upgrade Service, if enabled.
//      * Please refer the FW Upgrade documentation under components/homekit/extras/include/hap_fw_upgrade.h
//      * and the top level README for more information.
//      */
//     service = hap_serv_fw_upgrade_create(&ota_config);
//     if (!service) {
//         ESP_LOGE(TAG, "Failed to create Firmware Upgrade Service");
//         goto light_err;
//     }
//     hap_acc_add_serv(accessory, service);
// #endif

//     /* Add the Accessory to the HomeKit Database */
//     hap_add_accessory(accessory);

//     /* Register a common button for reset Wi-Fi network and reset to factory.
//      */
//     // reset_key_init(RESET_GPIO);

//     /* TODO: Do the actual hardware initialization here */

//     /* For production accessories, the setup code shouldn't be programmed on to
//      * the device. Instead, the setup info, derived from the setup code must
//      * be used. Use the factory_nvs_gen utility to generate this data and then
//      * flash it into the factory NVS partition.
//      *
//      * By default, the setup ID and setup info will be read from the factory_nvs
//      * Flash partition and so, is not required to set here explicitly.
//      *
//      * However, for testing purpose, this can be overridden by using hap_set_setup_code()
//      * and hap_set_setup_id() APIs, as has been done here.
//      */
// #ifdef CONFIG_EXAMPLE_USE_HARDCODED_SETUP_CODE
//     /* Unique Setup code of the format xxx-xx-xxx. Default: 111-22-333 */
//     hap_set_setup_code(CONFIG_EXAMPLE_SETUP_CODE);
//     /* Unique four character Setup Id. Default: ES32 */
//     hap_set_setup_id(CONFIG_EXAMPLE_SETUP_ID);
// #ifdef CONFIG_APP_WIFI_USE_WAC_PROVISIONING
//     app_hap_setup_payload(CONFIG_EXAMPLE_SETUP_CODE, CONFIG_EXAMPLE_SETUP_ID, true, cfg.cid);
// #else
//     app_hap_setup_payload(CONFIG_EXAMPLE_SETUP_CODE, CONFIG_EXAMPLE_SETUP_ID, false, cfg.cid);
// #endif
// #endif

//     /* Enable Hardware MFi authentication (applicable only for MFi variant of SDK) */
//     hap_enable_mfi_auth(HAP_MFI_AUTH_HW);

//     // /* Initialize Wi-Fi */
//     // app_wifi_init();

//     /* After all the initializations are done, start the HAP core */
//     hap_start();
//     // /* Start Wi-Fi */
//     // app_wifi_start(portMAX_DELAY);

//     /* The task ends here. The read/write callbacks will be invoked by the HAP Framework */
//     vTaskDelete(NULL);

// light_err:
//     hap_acc_delete(accessory);
//     vTaskDelete(NULL);
// }


/* callback for handling a write on the "On" characteristic of Led Strip.
 */
static int led_strip_write(hap_write_data_t write_data[], int count,
        void *serv_priv, void *write_priv)
{
    char* name = (char *)serv_priv;
    ESP_LOGI(TAG, "Write called for Accessory %s", name);
    int i, ret = HAP_SUCCESS;
    size_t len = strlen(name);
    int num = (int) name[len - 1] - '0';
    ESP_LOGI(TAG, "Write called for Accessory %d", num);
    hap_write_data_t *write;
    for (i = 0; i < count; i++) {
        write = &write_data[i];
        /* Setting a default error value */
        *(write->status) = HAP_STATUS_VAL_INVALID;
        if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ON)) {
            ESP_LOGI(TAG, "Received Write for Light %s", write->val.b ? "On" : "Off");
            if (led_strip_set_on(num, write->val.b, &segment_on[num], &leds_changed, &segment_intensity[num], &segment_intensity_off[num]) == 0) {
                *(write->status) = HAP_STATUS_SUCCESS;
            }
        } else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_BRIGHTNESS)) {
            ESP_LOGI(TAG, "Received Write for Light Brightness %d", write->val.i);
            if (led_strip_set_brightness(num, write->val.i, &segment_on[num], &leds_changed, &segment_intensity[num]) == 0) {
                *(write->status) = HAP_STATUS_SUCCESS;
            }
        } else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_HUE)) {
            ESP_LOGI(TAG, "Received Write for Light Hue %f", write->val.f);
            if (led_strip_set_hue(num, write->val.f, &segment_on[num], &leds_changed, &segment_hue[num]) == 0) {
                *(write->status) = HAP_STATUS_SUCCESS;
            }
        } else if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_SATURATION)) {
            ESP_LOGI(TAG, "Received Write for Light Saturation %f", write->val.f);
            if (led_strip_set_saturation(num, write->val.f, &segment_on[num], &leds_changed, &segment_saturation[num]) == 0) {
                *(write->status) = HAP_STATUS_SUCCESS;
            }
        } else {
            *(write->status) = HAP_STATUS_RES_ABSENT;
        }
        /* If the characteristic write was successful, update it in hap core
         */
        if (*(write->status) == HAP_STATUS_SUCCESS) {
            hap_char_update_val(write->hc, &(write->val));
        } else {
            /* Else, set the return value appropriately to report error */
            ret = HAP_FAIL;
        }
    }
    return ret;
}

/*The main thread for handling the Bridge Accessory */
static void leds_thread_entry(void *p)
{
    /* Initialize the Light Bulb Hardware */
    rc = neopixel_init(NEOPIXEL_PORT, NEOPIXEL_RMT_CHANNEL);
	ESP_LOGI(TAG, "neopixel_init rc = %d", rc);

	for	( i = 0 ; i < NR_LED; i ++ )	{
		pixels[i] = 0;
	}
	px.pixels = (uint8_t *)pixels;
	px.pixel_count = NR_LED;
#ifdef	NEOPIXEL_WS2812
	strcpy(px.color_order, "GRB");
#else
	strcpy(px.color_order, "GRBW");
#endif

	memset(&px.timings, 0, sizeof(px.timings));
	px.timings.mark.level0 = 1;
	px.timings.space.level0 = 1;
	px.timings.mark.duration0 = 12;
#ifdef	NEOPIXEL_WS2812
	px.nbits = 24;
	px.timings.mark.duration1 = 14;
	px.timings.space.duration0 = 7;
	px.timings.space.duration1 = 16;
	px.timings.reset.duration0 = 600;
	px.timings.reset.duration1 = 600;
#endif
#ifdef	NEOPIXEL_SK6812
	px.nbits = 32;
	px.timings.mark.duration1 = 12;
	px.timings.space.duration0 = 6;
	px.timings.space.duration1 = 18;
	px.timings.reset.duration0 = 900;
	px.timings.reset.duration1 = 900;
#endif
	px.brightness = 0x80;
	for	( i = 1 ; i < NR_LED - 1 ; i ++ )	{
		np_set_pixel_rgbw(&px, i , 100, 0, 0, 50);
	}

	np_show(&px, NEOPIXEL_RMT_CHANNEL);
    
    ESP_LOGI(TAG, "getting here (end) rc = %d", rc);
    for(;;)
    {
        if(leds_changed)
        {
            ESP_LOGI(TAG, "leds_changed %d", rc);
            for(int segment = 0; segment < NUM_LED_SEGMENTS - 1; segment++)
            {
                int start_at_led = segment_center[segment];
                int end_at_led = segment_center[segment + 1];
                int leds_in_segment = end_at_led - start_at_led;
                int start_at_0 = 1;
                if(segment  == 0){
                    start_at_0 = 0;
                }
                for(int led = start_at_0; led < leds_in_segment; led++){
                    uint32_t current_hue = segment_hue[start_at_led] + (segment_hue[end_at_led] - segment_hue[start_at_led]) / leds_in_segment * led;
                    uint32_t current_saturation = segment_saturation[start_at_led] + (segment_saturation[end_at_led] - segment_saturation[start_at_led]) / leds_in_segment * led;
                    uint32_t current_intensity = segment_intensity[start_at_led] + (segment_intensity[end_at_led] - segment_intensity[start_at_led]) / leds_in_segment * led;
                    int current_led = start_at_led + led;
                    int rgbw[4];
                    hsi2rgbw(current_hue, current_saturation, current_intensity, &rgbw);
                    np_set_pixel_rgbw(&px, current_led, rgbw[0], rgbw[1], rgbw[2], rgbw[3]);
                }
            }
        }
    }
}
/*The main thread for handling the Bridge Accessory */
static void bridge_thread_entry(void *p)
{
    hap_acc_t *accessory;
    hap_serv_t *service;

    /* Initialize the HAP core */
    hap_init(HAP_TRANSPORT_WIFI);

    /* Initialise the mandatory parameters for Accessory which will be added as
     * the mandatory services internally
     */
    hap_acc_cfg_t cfg = {
        .name = "Esp-Bridge",
        .manufacturer = "Espressif",
        .model = "EspBridge01",
        .serial_num = "001122334455",
        .fw_rev = "0.9.0",
        .hw_rev = NULL,
        .pv = "1.1.0",
        .identify_routine = bridge_identify,
        .cid = HAP_CID_BRIDGE,
    };
    /* Create accessory object */
    accessory = hap_acc_create(&cfg);

    /* Add a dummy Product Data */
    uint8_t product_data[] = {'E','S','P','3','2','H','A','P'};
    hap_acc_add_product_data(accessory, product_data, sizeof(product_data));

    /* Add the Accessory to the HomeKit Database */
    hap_add_accessory(accessory);

    /* Create and add the Accessory to the Bridge object*/
    for (uint8_t i = 0; i < NUM_BRIDGED_ACCESSORIES; i++) {
        char accessory_name[12] = {0};
        sprintf(accessory_name, "Led-Strip-%d", i);

        hap_acc_cfg_t bridge_cfg = {
            .name = accessory_name,
            .manufacturer = "Espressif",
            .model = "LedStrip01",
            .serial_num = "abcdefg",
            .fw_rev = "0.9.0",
            .hw_rev = NULL,
            .pv = "1.1.0",
            .identify_routine = accessory_identify,
            .cid = HAP_CID_BRIDGE,
        };
        /* Create accessory object */
        accessory = hap_acc_create(&bridge_cfg);

        /* Create the Lightbulb Service. Include the "name" since this is a user visible service  */
        service = hap_serv_lightbulb_create(false);
        // hap_serv_add_char(service, hap_char_name_create(accessory_name));
        /* Add the optional characteristic to the Light Bulb Service */
        int ret = hap_serv_add_char(service, hap_char_name_create("My Light"));
        ret |= hap_serv_add_char(service, hap_char_brightness_create(50));
        ret |= hap_serv_add_char(service, hap_char_hue_create(180));
        ret |= hap_serv_add_char(service, hap_char_saturation_create(100));
        if (ret != HAP_SUCCESS) {
            ESP_LOGE(TAG, "Failed to add optional characteristics to LightBulb");
        }
        /* Set the Accessory name as the Private data for the service,
         * so that the correct accessory can be identified in the
         * write callback
         */
        hap_serv_set_priv(service, strdup(accessory_name));

        /* Set the write callback for the service */
        hap_serv_set_write_cb(service, led_strip_write);
 
        /* Add the Fan Service to the Accessory Object */
        hap_acc_add_serv(accessory, service);

        /* Add the Accessory to the HomeKit Database */
        hap_add_bridged_accessory(accessory, hap_get_unique_aid(accessory_name));
    }

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
    xTaskCreate(leds_thread_entry, LED_STRIP_LEDS_TASK_NAME, LED_STRIP_TASK_STACKSIZE, NULL, LED_STRIP_TASK_PRIORITY, NULL);
    xTaskCreate(bridge_thread_entry, LED_STRIP_BRIDGE_TASK_NAME, LED_STRIP_TASK_STACKSIZE, NULL, LED_STRIP_TASK_PRIORITY, NULL);
}
