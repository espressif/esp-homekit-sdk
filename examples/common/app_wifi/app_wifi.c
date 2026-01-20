/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


/*
 * This supports 3 ways of connecting to the Wi-Fi Network
 * 1. Hard coded credentials
 * 2. Unified Provisioning
 * 3. Apple Wi-Fi Accessory Configuration (WAC. Available only on MFi variant of the SDK)
 *
 * Unified Provisioning has 2 options
 * 1. BLE Provisioning
 * 2. SoftAP Provisioning
 *
 * Unified and WAC Provisioning can co-exist.
 * If the SoftAP Unified Provisioning is being used, the same SoftAP interface
 * will be used for WAC. However, if BLE Unified Provisioning is used, the HAP
 * Helper functions for softAP start/stop will be used
 */
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_idf_version.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0)
// Features supported in 4.1+
#define ESP_NETIF_SUPPORTED
#endif

#ifdef ESP_NETIF_SUPPORTED
#include <esp_netif.h>
#else
#include <tcpip_adapter.h>
#endif

#include <wifi_provisioning/manager.h>

#ifdef CONFIG_APP_WIFI_USE_BOTH_PROVISIONING
#define USE_UNIFIED_PROVISIONING
#define USE_WAC_PROVISIONING
#endif

#ifdef CONFIG_APP_WIFI_USE_UNIFIED_PROVISIONING
#define USE_UNIFIED_PROVISIONING
#endif

#ifdef CONFIG_APP_WIFI_USE_WAC_PROVISIONING
#define USE_WAC_PROVISIONING
#endif

#ifdef USE_UNIFIED_PROVISIONING
#ifdef CONFIG_APP_WIFI_PROV_TRANSPORT_BLE
#include <wifi_provisioning/scheme_ble.h>
#else /* CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP */
#include <wifi_provisioning/scheme_softap.h>
#include <hap_platform_httpd.h>
#endif /* CONFIG_APP_WIFI_PROV_TRANSPORT_BLE */
#include <qrcode.h>
#endif /* USE_UNIFIED_PROVISIONING */

#ifdef USE_WAC_PROVISIONING
#include <hap_wac.h>
#endif /* USE_WAC_PROVISIONING */

#include <nvs.h>
#include <nvs_flash.h>
#include "app_wifi.h"

static const char *TAG = "app_wifi";
static const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t wifi_event_group;


#ifdef USE_UNIFIED_PROVISIONING
#define PROV_QR_VERSION "v1"

#define PROV_TRANSPORT_SOFTAP   "softap"
#define PROV_TRANSPORT_BLE      "ble"
#define QRCODE_BASE_URL     "https://espressif.github.io/esp-jumpstart/qrcode.html"

#define CREDENTIALS_NAMESPACE   "rmaker_creds"
#define RANDOM_NVS_KEY          "random"

static void app_wifi_print_qr(const char *name, const char *pop, const char *transport)
{
    if (!name || !pop || !transport) {
        ESP_LOGW(TAG, "Cannot generate QR code payload. Data missing.");
        return;
    }
    char payload[150];
    snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                    ",\"pop\":\"%s\",\"transport\":\"%s\"}",
                    PROV_QR_VERSION, name, pop, transport);
#ifdef CONFIG_APP_WIFI_PROV_SHOW_QR
    ESP_LOGI(TAG, "-----QR Code for ESP Provisioning-----");
    ESP_LOGI(TAG, "Scan this QR code from the phone app for Provisioning.");
    qrcode_display(payload);
#endif /* CONFIG_APP_WIFI_PROV_SHOW_QR */
    ESP_LOGI(TAG, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s", QRCODE_BASE_URL, payload);
}

static void get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X",
             ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

static esp_err_t get_device_pop(char *pop, size_t max)
{
    if (!pop || !max) {
        return ESP_ERR_INVALID_ARG;
    }

        uint8_t eth_mac[6];
        esp_err_t err = esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
        if (err == ESP_OK) {
            snprintf(pop, max, "%02x%02x%02x%02x", eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);
            return ESP_OK;
        } else {
            return err;
        }
}
#endif /* USE_UNIFIED_PROVISIONING */

#ifdef USE_WAC_PROVISIONING
#ifdef CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP
static void app_wac_softap_start(char *ssid)
{
}
#else
static void app_wac_softap_start(char *ssid)
{
    hap_wifi_softap_start(ssid);
}
#endif /* ! CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP */
static void app_wac_softap_stop(void)
{
    hap_wifi_softap_stop();
}
static void app_wac_sta_connect(wifi_config_t *wifi_cfg)
{
#ifdef USE_UNIFIED_PROVISIONING
    wifi_prov_mgr_configure_sta(wifi_cfg);
#else
    hap_wifi_sta_connect(wifi_cfg);
#endif
}
#endif /* USE_WAC_PROVISIONING */

