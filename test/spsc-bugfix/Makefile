include ../benchmarks.mk

all: spsc-queue.o

spsc-queue.o: spsc-queue.cc queue.h eventcount.h
	$(CXX) -o $@ $< $(SPEC_OBJ) $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f $(TESTNAME) *.o
