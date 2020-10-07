set(srcs src/esp32/esp_mfi_aes.c src/esp32/esp_mfi_base64.c src/esp32/esp_mfi_rand.c src/esp32/esp_mfi_sha.c src/esp32/hap_platform_httpd.c src/esp32/hap_platform_keystore.c src/esp32/hap_platform_memory.c src/esp32/hap_platform_os.c)

if(NOT CONFIG_IDF_TARGET_ESP8266)
    list(APPEND srcs src/esp32/esp_mfi_i2c.c)
endif()
idf_component_register(SRCS ${srcs}
                        INCLUDE_DIRS "include"
                        REQUIRES esp_http_server
                        PRIV_REQUIRES mbedtls nvs_flash mdns esp_hap_core)
component_compile_options(-Wno-unused-function)
