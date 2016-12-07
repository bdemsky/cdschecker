BENCH := mpmc-queue

TESTS := main testcase1 testcase2

all: $(TESTS)

# Should include it after "all"; otherwise it will only make cdsspec-generated.o
include ../benchmarks.mk

%.o : %.cc 
	$(CXX) -fPIC -c -MMD -MF .$@.d -o $@ $< $(CXXFLAGS) $(LDFLAGS)

$(TESTS) : % : %.o $(BENCH).o $(CDSSPEC)
	$(CXX) -MMD -MF .$@.d -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

-include .*.d

clean:
	rm -rf $(TESTS) *.o .*.d *.dSYM

cleanse: clean 
	rm -rf *.c *.cc *.h

.PHONY: clean all cleanse
