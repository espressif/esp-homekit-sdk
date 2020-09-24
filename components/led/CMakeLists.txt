set(COMPONENT_ADD_INCLUDEDIRS ./led/include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS ./led/led.c)

register_component()
