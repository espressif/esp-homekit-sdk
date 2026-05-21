idf_component_register(SRCS "app_wifi.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_hap_core esp_hap_platform nvs_flash
                    PRIV_REQUIRES network_provisioning qrcode)
