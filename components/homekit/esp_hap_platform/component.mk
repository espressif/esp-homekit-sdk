#
# Component Makefile
#
CFLAGS += -Wno-unused-function
COMPONENT_SRCDIRS := src
ifdef CONFIG_IDF_TARGET_ESP8266
COMPONENT_OBJEXCLUDE += src/esp_mfi_i2c.o
endif
COMPONENT_ADD_INCLUDEDIRS := include
