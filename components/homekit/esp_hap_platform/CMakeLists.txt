set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES esp_http_server)
set(COMPONENT_PRIV_REQUIRES mbedtls nvs_flash mdns esp_eth esp_hap_core)

set(COMPONENT_SRCS src/esp32/esp_mfi_aes.c src/esp32/esp_mfi_base64.c src/esp32/esp_mfi_i2c.c src/esp32/esp_mfi_rand.c src/esp32/esp_mfi_sha.c src/esp32/hap_platform_httpd.c src/esp32/hap_platform_keystore.c src/esp32/hap_platform_memory.c src/esp32/hap_platform_os.c)

register_component()
component_compile_options(-Wno-unused-function)
