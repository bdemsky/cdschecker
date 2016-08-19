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
    memory_order originalMO = act->get_original_mo();
    if (!is_wildcard(originalMO)) {
        // Simply push back for non-wildcards
        SCPatchUnit *unit = new SCPatchUnit(act, mo);
	    units->push_back(unit);
        return;
    } else {
        int wildcard = get_wildcard_id(originalMO);
        SnapVector<SCPatchUnit*>::iterator i = units->begin();
        for (; i != units->end(); i++) {
		    SCPatchUnit *u = *i;
            memory_order existingMO = u->getAct()->get_original_mo();
            if (!is_wildcard(existingMO)) {
	            SCPatchUnit *unit = new SCPatchUnit(act, mo);
                units->insert(i, unit);
                return;
            } else {
                int existingWildcard = get_wildcard_id(existingMO);
                if (existingWildcard < wildcard)
                    continue;
                else if (existingWildcard == wildcard) {
                    // Simply add up the mo to the existing
                    memory_order existingMO = u->getMO();
                    if (existingMO == memory_order_acquire) {
                        if (mo == memory_order_release) {
                            u->setMO(memory_order_acq_rel);
                        } else if (mo == memory_order_seq_cst) {
                            u->setMO(memory_order_seq_cst);
                        }
                    } else if (existingMO == memory_order_release) {
                        if (mo == memory_order_acquire) {
                            u->setMO(memory_order_acq_rel);
                        } else if (mo == memory_order_seq_cst) {
                            u->setMO(memory_order_seq_cst);
                        }
                    }  else if (existingMO == memory_order_acq_rel) {
                        if (mo == memory_order_seq_cst) {
                            u->setMO(memory_order_seq_cst);
                        }
                    }
                    return;
                } else {
                    // The wildcard has not been required to strengthen yet
                    SCPatchUnit *unit = new SCPatchUnit(act, mo);
                    units->insert(i, unit);
                    return;
                }
            }
        }
	    SCPatchUnit *unit = new SCPatchUnit(act, mo);
        units->insert(i, unit);
    }
	SCPatchUnit *unit = new SCPatchUnit(act, mo);
	units->push_back(unit);
}

unsigned long SCPatch::getHash() {
	unsigned long hash = 0;
	for (unsigned i = 0; i < units->size(); i++) {
		SCPatchUnit *u = (*units)[i];
        int seq = u->getAct()->get_seq_number();
        memory_order mo = u->getMO();

		hash *= 37;
		hash += (seq);
		hash *= 37;
		hash += (mo + 4096);
	}
	return hash;
}


bool SCPatch::equals(SCPatch *o) {
    if (o->units->size() != units->size())
        return false;
    for (unsigned i = 0; i < units->size(); i++) {
        SCPatchUnit *u = (*units)[i];
        SCPatchUnit *u1 = (*o->units)[i];
        if (u->getAct()->get_original_mo() != u1->getAct()->get_original_mo() ||
            u->getMO() != u1->getMO())
            return false;
    }
    return true;
}

int SCPatch::getSize() {
	return units->size();
}

SCPatchUnit* SCPatch::get(int i) {
	return (*units)[i];
}

void SCPatch::print() {
    model_print("Patch: ");
    if (units->empty()) {
        model_print("empty\n");
    } else {
        unsigned i = 0;
        SCPatchUnit *u = (*units)[0];
        model_print("(%d->%s)",
            get_wildcard_id_zero(u->getAct()->get_original_mo()),
            get_mo_str(u->getMO()));
        for (i++; i < units->size(); i++) {
            u = (*units)[i];
            model_print(", (%d->%s)",
                get_wildcard_id_zero(u->getAct()->get_original_mo()),
                get_mo_str(u->getMO()));
        }
        model_print("\n");
    }
}


// An optimized way to add a patch to the list to avoid duplication
bool SCPatch::addPatchToList(patch_list_t *patches, SCPatch *p) {
    unsigned long pHash = p->getHash();
    for (patch_list_t::iterator i = patches->begin(); i != patches->end(); i++)
    {
        SCPatch *p0 = *i;
        if (p0->getHash() == pHash) {
            if (p0->equals(p))
                return false;
        }
    }
    patches->push_back(p);
    return true;
}
