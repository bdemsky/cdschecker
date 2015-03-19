#BASE := ..

DEP := $(SCFENCE_DIR)/scgen.o \
	$(SCFENCE_DIR)/inference.o \
	$(SCFENCE_DIR)/inferset.o \
	$(SCFENCE_DIR)/inferlist.o \
	$(SCFENCE_DIR)/scfence.o

$(DEP): $(SCFENCE_DIR)/%.o : $(SCFENCE_DIR)/%.cc $(SCFENCE_DIR)/%.h
	$(CXX) -MMD -MF $(SCFENCE_DIR)/.$(notdir $@).d -fPIC -c $< $(CPPFLAGS) -o $@

OBJECTS += $(DEP)

PHONY += clean

clean:
	rm -f *.o .*.d

.PHONY: $(PHONY)
