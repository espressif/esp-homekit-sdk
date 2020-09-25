set(COMPONENT_ADD_INCLUDEDIRS ./button/include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS ./button/button.c ./button/button_obj.cpp)

register_component()