/* Event handler for catching system events */
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
#ifdef ESP_NETIF_SUPPORTED
        esp_netif_create_ip6_linklocal((esp_netif_t *)arg);
#else
        tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
#endif
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_GOT_IP6) {
        ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
        ESP_LOGI(TAG, "Connected with IPv6 Address:" IPV6STR, IPV62STR(event->ip6_info.ip));
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
        esp_wifi_connect();
#ifdef USE_UNIFIED_PROVISIONING
    } else if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                ESP_LOGI(TAG, "Provisioning started");
                break;
            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(TAG, "Received Wi-Fi credentials"
                         "\n\tSSID     : %s\n\tPassword : %s",
                         (const char *) wifi_sta_cfg->ssid,
                         (const char *) wifi_sta_cfg->password);
                break;
            }
            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                         "\n\tPlease reset to factory and retry provisioning",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                         "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Provisioning successful");
                break;
            case WIFI_PROV_END:
#ifdef USE_WAC_PROVISIONING
                hap_wac_stop();
#endif
                /* De-initialize manager once provisioning is finished */
                wifi_prov_mgr_deinit();
                break;
            default:
                break;
        }
#endif /* USE_UNIFIED_PROVISIONING */
#ifdef USE_WAC_PROVISIONING
    } else if (event_base == HAP_WAC_EVENT) {
        switch (event_id) {
            case HAP_WAC_EVENT_REQ_SOFTAP_START:
                app_wac_softap_start((char *)event_data);
                break;
            case HAP_WAC_EVENT_REQ_SOFTAP_STOP:
                app_wac_softap_stop();
                break;
            case HAP_WAC_EVENT_RECV_CRED:
                app_wac_sta_connect((wifi_config_t *)event_data);
                break;
            case HAP_WAC_EVENT_STOPPED:
                ESP_LOGI(TAG, "WAC Stopped");
                break;
            default:
                break;
        }
#endif /* USE_WAC_PROVISIONING */
    }
}

void app_wifi_init(void)
{
    /* Initialize TCP/IP */
#ifdef ESP_NETIF_SUPPORTED
    esp_netif_init();
#else
    tcpip_adapter_init();
#endif

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_event_group = xEventGroupCreate();

    /* Initialize Wi-Fi including netif with default config */
#ifdef ESP_NETIF_SUPPORTED
    esp_netif_t *wifi_netif = esp_netif_create_default_wifi_sta();
#endif

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
#ifdef ESP_NETIF_SUPPORTED
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, wifi_netif));
#else
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
#endif
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6, &event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

#ifdef CONFIG_APP_WIFI_USE_HARDCODED
#define APP_WIFI_SSID   CONFIG_APP_WIFI_SSID
#define APP_WIFI_PASS   CONFIG_APP_WIFI_PASSWORD
esp_err_t app_wifi_start(TickType_t ticks_to_wait)
{
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = APP_WIFI_SSID,
            .password = APP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. In case your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    /* Wait for Wi-Fi connection */
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, false, true, ticks_to_wait);
    return ESP_OK;
    return ESP_OK;
}
#endif

#ifdef CONFIG_APP_WIFI_USE_PROVISIONING
static void wifi_init_sta()
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

