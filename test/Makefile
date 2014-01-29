BASE := ..
SPEC_DIR := $(BASE)/spec-analysis
SPEC_OBJ := $(SPEC_DIR)/spec_lib.o

OBJECTS := $(patsubst %.c, %.o, $(wildcard *.c))
OBJECTS += $(patsubst %.cc, %.o, $(wildcard *.cc))

include $(BASE)/common.mk

DIR := litmus
include $(DIR)/Makefile

DEPS := $(join $(addsuffix ., $(dir $(OBJECTS))), $(addsuffix .d, $(notdir $(OBJECTS))))

CPPFLAGS += -I$(BASE) -I$(BASE)/include -I$(SPEC_DIR)

all: $(OBJECTS)
	$(MAKE) -C ms-queue
	$(MAKE) -C linuxrwlocks
	$(MAKE) -C mcs-lock 
	$(MAKE) -C spsc-bugfix
	$(MAKE) -C read-copy-update
	$(MAKE) -C chase-lev-deque-bugfix
	$(MAKE) -C mpmc-queue
	$(MAKE) -C cliffc-hashtable

-include $(DEPS)

%.o: %.c
	$(CC) -MMD -MF $(@D)/.$(@F).d -o $@ $< $(SPEC_OBJ) $(CPPFLAGS) -std=c99 -L$(BASE) -l$(LIB_NAME)

%.o: %.cc
	$(CXX) -MMD -MF $(@D)/.$(@F).d -o $@ $< $(SPEC_OBJ) $(CPPFLAGS) -L$(BASE) -l$(LIB_NAME)

clean::
	rm -f $(OBJECTS) $(DEPS)
	$(MAKE) -C ms-queue clean
	$(MAKE) -C linuxrwlocks clean
	$(MAKE) -C mcs-lock clean
	$(MAKE) -C spsc-bugfix clean
	$(MAKE) -C read-copy-update clean
	$(MAKE) -C chase-lev-deque-bugfix clean
	$(MAKE) -C mpmc-queue clean
	$(MAKE) -C cliffc-hashtable clean
	
