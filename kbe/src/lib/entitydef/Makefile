LIB =	entitydef

SRCS =				\
	fixedarray		\
	common			\
	datatype		\
	datatypes		\
	detaillevel		\
	entity_call		\
	entitycall_cross_server	\
	entitydef		\
	entitycallabstract		\
	fixeddict		\
	method			\
	property		\
	remote_entity_method	\
	scriptdef_module\
	volatileinfo

ifndef KBE_ROOT
export KBE_ROOT := $(subst /kbe/src/lib/$(LIB),,$(CURDIR))
endif

USE_PYTHON=1
USE_G3DMATH=1
include $(KBE_ROOT)/kbe/src/build/common.mak