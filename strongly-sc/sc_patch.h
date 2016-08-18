#ifndef _SC_PATCH_H
#define _SC_PATCH_H

#include "action.h"
#include "wildcard.h"
#include "stl-model.h"

class SCPatchUnit;
class SCPatch;

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

	SNAPSHOTALLOC
};

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

	SCPatchUnit* get(int i);

	void print();

	SNAPSHOTALLOC
};

#endif
