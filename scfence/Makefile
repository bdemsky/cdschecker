#BASE := ..

$(SCFENCE_PLUGIN): $(SCFENCE_DIR)/scfence.cc $(SCFENCE_DIR)/scgen.o
	$(CXX) -MMD -MF $(SCFENCE_DIR)/.$(notdir $<).d -c -fPIC -o $@ $< $(CPPFLAGS)

scgen.o : $(SCFENCE_DIR)/scgen.c $(SCFENCE_DIR)/scgen.h
	$(CXX) -MMD -MF $(SCFENCE_DIR)/.$(notdir $<).d -c -fPIC -o $@ $< $(CPPFLAGS)


PHONY += clean

clean:
	rm -f *.o .*.d

.PHONY: $(PHONY)
