DIRS := ms-queue linuxrwlocks mcs-lock \
	chase-lev-deque-bugfix ticket-lock seqlock read-copy-update \
	concurrent-hashmap spsc-bugfix mpmc-queue

.PHONY: $(DIRS)

all: $(DIRS)

clean: $(DIRS:%=clean-%)
cleanse: $(DIRS:%=cleanse-%)

$(DIRS):
	$(MAKE) -C $@

clean-%:
	-$(MAKE) -C $* clean

cleanse-%: 
	-$(MAKE) -C $* cleanse
