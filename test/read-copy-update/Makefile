include ../benchmarks.mk

all: rcu.o

rcu.o: rcu.cc
	$(CXX) -o $@ $< $(SPEC_OBJ) $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f rcu *.o
