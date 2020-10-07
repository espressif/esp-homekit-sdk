set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES esp_hap_core)
set(COMPONENT_PRIV_REQUIRES esp_http_server esp_https_ota esp_hap_platform app_update)

set(COMPONENT_SRCS src/hap_bct_http_handlers.c src/hap_fw_upgrade.c)

register_component()
component_compile_options(-Wno-unused-function)
