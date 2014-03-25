# A few common Makefile items

CC = gcc
CXX = g++

UNAME = $(shell uname)

LIB_NAME = model
LIB_SO = lib$(LIB_NAME).so

BASE = ../..
SPEC_DIR := $(BASE)/spec-analysis
SPEC_OBJ := $(SPEC_DIR)/spec_lib.o

INCLUDE = -I$(BASE)/include -I$(BASE)/spec-analysis -I$(BASE)

# C preprocessor flags
CPPFLAGS += $(INCLUDE) -g -O3

# C++ compiler flags
CXXFLAGS += $(CPPFLAGS)

# C compiler flags
CFLAGS += $(CPPFLAGS) -std=c99

# Linker flags
LDFLAGS += -L$(BASE) -l$(LIB_NAME) -rdynamic

# Mac OSX options
ifeq ($(UNAME), Darwin)
MACFLAGS = -D_XOPEN_SOURCE -DMAC
CPPFLAGS += $(MACFLAGS)
CXXFLAGS += $(MACFLAGS)
CFLAGS += $(MACFLAGS)
CFLAGS += -std=c99
LDFLAGS += $(MACFLAGS)
endif
