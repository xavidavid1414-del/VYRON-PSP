TARGET := vyron_psp
BUILD := build
SOURCES := source
DATA := data
INCLUDES := include

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS := -g -Wall -Wextra -O2 $(ARCH)
CXXFLAGS := $(CFLAGS)

LDFLAGS :=

LIBS := -lctru

include $(DEVKITPRO)/libctru/3ds_rules
