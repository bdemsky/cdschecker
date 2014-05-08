BASE := ..

include $(BASE)/common.mk
OBJECTS := scfence.o

CPPFLAGS += -I$(BASE) -I$(BASE)/include -I. -O3

all: $(OBJECTS)

PHONY += all 

scfence.o: scfence.cc scfence.h
	$(CXX) -c -fPIC -o $@ $< $(CPPFLAGS)

PHONY += clean
clean::
	rm -f *.o .*.d

.PHONY: $(PHONY)
