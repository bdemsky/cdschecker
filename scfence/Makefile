#BASE := ..

#include $(BASE)/common.mk
#OBJECTS := scfence.o

#all: $(OBJECTS)

#-include $(OBJECTS:%=.%.d)

#CPPFLAGS += -I$(BASE) -I$(BASE)/include -I. -g


#PHONY += all 

scgen.o : $(SCFENCE_DIR)/scgen.c $(SCFENCE_DIR)/scgen.h
	$(CXX) -MMD -MF $(SCFENCE_DIR)/.$(notdir $<).d -c -fPIC -o $@ $< $(CPPFLAGS)

$(SCFENCE_PLUGIN): $(SCFENCE_DIR)/scfence.cc $(SCFENCE_DIR)/scgen.o
	$(CXX) -MMD -MF $(SCFENCE_DIR)/.$(notdir $<).d -c -fPIC -o $@ $< $(CPPFLAGS)

#PHONY += clean
clean:
	rm -f *.o .*.d

#.PHONY: $(PHONY)
