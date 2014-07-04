#BASE := ..

#include $(BASE)/common.mk
#OBJECTS := scfence.o

#all: $(OBJECTS)

#-include $(OBJECTS:%=.%.d)

#CPPFLAGS += -I$(BASE) -I$(BASE)/include -I. -g


#PHONY += all 

$(SCFENCE_PLUGIN): $(SCFENCE_DIR)/scfence.cc
	$(CXX) -MMD -MF $(SCFENCE_DIR)/.$(notdir $@).d -c -fPIC -o $@ $< $(CPPFLAGS)

#PHONY += clean
clean:
	rm -f *.o .*.d

#.PHONY: $(PHONY)
