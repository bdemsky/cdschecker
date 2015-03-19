#ifndef _PATCH_H
#define _PATCH_H

#include "inference.h"

extern const char* get_mo_str(memory_order order);

class PatchUnit {
	private:
	const ModelAction *act;
	memory_order mo;

	public:
	PatchUnit(const ModelAction *act, memory_order mo) {
		this->act= act;
		this->mo = mo;
	}

	const ModelAction* getAct() {
		return act;
	}

	memory_order getMO() {
		return mo;
	}

	SNAPSHOTALLOC
};

class Patch {
	private:
	SnapVector<PatchUnit*> *units;

	public:
	Patch(const ModelAction *act, memory_order mo) {
		PatchUnit *unit = new PatchUnit(act, mo);
		units = new SnapVector<PatchUnit*>;
		units->push_back(unit);
	}

	Patch(const ModelAction *act1, memory_order mo1, const ModelAction *act2, memory_order mo2) {
		units = new SnapVector<PatchUnit*>;
		PatchUnit *unit = new PatchUnit(act1, mo1);
		units->push_back(unit);
		unit = new PatchUnit(act2, mo2);
		units->push_back(unit);
	}

	Patch() {
		units = new SnapVector<PatchUnit*>;
	}

	/** return value:
	  * 0 -> mo1 == mo2;
	  * 1 -> mo1 stronger than mo2;
	  * -1 -> mo1 weaker than mo2;
	  * 2 -> mo1 & mo2 are uncomparable.
	 */
	static int compareMemoryOrder(memory_order mo1, memory_order mo2) {
		if (mo1 == mo2)
			return 0;
		if (mo1 == memory_order_relaxed)
			return -1;
		if (mo1 == memory_order_acquire) {
			if (mo2 == memory_order_relaxed)
				return 1;
			if (mo2 == memory_order_release)
				return 2;
			return -1;
		}
		if (mo1 == memory_order_release) {
			if (mo2 == memory_order_relaxed)
				return 1;
			if (mo2 == memory_order_acquire)
				return 2;
			return -1;
		}
		if (mo1 == memory_order_acq_rel) {
			if (mo2 == memory_order_seq_cst)
				return -1;
			else
				return 1;
		}
		// mo1 now must be SC and mo2 can't be SC
		return 1;
	}

	bool isApplicable() {
		for (unsigned i = 0; i < units->size(); i++) {
			PatchUnit *u = (*units)[i];
			memory_order wildcard = u->getAct()->get_original_mo();
			if (is_wildcard(wildcard))
				continue;
			int compVal = compareMemoryOrder(wildcard, u->getMO());
			if (compVal == 2 || compVal == -1)
				return false;
		}
		return true;
	}

	void addPatchUnit(const ModelAction *act, memory_order mo) {
		PatchUnit *unit = new PatchUnit(act, mo);
		units->push_back(unit);
	}

	int getSize() {
		return units->size();
	}

	PatchUnit* get(int i) {
		return (*units)[i];
	}

	void print() {
		for (int i = 0; i < units->size(); i++) {
			PatchUnit *u = (*units)[i];
			model_print("wildcard %d -> %s\n",
				get_wildcard_id_zero(u->getAct()->get_original_mo()),
				get_mo_str(u->getMO()));
		}
	}

	SNAPSHOTALLOC
};

#endif
