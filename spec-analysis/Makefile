#BASE := ..

#include $(BASE)/common.mk

#OBJECTS := spec_lib.o specanalysis.o

#all: $(OBJECTS)

#-include $(OBJECTS:%=.%.d)

#CPPFLAGS += -I$(BASE) -I$(BASE)/include -I. -O3


$(SPEC_PLUGIN): $(SPEC_DIR)/specanalysis.cc
	$(CXX) -MMD -MF $(SPEC_DIR)/.$(notdir $@).d -c -fPIC -o $@ $< $(CPPFLAGS)

# Non-model rules
$(SPEC_LIB): $(SPEC_DIR)/spec_lib.c
	$(CC) -MMD -MF $(SPEC_DIR)/.$(notdir $@).d -c -fPIC -o $@ $< $(CPPFLAGS) -std=c99

PHONY += clean
clean:
	rm -f *.o .*.d

.PHONY: $(PHONY)
