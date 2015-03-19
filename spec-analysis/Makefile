#BASE := ..

SPEC_PLUGIN := $(SPEC_DIR)/specanalysis.o
SPEC_LIB := $(SPEC_DIR)/spec_lib.o

SPEC_DEP := $(SPEC_LIB) $(SPEC_PLUGIN)

OBJECTS += $(SPEC_DEP)

$(SPEC_PLUGIN): $(SPEC_DIR)/specanalysis.cc
	$(CXX) -MMD -MF $(SPEC_DIR)/.$(notdir $@).d -c -fPIC -o $@ $< $(CPPFLAGS)

# Non-model rules
$(SPEC_LIB): $(SPEC_DIR)/spec_lib.c
	$(CC) -MMD -MF $(SPEC_DIR)/.$(notdir $@).d -c -fPIC -o $@ $< $(CPPFLAGS) -std=c99