esp_err_t app_wifi_start(TickType_t ticks_to_wait)
{
#ifdef USE_UNIFIED_PROVISIONING
    /* Configuration for the provisioning manager */
    wifi_prov_mgr_config_t config = {
        /* What is the Provisioning Scheme that we want ?
         * wifi_prov_scheme_softap or wifi_prov_scheme_ble */
#ifdef CONFIG_APP_WIFI_PROV_TRANSPORT_BLE
        .scheme = wifi_prov_scheme_ble,
#else /* CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP */
        .scheme = wifi_prov_scheme_softap,
#endif /* CONFIG_APP_WIFI_PROV_TRANSPORT_BLE */

        /* Any default scheme specific event handler that you would
         * like to choose. Since our example application requires
         * neither BT nor BLE, we can choose to release the associated
         * memory once provisioning is complete, or not needed
         * (in case when device is already provisioned). Choosing
         * appropriate scheme specific event handler allows the manager
         * to take care of this automatically. This can be set to
         * WIFI_PROV_EVENT_HANDLER_NONE when using wifi_prov_scheme_softap*/
#ifdef CONFIG_APP_WIFI_PROV_TRANSPORT_BLE
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
#else /* CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP */
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
#endif /* CONFIG_APP_WIFI_PROV_TRANSPORT_BLE */
    };

    /* Initialize provisioning manager with the
     * configuration parameters set above */
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
#endif /* USE_UNIFIED_PROVISIONING */

    /* Let's find out if the device is provisioned */
    bool provisioned = false;
#ifdef USE_UNIFIED_PROVISIONING
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
#else
    ESP_ERROR_CHECK(hap_wifi_is_provisioned(&provisioned));
#endif

    /* If device is not yet provisioned start provisioning service */
    if (!provisioned) {
        ESP_LOGI(TAG, "Starting provisioning");
#ifdef ESP_NETIF_SUPPORTED
#if defined(CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP) || defined(USE_WAC_PROVISIONING)
        esp_netif_create_default_wifi_ap();
#endif
#endif /* ESP_NETIF_SUPPORTED */
#ifdef USE_UNIFIED_PROVISIONING
        esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
        /* What is the Device Service Name that we want
         * This translates to :
         *     - Wi-Fi SSID when scheme is wifi_prov_scheme_softap
         *     - device name when scheme is wifi_prov_scheme_ble
         */
        char service_name[12];
        get_device_service_name(service_name, sizeof(service_name));

        /* What is the security level that we want (0 or 1):
         *      - WIFI_PROV_SECURITY_0 is simply plain text communication.
         *      - WIFI_PROV_SECURITY_1 is secure communication which consists of secure handshake
         *          using X25519 key exchange and proof of possession (pop) and AES-CTR
         *          for encryption/decryption of messages.
         */
        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

        /* Do we want a proof-of-possession (ignored if Security 0 is selected):
         *      - this should be a string with length > 0
         *      - NULL if not used
         */
        char pop[9];
        esp_err_t err = get_device_pop(pop, sizeof(pop));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error: %d. Failed to get PoP from NVS, Please perform Claiming.", err);
            return err;
        }

        /* What is the service key (Wi-Fi password)
         * NULL = Open network
         * This is ignored when scheme is wifi_prov_scheme_ble
         */
        const char *service_key = NULL;

#ifdef CONFIG_APP_WIFI_PROV_TRANSPORT_BLE
        /* This step is only useful when scheme is wifi_prov_scheme_ble. This will
         * set a custom 128 bit UUID which will be included in the BLE advertisement
         * and will correspond to the primary GATT service that provides provisioning
         * endpoints as GATT characteristics. Each GATT characteristic will be
         * formed using the primary service UUID as base, with different auto assigned
         * 12th and 13th bytes (assume counting starts from 0th byte). The client side
         * applications must identify the endpoints by reading the User Characteristic
         * Description descriptor (0x2901) for each characteristic, which contains the
         * endpoint name of the characteristic */
        uint8_t custom_service_uuid[] = {
            /* This is a random uuid. This can be modified if you want to change the BLE uuid. */
            /* 12th and 13th bit will be replaced by internal bits. */
            0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
            0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
        };
        err = wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "wifi_prov_scheme_ble_set_service_uuid failed %d", err);
            return err;
        }
#endif /* CONFIG_APP_WIFI_PROV_TRANSPORT_BLE */
#ifdef CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP
        wifi_prov_scheme_softap_set_httpd_handle(hap_platform_httpd_get_handle());
#endif /* CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP */

        /* Start provisioning service */
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, pop, service_name, service_key));
        /* Print QR code for provisioning */
#ifdef CONFIG_APP_WIFI_PROV_TRANSPORT_BLE
        app_wifi_print_qr(service_name, pop, PROV_TRANSPORT_BLE);
#else /* CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP */
        app_wifi_print_qr(service_name, pop, PROV_TRANSPORT_SOFTAP);
#endif /* CONFIG_APP_WIFI_PROV_TRANSPORT_BLE */
        ESP_LOGI(TAG, "Provisioning Started. Name : %s, POP : %s", service_name, pop);
#endif /* USE_UNIFIED_PROVISIONING */
#ifdef USE_WAC_PROVISIONING
        esp_event_handler_register(HAP_WAC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
        hap_wac_start();
#endif /* USE_WAC_PROVISIONING */
    } else {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");
#ifdef USE_UNIFIED_PROVISIONING
        /* We don't need the manager as device is already provisioned,
         * so let's release it's resources */
        wifi_prov_mgr_deinit();
#endif /* USE_UNIFIED_PROVISIONING */
        /* Start Wi-Fi station */
        wifi_init_sta();
    }
    /* Wait for Wi-Fi connection */
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, false, true, ticks_to_wait);
    return ESP_OK;
}
#endif /* CONFIG_APP_WIFI_USE_PROVISIONING */
