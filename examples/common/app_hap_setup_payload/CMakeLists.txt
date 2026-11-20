idf_component_register(SRCS "app_hap_setup_payload.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_hap_core
                    PRIV_REQUIRES qrcode)
