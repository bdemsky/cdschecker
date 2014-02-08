include ../benchmarks.mk

TARGET := table.o

all: $(TARGET)
$(TARGET): main.cc cliffc_hashtable.h
	$(CXX) -o $@ $^ $(SPEC_OBJ) $(CXXFLAGS) -std=c++0x $(LDFLAGS)

#all: table
#table: main.cc
#	$(CXX) -o $@ $^ -std=c++0x 

clean:
	rm -f *.o *.d $(TARGET)
