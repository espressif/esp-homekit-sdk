#
# Component Makefile
#
HAP_SDK_VER := "4.0"
COMPONENT_SRCDIRS := src
COMPONENT_PRIV_INCLUDEDIRS := src/priv_includes
MFI_VER := $(HAP_SDK_VER)-$(shell git log --pretty=format:"%h" -1)

ifdef CONFIG_HAP_MFI_ENABLE
COMPONENT_SRCDIRS += src/mfi
COMPONENT_OBJEXCLUDE += src/esp_mfi_dummy.o
COMPONENT_PRIV_INCLUDEDIRS += src/mfi
endif

CPPFLAGS += -D MFI_VER=\"$(MFI_VER)\"

CFLAGS += -Wno-unused-function
COMPONENT_ADD_INCLUDEDIRS := include
