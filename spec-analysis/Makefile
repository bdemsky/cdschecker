BASE := ..

OBJECTS := specanalysis.o spec_lib.o testanalysis.o
include $(BASE)/common.mk

CPPFLAGS += -I$(BASE) -I$(BASE)/include -I. -O0 -g

all: $(OBJECTS)

specanalysis.o: specanalysis.cc specannotation.h
	$(CXX) -c -fPIC -o $@ $< $(CPPFLAGS)
testanalysis.o: testanalysis.cc
	$(CXX) -c -fPIC -o $@ $< $(CPPFLAGS)
#spec_lib.o: spec_lib.c
#	$(CXX) -c -o $@ $< $(CPPFLAGS)

# Non-model rules
spec_lib.o: spec_lib.c spec_lib.h specannotation.h
	$(CC) -c -o $@ $< $(CPPFLAGS) -std=c99

PHONY += clean
clean::
	rm -f *.o .*.d
