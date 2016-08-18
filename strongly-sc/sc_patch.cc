#include "sc_patch.h"
#include "common.h"
#include "strongsc_common.h"

SCPatch::SCPatch(const ModelAction *act, memory_order mo) {
	SCPatchUnit *unit = new SCPatchUnit(act, mo);
	units = new SnapVector<SCPatchUnit*>;
	units->push_back(unit);
}

SCPatch::SCPatch(const ModelAction *act1, memory_order mo1, const ModelAction *act2, memory_order mo2) {
	units = new SnapVector<SCPatchUnit*>;
	SCPatchUnit *unit = new SCPatchUnit(act1, mo1);
	units->push_back(unit);
	unit = new SCPatchUnit(act2, mo2);
	units->push_back(unit);
}

SCPatch::SCPatch() {
	units = new SnapVector<SCPatchUnit*>;
}

void SCPatch::addPatchUnit(const ModelAction *act, memory_order mo) {
	SCPatchUnit *unit = new SCPatchUnit(act, mo);
	units->push_back(unit);
}

int SCPatch::getSize() {
	return units->size();
}

SCPatchUnit* SCPatch::get(int i) {
	return (*units)[i];
}

void SCPatch::print() {
	for (unsigned i = 0; i < units->size(); i++) {
		SCPatchUnit *u = (*units)[i];
		model_print("wildcard %d -> %s\n",
			get_wildcard_id_zero(u->getAct()->get_original_mo()),
			get_mo_str(u->getMO()));
	}
}
