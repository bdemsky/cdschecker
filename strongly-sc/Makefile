#BASE := ..

STRONGSC_OBJS := $(STRONGSC_DIR)/strongsc.o \
    $(STRONGSC_DIR)/scgraph.o \
    $(STRONGSC_DIR)/mo_assignment.o \
    $(STRONGSC_DIR)/sc_patch.o \
    $(STRONGSC_DIR)/assign_list.o

$(STRONGSC_OBJS): $(STRONGSC_DIR)/%.o : $(STRONGSC_DIR)/%.cc $(STRONGSC_DIR)/%.h
	$(CXX) -MMD -MF $(STRONGSC_DIR)/.$(notdir $@).d -fPIC -c $< $(CPPFLAGS) -o $@

OBJECTS += $(STRONGSC_OBJS)
