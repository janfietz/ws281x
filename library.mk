
WS281X_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
CSRC += $(wildcard $(WS281X_DIR)/ws281x.c)
EXTRAINCDIRS += $(WS281X_DIR)

CFLAGS += -DHAS_WS281X
