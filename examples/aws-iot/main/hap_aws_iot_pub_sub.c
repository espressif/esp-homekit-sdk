/* AWS-IOT Homekit Lightbulb Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <sdkconfig.h>
#ifdef CONFIG_EXAMPLE_AWS_PUB_SUB
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"

#include <json_parser.h>

#include "hap.h"
#include "hap_apple_servs.h"
#include "hap_apple_chars.h"
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

static QueueHandle_t aws_iot_queue;
static bool aws_iot_stop;
static bool aws_iot_started;

/* Sends the message to the shared queue */
static void aws_iot_send_message(char * out)
{
    char *jsondata = strdup(out);
    if (jsondata) {
        if(aws_iot_queue) {
            xQueueSend(aws_iot_queue, &jsondata, 0);
        } else {
            ESP_LOGE(TAG, "AWS Queue not created. Not reporting to AWS");
            free(jsondata);
        }
    }
}

void hap_aws_iot_report_on_char(bool val)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"%s\":%s}", AWS_JSON_KEY_ON, val ? "true" : "false");
    aws_iot_send_message(buf);
}

void hap_aws_iot_report_brightness_char(int val)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"%s\":%d}", AWS_JSON_KEY_BRIGHTNESS, val);
    aws_iot_send_message(buf);
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

static void aws_iot_subscribe_cb(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                    IoT_Publish_Message_Params *params, void *pData)
{
    if (!topicName)
        return;
    ESP_LOGI(TAG, "Subscribe callback %.*s", topicNameLen, topicName);

    if (!params) {
        ESP_LOGE(TAG, "No data received");
        return;
    }
    ESP_LOGI(TAG, "Received Data: %.*s", params->payloadLen, (char *)params->payload);

    hap_val_t val;
    jparse_ctx_t jctx;

    if (!json_parse_start(&jctx, (char *)params->payload, params->payloadLen)) {
        if(!json_obj_get_bool(&jctx, AWS_JSON_KEY_ON, &val.b))
            set_char_value(HAP_CHAR_UUID_ON, &val);

        if(!json_obj_get_int(&jctx, AWS_JSON_KEY_BRIGHTNESS, &val.i))
            set_char_value(HAP_CHAR_UUID_BRIGHTNESS, &val);
    }
    json_parse_end(&jctx);
}

/* Task function created for main loop */
static void aws_iot_task(void *p)
{
    AWS_IoT_Client aws_client;
    IoT_Error_t rc = FAILURE;

    IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
    IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

    ESP_LOGI(TAG, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

    mqttInitParams.enableAutoReconnect = true;
    ESP_LOGI(TAG, "Server %s, Port %d", CONFIG_AWS_IOT_MQTT_HOST, CONFIG_AWS_IOT_MQTT_PORT);
    mqttInitParams.pHostURL = CONFIG_AWS_IOT_MQTT_HOST;
    mqttInitParams.port = CONFIG_AWS_IOT_MQTT_PORT;

    mqttInitParams.pRootCALocation = (const char *)server_cert;
    mqttInitParams.pDeviceCertLocation = (const char *)device_cert;
    mqttInitParams.pDevicePrivateKeyLocation = (const char *)device_key;

    mqttInitParams.mqttCommandTimeout_ms = 20000;
    mqttInitParams.tlsHandshakeTimeout_ms = 5000;
    mqttInitParams.isSSLHostnameVerify = true;
    mqttInitParams.disconnectHandler = NULL;

    rc = aws_iot_mqtt_init(&aws_client, &mqttInitParams);
    if (SUCCESS != rc) {
        ESP_LOGE(TAG, "aws_iot_mqtt_init returned error : %d ", rc);
        goto err;
    }

    connectParams.keepAliveIntervalInSec = 10;
    connectParams.isCleanSession = true;
    connectParams.MQTTVersion = MQTT_3_1_1;
    connectParams.pClientID = CONFIG_EXAMPLE_AWS_CLIENT_ID;
    connectParams.clientIDLen = strlen(CONFIG_EXAMPLE_AWS_CLIENT_ID);
    connectParams.isWillMsgPresent = false;

    ESP_LOGI(TAG, "Connecting to AWS.");
    do {
        rc = aws_iot_mqtt_connect(&aws_client, &connectParams);
        if (SUCCESS != rc) {
            ESP_LOGE(TAG, "Error(%d) connecting to %s: %d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
            ESP_LOGI(TAG, "Please check the device key/certificate, server certificate, endpoint URL and Port");
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
    } while (SUCCESS != rc);
    ESP_LOGI(TAG, "AWS connected successfully.");
    rc = aws_iot_mqtt_subscribe(&aws_client, CONFIG_EXAMPLE_AWS_SUBSCRIBE_TOPIC, strlen(CONFIG_EXAMPLE_AWS_SUBSCRIBE_TOPIC),
            QOS0, aws_iot_subscribe_cb, NULL);
    if (SUCCESS != rc) {
        ESP_LOGW(TAG, "Error in subscribing to topic : %d ", rc);
    }

    while((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc) && !aws_iot_stop) {

        //Max time the yield function will wait for read messagesa
        rc = aws_iot_mqtt_yield(&aws_client, 100);
        /* Check for the stop command once after yield */
        if (aws_iot_stop) {
            break;
        }
        if(NETWORK_ATTEMPTING_RECONNECT == rc) {
            // If the client is attempting to reconnect we will skip the rest of the loop.
            continue;
        }
        /* Read all data from queue and publish to cloud */
        BaseType_t ret;
        do {
            char *payload;
            ret = xQueueReceive(aws_iot_queue, &payload, 0);
            if (ret == pdFAIL) {
                break;
            }
            IoT_Publish_Message_Params paramsQOS1;
            paramsQOS1.qos = QOS1;
            paramsQOS1.payload = (void *) payload;
            paramsQOS1.isRetained = 0;
            paramsQOS1.payloadLen = strlen(payload);
            ESP_LOGI(TAG, "Sending data: %s", payload);
            rc = aws_iot_mqtt_publish(&aws_client, CONFIG_EXAMPLE_AWS_PUBLISH_TOPIC, strlen(CONFIG_EXAMPLE_AWS_PUBLISH_TOPIC), &paramsQOS1);
            if(rc) {
                ESP_LOGE(TAG, "Error publishing the data.");
            }
            free(payload);
        } while (ret == pdPASS);
    }
err:
    ESP_LOGI(TAG, "Stopping AWS-IOT Task");
    vQueueDelete(aws_iot_queue);
    aws_iot_queue = NULL;
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
    aws_iot_queue = xQueueCreate(AWS_IOT_QUEUE_SIZE, sizeof(char *));
    if (!aws_iot_queue) {
        ESP_LOGE(TAG, "Queue Creation Failed");
        return -ENOMEM;
    }

    if (xTaskCreate(&aws_iot_task, "aws_iot_task", AWS_IOT_TASK_STACKSIZE, NULL, AWS_IOT_TASK_PRIORITY, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Couldn't create cloud task");
        vQueueDelete(aws_iot_queue);
        aws_iot_queue = NULL;
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
#endif /* CONFIG_EXAMPLE_AWS_PUB_SUB */
