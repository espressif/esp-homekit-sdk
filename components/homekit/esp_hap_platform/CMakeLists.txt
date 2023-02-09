set(srcs src/esp_mfi_aes.c src/esp_mfi_base64.c src/esp_mfi_rand.c src/esp_mfi_sha.c src/hap_platform_httpd.c src/hap_platform_keystore.c src/hap_platform_memory.c src/hap_platform_os.c)

if(NOT CONFIG_IDF_TARGET_ESP8266)
    list(APPEND srcs src/esp_mfi_i2c.c)
endif()

if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "5.0")
    list(APPEND priv_req driver)
endif()

idf_component_register(SRCS ${srcs}
                        INCLUDE_DIRS "include"
                        REQUIRES esp_http_server
                        PRIV_REQUIRES ${priv_req} mbedtls nvs_flash mdns esp_hap_core)
component_compile_options(-Wno-unused-function)
