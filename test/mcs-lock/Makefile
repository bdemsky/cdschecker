include ../benchmarks.mk

all: mcslock.o backup-mcslock.o

mcslock.o :mcs-lock.cc mcs-lock.h
	$(CXX) -o $@ $< $(SPEC_OBJ) $(CXXFLAGS) $(LDFLAGS)

backup-mcslock.o :backup-mcs-lock.cc backup-mcs-lock.h
	$(CXX) -o $@ $< $(SPEC_OBJ) $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f *.o
