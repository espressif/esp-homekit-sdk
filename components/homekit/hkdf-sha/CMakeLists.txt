set(COMPONENT_ADD_INCLUDEDIRS ./include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS ./upstream/hkdf.c ./upstream/hmac.c ./upstream/sha1.c ./upstream/sha224-256.c ./upstream/sha384-512.c ./upstream/shatest.c ./upstream/usha.c)

register_component()
