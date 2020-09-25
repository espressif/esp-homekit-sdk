idf_component_register(SRCS ./app_main.c ./hap_aws_iot_pub_sub.c ./hap_aws_iot_thing_shadow.c
                        INCLUDE_DIRS ".")

target_add_binary_data(${COMPONENT_TARGET} "certs/server.crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/device.crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/device.key" TEXT)
