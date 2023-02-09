set(HAP_SDK_VER "4.0")
execute_process (
    COMMAND bash -c "git log --pretty=format:\"%h\" -1"
    OUTPUT_VARIABLE GIT_COMMIT
    WORKING_DIRECTORY ${COMPONENT_DIR}
)
#execute_process(COMMAND "git log --pretty=format:\"%h\"" -1 OUTPUT_VARIABLE GIT_COMMIT)
set(MFI_VER ${HAP_SDK_VER}-${GIT_COMMIT})
message(STATUS ${MFI_VER})


# CORE
set(srcs src/byte_convert.c
        src/esp_hap_acc.c
        src/esp_hap_bct.c
        src/esp_hap_char.c
        src/esp_hap_controllers.c
        src/esp_hap_database.c
        src/esp_hap_ip_services.c
        src/esp_hap_keystore.c
        src/esp_hap_main.c
        src/esp_hap_mdns.c
        src/esp_hap_network_io.c
        src/esp_hap_pair_common.c
        src/esp_hap_pair_setup.c
        src/esp_hap_pair_verify.c
        src/esp_hap_pairings.c
        src/esp_hap_serv.c
        src/esp_hap_wifi.c
        src/esp_hap_setup_payload.c
        src/hexbin.c
        src/hexdump.c
        src/esp_mfi_debug.c)

set(priv_includes src/priv_includes)

if(CONFIG_HAP_MFI_ENABLE)
    list(APPEND srcs src/mfi/esp_mfi_auth.c src/mfi/esp_mfi_coprocessor.c src/mfi/esp_mfi_element.c src/mfi/esp_mfi_sap.c src/mfi/esp_mfi_pair_setup.c src/mfi/esp_hap_secure_message.c src/mfi/esp_mfi_wac.c src/mfi/esp_hap_wifi_transport.c)
    list(APPEND priv_includes src/mfi)
else()
    list(APPEND srcs src/esp_mfi_dummy.c)

endif()

set(priv_req libsodium esp_http_server hkdf-sha mu_srp json_generator json_parser esp_hap_platform esp_hap_apple_profiles mdns)
# esp_timer component was introduced in v4.2
if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER "4.1")
    list(APPEND priv_req esp_timer)
endif()

if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "5.0")
    list(APPEND priv_req esp_wifi)
    list(APPEND req esp_event)
endif()

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ${priv_includes}
                       REQUIRES ${req}
                       PRIV_REQUIRES ${priv_req})
component_compile_options(-Wno-unused-function)
target_compile_definitions(${COMPONENT_TARGET} PRIVATE "-D MFI_VER=\"${MFI_VER}\"")
# Added just to automatically trigger re-runs of CMake
git_describe(ESP_HOMEKIT_VERSION ${COMPONENT_DIR})
message(STATUS "ESP HomeKit SDK Project commit: " ${ESP_HOMEKIT_VERSION})
