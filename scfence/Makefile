#BASE := ..

$(SCFENCE_PLUGIN): $(SCFENCE_DIR)/scfence.cc
	$(CXX) -MMD -MF $(SCFENCE_DIR)/.$(notdir $<).d -c -fPIC -o $@ $< $(CPPFLAGS)

PHONY += clean

clean:
	rm -f *.o .*.d

.PHONY: $(PHONY)
