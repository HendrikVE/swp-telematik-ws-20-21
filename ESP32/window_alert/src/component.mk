#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_EMBED_TXTFILES := storage/certs/ca.crt storage/certs/client.crt storage/certs/client.key

COMPONENT_ADD_INCLUDEDIRS := . hardware manager storage
COMPONENT_SRCDIRS := . hardware manager storage
