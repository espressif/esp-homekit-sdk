/* AWS-IOT Homekit Lightbulb Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <sdkconfig.h>

#ifdef CONFIG_EXAMPLE_AWS_THING_SHADOW
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include <aws_iot_shadow_interface.h>

#include "hap.h"
#include "hap_apple_chars.h"
#include "hap_apple_servs.h"
#include "hap_aws_iot.h"

#define AWS_JSON_KEY_ON         "on"
#define AWS_JSON_KEY_BRIGHTNESS "brightness"

extern const uint8_t server_cert[] asm("_binary_server_crt_start");
extern const uint8_t device_cert[] asm("_binary_device_crt_start");
extern const uint8_t device_key[] asm("_binary_device_key_start");

#define AWS_IOT_TASK_PRIORITY  5
#define AWS_IOT_TASK_STACKSIZE 12 * 1024
#define AWS_IOT_TASK_NAME      "aws_iot_task"

#define AWS_IOT_QUEUE_SIZE  8

static const char *TAG = "HAP-AWS-IOT";

static bool aws_iot_stop;
static bool aws_iot_started;
static bool aws_iot_report_change;

void hap_aws_iot_report_on_char(bool val)
{
    ESP_LOGI(TAG, "On value changed to %s. Will be reported to cloud", val? "true":"false");
}

void hap_aws_iot_report_brightness_char(int val)
{    
    ESP_LOGI(TAG, "Brightness value changed to %d. Will be reported to cloud", val);
}
static bool hap_get_on_char_val()
{
    hap_serv_t *hs = hap_acc_get_serv_by_uuid(hap_get_first_acc(), HAP_SERV_UUID_LIGHTBULB);
    if (hs) {
        hap_char_t *hc =  hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_ON);
        if (hc) {
            const hap_val_t *val = hap_char_get_val(hc);
            return val->b;
        }
    }
    ESP_LOGE(TAG, "Could not find the ON characteristic. Reporting a dummy value");
    return false;
}
static int hap_get_brightness_char_val()
{
    hap_serv_t *hs = hap_acc_get_serv_by_uuid(hap_get_first_acc(), HAP_SERV_UUID_LIGHTBULB);
    if (hs) {
        hap_char_t *hc =  hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_BRIGHTNESS);
        if (hc) {
            const hap_val_t *val = hap_char_get_val(hc);
            return val->i;
        }
    }
    ESP_LOGE(TAG, "Could not find the Brightness characteristic. Reporting a dummy value");
    return 0;
}

static void set_char_value(const char* char_uuid, hap_val_t *val) 
{
    hap_serv_t *hs = hap_acc_get_serv_by_uuid(hap_get_first_acc(), HAP_SERV_UUID_LIGHTBULB);
    if (hs) {
        hap_char_t *hc =  hap_serv_get_char_by_uuid(hs, char_uuid);
        if (hc) {
            hap_char_update_val(hc, val);
        }
    }
}

void onState_callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
    IOT_UNUSED(pJsonString);
    IOT_UNUSED(JsonStringDataLen);

    if(pContext) {
        hap_val_t val = {
            .b = *(bool *)pContext->pData,
        };
        set_char_value(HAP_CHAR_UUID_ON, &val);
    }
    aws_iot_report_change = true;
}

void brightness_callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
    IOT_UNUSED(pJsonString);
    IOT_UNUSED(JsonStringDataLen);

    if(pContext) {
        hap_val_t val = {
            .i = *(int *)pContext->pData,
        };
        set_char_value(HAP_CHAR_UUID_BRIGHTNESS, &val);
    }
    aws_iot_report_change = true;
}

static bool shadowUpdateInProgress;

void ShadowUpdateStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
                                const char *pReceivedJsonDocument, void *pContextData) {
    IOT_UNUSED(pThingName);
    IOT_UNUSED(action);
    IOT_UNUSED(pReceivedJsonDocument);
    IOT_UNUSED(pContextData);

    shadowUpdateInProgress = false;

    if(SHADOW_ACK_TIMEOUT == status) {
        ESP_LOGE(TAG, "Update timed out");
    } else if(SHADOW_ACK_REJECTED == status) {
        ESP_LOGE(TAG, "Update rejected");
    } else if(SHADOW_ACK_ACCEPTED == status) {
        ESP_LOGI(TAG, "Update accepted");
    }
}

IoT_Error_t aws_iot_shadow_update_reported(AWS_IoT_Client *aws_client, jsonStruct_t *firstHandler, jsonStruct_t *secondHandler)
{
    char JsonDocumentBuffer[200];
    size_t sizeOfJsonDocumentBuffer = sizeof(JsonDocumentBuffer) / sizeof(JsonDocumentBuffer[0]);
    IoT_Error_t rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
    if(SUCCESS != rc) {
        return rc;
    }
    rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 2, firstHandler, secondHandler);
    if(SUCCESS != rc) {
        return rc;
    }
    rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
    if(SUCCESS != rc) {
        return rc;
    }
    ESP_LOGI(TAG, "Update Shadow: %s", JsonDocumentBuffer);
    rc = aws_iot_shadow_update(aws_client, CONFIG_EXAMPLE_AWS_CLIENT_ID, JsonDocumentBuffer,
            ShadowUpdateStatusCallback, NULL, 4, true);
    shadowUpdateInProgress = true;
    return rc;
}
/* While updating desired, we will also update the reported, so that there is no delta */
IoT_Error_t aws_iot_shadow_update_desired(AWS_IoT_Client *aws_client, jsonStruct_t *firstHandler, jsonStruct_t *secondHandler)
{
    char JsonDocumentBuffer[200];
    size_t sizeOfJsonDocumentBuffer = sizeof(JsonDocumentBuffer) / sizeof(JsonDocumentBuffer[0]);
    IoT_Error_t rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
    if(SUCCESS != rc) {
        return rc;
    }
    if (firstHandler && secondHandler) {
        aws_iot_shadow_add_desired(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 2, firstHandler, secondHandler);
        /* Checking only second call is enough as the JSON overflow can be detected here as well */
        rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 2, firstHandler, secondHandler);
        if(SUCCESS != rc) {
            return rc;
        }
    } else if (firstHandler) {
        aws_iot_shadow_add_desired(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 1, firstHandler);
        rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 1, firstHandler);
        if(SUCCESS != rc) {
            return rc;
        }
    } else if (secondHandler) {
        aws_iot_shadow_add_desired(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 1, secondHandler);
        rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 1, secondHandler);
        if(SUCCESS != rc) {
            return rc;
        }
    }
    rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
    if(SUCCESS != rc) {
        return rc;
    }
    ESP_LOGI(TAG, "Update Shadow: %s", JsonDocumentBuffer);
    rc = aws_iot_shadow_update(aws_client, CONFIG_EXAMPLE_AWS_CLIENT_ID, JsonDocumentBuffer,
            ShadowUpdateStatusCallback, NULL, 4, true);
    shadowUpdateInProgress = true;
    return rc;
}
/* Task function created for main loop */
static void aws_iot_task(void *p)
{
    AWS_IoT_Client aws_client;
    IoT_Error_t rc = FAILURE;

    ShadowInitParameters_t sp = ShadowInitParametersDefault;
    ESP_LOGI(TAG, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);
    ESP_LOGI(TAG, "Server %s, Port %d", CONFIG_AWS_IOT_MQTT_HOST, CONFIG_AWS_IOT_MQTT_PORT);
    sp.pHost = CONFIG_AWS_IOT_MQTT_HOST;
    sp.port = CONFIG_AWS_IOT_MQTT_PORT;
    sp.pRootCA = (const char *)server_cert;
    sp.pClientCRT = (const char *)device_cert;
    sp.pClientKey = (const char *)device_key;
    sp.enableAutoReconnect = true;
    sp.disconnectHandler = NULL;

    ESP_LOGI(TAG, "Shadow Init");
    rc = aws_iot_shadow_init(&aws_client, &sp);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "aws_iot_shadow_init returned error %d, aborting...", rc);
        goto err;
    }
    ShadowConnectParameters_t scp = ShadowConnectParametersDefault;
    /* Thing name and Client ID can be different, Using same, just for convenience */
    scp.pMyThingName = CONFIG_EXAMPLE_AWS_CLIENT_ID;
    scp.pMqttClientId = CONFIG_EXAMPLE_AWS_CLIENT_ID;
    scp.mqttClientIdLen = (uint16_t) strlen(CONFIG_EXAMPLE_AWS_CLIENT_ID);

    ESP_LOGI(TAG, "Connecting to AWS.");
    do {
        rc = aws_iot_shadow_connect(&aws_client, &scp);
        if (SUCCESS != rc) {
            ESP_LOGE(TAG, "Error(%d) connecting to %s: %d", rc, sp.pHost, sp.port);
            ESP_LOGI(TAG, "Please check the device key/certificate, server certificate, endpoint URL and Port");
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
    } while (SUCCESS != rc);
    ESP_LOGI(TAG, "AWS connected successfully.");

    bool on_state = hap_get_on_char_val();
    jsonStruct_t onHandler;
    onHandler.cb = onState_callback;
    onHandler.pData = &on_state;
    onHandler.pKey = AWS_JSON_KEY_ON;
    onHandler.type = SHADOW_JSON_BOOL;
    onHandler.dataLength = sizeof(bool);

    int brightness = hap_get_brightness_char_val();
    jsonStruct_t brightnessHandler;
    brightnessHandler.cb = brightness_callback;
    brightnessHandler.pKey = AWS_JSON_KEY_BRIGHTNESS;
    brightnessHandler.pData = &brightness;
    brightnessHandler.type = SHADOW_JSON_INT32;
    brightnessHandler.dataLength = sizeof(int32_t);

    rc = aws_iot_shadow_register_delta(&aws_client, &onHandler);
    if (SUCCESS != rc) {
        ESP_LOGE(TAG, "Shadow Register Delta Error for %s", AWS_JSON_KEY_ON);
    }
    rc = aws_iot_shadow_register_delta(&aws_client, &brightnessHandler);
    if (SUCCESS != rc) {
        ESP_LOGE(TAG, "Shadow Register Delta Error for %s", AWS_JSON_KEY_BRIGHTNESS);
    }

    /* Report the initial values once */
    rc = aws_iot_shadow_update_reported(&aws_client, &onHandler, &brightnessHandler);
    while((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc) && !aws_iot_stop) {
        rc = aws_iot_shadow_yield(&aws_client, 200);
        if(NETWORK_ATTEMPTING_RECONNECT == rc || shadowUpdateInProgress) {
            rc = aws_iot_shadow_yield(&aws_client, 10);
            // If the client is attempting to reconnect, or waiting on a shadow update, we skip the rest of the loop
            continue;
        }
        /* Check for the stop command once after yield */
        if (aws_iot_stop) {
            break;
        }

        /* Check for any local changes and report accordingly */
        jsonStruct_t *firstHandler = NULL;
        jsonStruct_t *secondHandler = NULL;

        if (on_state != hap_get_on_char_val()) {
            on_state = hap_get_on_char_val();
            firstHandler = &onHandler;
        }
        if (brightness != hap_get_brightness_char_val()) {
            brightness = hap_get_brightness_char_val();
            secondHandler = &brightnessHandler;
        }
        if (firstHandler || secondHandler) {
            rc = aws_iot_shadow_update_desired(&aws_client, firstHandler, secondHandler);
            aws_iot_report_change = false;
            continue;
        }

        /* If there are no local changes, check if there is any change (triggered from cloud)
         * pending for reporting */
        if (aws_iot_report_change) {
            rc = aws_iot_shadow_update_reported(&aws_client, &onHandler, &brightnessHandler);
            aws_iot_report_change = false;
        }
    }
err:
    ESP_LOGI(TAG, "Stopping AWS-IOT Task");
    aws_iot_stop = false;
    aws_iot_started = false;
    vTaskDelete(NULL);
}

esp_err_t hap_aws_iot_start(void)
{
    if (aws_iot_started) {
        ESP_LOGW(TAG, "AWS IoT client already running");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Starting AWS IoT client");
    if (xTaskCreate(&aws_iot_task, "aws_iot_task", AWS_IOT_TASK_STACKSIZE, NULL, AWS_IOT_TASK_PRIORITY, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Couldn't create cloud task");
        return -ENOMEM;
    }
    aws_iot_started = true;
    return ESP_OK;
}

void hap_aws_iot_stop(void)
{
    if (aws_iot_started) {
        aws_iot_stop = true;
    }
}
#endif /* CONFIG_EXAMPLE_AWS_THING_SHADOW */
