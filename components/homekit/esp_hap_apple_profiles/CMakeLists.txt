set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES esp_hap_core)
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS src/hap_apple_chars.c src/hap_apple_servs.c)

register_component()
component_compile_options(-Wno-unused-function)
