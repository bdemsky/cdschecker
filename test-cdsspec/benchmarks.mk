# A few common Makefile items

CC = gcc
CXX = g++

UNAME = $(shell uname)

LIB_NAME = model
LIB_SO = lib$(LIB_NAME).so

BASE = ../..
INCLUDE = -I$(BASE) -I$(BASE)/include -I../include -I$(BASE)/spec-analysis/include

# C preprocessor flags
CPPFLAGS += $(INCLUDE) -O3 -g

# C++ compiler flags
CXXFLAGS += $(CPPFLAGS)

# C compiler flags
CFLAGS += $(CPPFLAGS)

# Linker flags
LDFLAGS += -L$(BASE) -l$(LIB_NAME) -rdynamic

# Mac OSX options
ifeq ($(UNAME), Darwin)
MACFLAGS = -D_XOPEN_SOURCE -DMAC
CPPFLAGS += $(MACFLAGS)
CXXFLAGS += $(MACFLAGS)
CFLAGS += $(MACFLAGS)
LDFLAGS += $(MACFLAGS)
endif


CDSSPEC_NAME = cdsspec-generated
CDSSPEC = $(CDSSPEC_NAME).o

$(CDSSPEC): $(CDSSPEC_NAME).cc
	$(CXX) -MMD -MF .$@.d -o $@ -fPIC -c $< $(CXXFLAGS) -I$(BASE)/spec-analysis -std=c++11 $(LDFLAGS)

