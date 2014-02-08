include ../benchmarks.mk

all: linuxrwlocks.o

linuxrwlocks.o :linuxrwlocks.c 
	$(CXX) -o $@ $< $(SPEC_OBJ) $(CXXFLAGS) -std=c++0x $(LDFLAGS)

clean:
	rm -f *.o
