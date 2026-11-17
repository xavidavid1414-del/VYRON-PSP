.SUFFIXES:

TARGET := vyron_psp
BUILD := build

SOURCES := source \
           source/cpu \
           source/memory \
           source/loader

DATA := data
INCLUDES := include

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS := -g -Wall -Wextra -O2 -mword-relocations \
          -ffunction-sections -fdata-sections $(ARCH)

CXXFLAGS := $(CFLAGS)
LDFLAGS := -specs=3dsx.specs

LIBS := -lctru
LIBDIRS := $(DEVKITPRO)/libctru

include $(DEVKITARM)/3ds_rules
