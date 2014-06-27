BASE := ..

include $(BASE)/common.mk

OBJECTS := spec_lib.o specanalysis.o

all: $(OBJECTS)

-include $(OBJECTS:%=.%.d)

CPPFLAGS += -I$(BASE) -I$(BASE)/include -I. -O3


specanalysis.o: specanalysis.cc
	$(CXX) -MMD -MF .$@.d -c -fPIC -o $@ $< $(CPPFLAGS)

# Non-model rules
spec_lib.o: spec_lib.c
	$(CC) -MMD -MF .$@.d -c -fPIC -o $@ $< $(CPPFLAGS) -std=c99

PHONY += clean
clean::
	rm -f *.o .*.d

.PHONY: $(PHONY)
