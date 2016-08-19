#ifndef _SC_PATCH_H
#define _SC_PATCH_H

#include "action.h"
#include "wildcard.h"
#include "stl-model.h"

class SCPatchUnit;
class SCPatch;

typedef SnapList<SCPatch*> patch_list_t;

/**
    This represents a unit of a patch. A patch unit has an memory operation
    (act) and a corresponding memory order (which can only be
    release/acquire/seq_cst). It means it requires a speicific memory operation
    to have the corresponding semantics.
*/
class SCPatchUnit {
	private:
	const ModelAction *act;
	memory_order mo;

	public:
	SCPatchUnit(const ModelAction *act, memory_order mo) {
		this->act= act;
		this->mo = mo;
	}

	const ModelAction* getAct() {
		return act;
	}

	memory_order getMO() {
		return mo;
	}

	void setMO(memory_order mo) {
		this->mo = mo;
	}

	SNAPSHOTALLOC
};

/**
    This represents a patch (which consists of a list of patch units).
    Generally, it represents the list of memory operations that require special
    stronger (than relaxed) semantics for a speicific strong SC rule.
*/
class SCPatch {
	private:
	SnapVector<SCPatchUnit*> *units;

	public:
	SCPatch(const ModelAction *act, memory_order mo);

	SCPatch(const ModelAction *act1, memory_order mo1, const ModelAction *act2,
		memory_order mo2);

	SCPatch();

	void addPatchUnit(const ModelAction *act, memory_order mo);

	int getSize();

	unsigned long getHash();

    bool equals(SCPatch *o);

	SCPatchUnit* get(int i);

	void print();

    // An optimized way to add a patch to the list to avoid duplication
    static bool addPatchToList(patch_list_t *patches, SCPatch *p);

	SNAPSHOTALLOC
};

#endif
