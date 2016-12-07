BENCH := lock

TESTS := main testcase1

all: $(TESTS) 

# Should include it after "all"; otherwise it will only make cdsspec-generated.o 
include ../benchmarks.mk

%.o : %.c 
	$(CC) -fPIC -c -MMD -MF .$@.d -o $@ $< $(CPPFLAGS) $(LDFLAGS)

$(TESTS): % : %.o $(BENCH).o $(CDSSPEC)
	$(CXX) -o $@ $^ $(CPPFLAGS) $(LDFLAGS)

-include .*.d

clean:
	rm -rf $(TESTS) *.o .*.d *.dSYM

cleanse: clean 
	rm -rf *.c *.cc *.h

.PHONY: clean all cleanse
