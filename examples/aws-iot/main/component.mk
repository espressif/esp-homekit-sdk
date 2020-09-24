#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

# Certificate files. certificate.pem.crt & private.pem.key must be downloaded
# from AWS, see README for details.
COMPONENT_EMBED_TXTFILES := certs/server.crt certs/device.crt certs/device.key
