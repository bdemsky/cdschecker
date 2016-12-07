include ../benchmarks.mk

TESTNAME = register
all: $(TESTNAME)

CFLAGS := $(CFLAGS) --std=c++11

$(TESTNAME): $(TESTNAME).cc $(TESTNAME).h
	$(CXX) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(TESTNAME) *.o 
