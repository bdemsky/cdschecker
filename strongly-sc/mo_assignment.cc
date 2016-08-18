#include "wildcard.h"
#include "action.h"
#include "execution.h"
#include "threads-model.h"
#include "mo_assignment.h"
#include "mydebug.h"

static const char * get_mo_str(memory_order order) {
	switch (order) {
		case std::memory_order_relaxed: return "relaxed";
		case std::memory_order_acquire: return "acquire";
		case std::memory_order_release: return "release";
		case std::memory_order_acq_rel: return "acq_rel";
		case std::memory_order_seq_cst: return "seq_cst";
		default: 
			//model_print("Weird memory order, a bug or something!\n");
			//model_print("memory_order: %d\n", order);
			return "unknown";
	}
}


void MOAssignment::resize(int newsize) {
	ASSERT (newsize > size && newsize <= MAX_WILDCARD_NUM);
	memory_order *newOrders = (memory_order *) model_malloc((newsize + 1) * sizeof(memory_order*));
	int i;
	for (i = 0; i <= size; i++)
		newOrders[i] = orders[i];
	for (; i <= newsize; i++)
		newOrders[i] = WILDCARD_NONEXIST;
	model_free(orders);
	size = newsize;
	orders = newOrders;
}

MOAssignment::MOAssignment() {
	orders = (memory_order *) model_malloc((4 + 1) * sizeof(memory_order*));
	size = 4;
	for (int i = 0; i <= size; i++)
		orders[i] = WILDCARD_NONEXIST;
}

MOAssignment::MOAssignment(MOAssignment *assign) {
	ASSERT (assign->size > 0 && assign->size <= MAX_WILDCARD_NUM);
	orders = (memory_order *) model_malloc((assign->size + 1) * sizeof(memory_order*));
	this->size = assign->size;
	for (int i = 0; i <= size; i++)
		orders[i] = assign->orders[i];
}

MOAssignment::~MOAssignment() {
    model_free(orders);
}


bool MOAssignment::hasSatisfied(SCPatch *p) {
    for (int i = 0; i < p->getSize(); i++) {
        SCPatchUnit *u = p->get(i);
        const ModelAction *act = u->getAct();
        memory_order mo = u->getMO();
        switch (mo) {
            case memory_order_release:
                if (!isRelease(act))
                    return false;
                break;
            case memory_order_acquire:
                if (!isAcquire(act))
                    return false;
                break;
            case memory_order_seq_cst:
                if (!isSC(act))
                    return false;
                break;
            default:
                ASSERT (false);
                break;
        }
    }
    return true;
}

bool MOAssignment::apply(SCPatch *p) {
    for (int i = 0; i < p->getSize(); i++) {
        SCPatchUnit *u = p->get(i);
        const ModelAction *act = u->getAct();
        memory_order mo = u->getMO();
        switch (mo) {
            case memory_order_release:
                if (!imposeRelease(act))
                    return false;
                break;
            case memory_order_acquire:
                if (!imposeAcquire(act))
                    return false;
                break;
            case memory_order_seq_cst:
                if (!imposeSC(act))
                    return false;
                break;
            default:
                ASSERT (false);
                break;
        }
    }
    return true;
}


// Check whether specific semantics (wildcard) is on an action
bool MOAssignment::isRelease(const ModelAction *act) {
    memory_order mo = act->get_original_mo();
    if (is_wildcard(mo)) {
        int wildcard = get_wildcard_id(mo);
        memory_order curMO = (*this)[wildcard];
        return curMO == memory_order_release || 
            curMO == memory_order_acq_rel ||
            curMO == memory_order_seq_cst;
    } else {
        return act->is_release();
    }
}

bool MOAssignment::isAcquire(const ModelAction *act) {
    memory_order mo = act->get_original_mo();
    if (is_wildcard(mo)) {
        int wildcard = get_wildcard_id(mo);
        memory_order curMO = (*this)[wildcard];
        return curMO == memory_order_acquire || 
            curMO == memory_order_acq_rel ||
            curMO == memory_order_seq_cst;
    } else {
        return act->is_acquire();
    }
}

bool MOAssignment::isSC(const ModelAction *act) {
    memory_order mo = act->get_original_mo();
    if (is_wildcard(mo)) {
        int wildcard = get_wildcard_id(mo);
        memory_order curMO = (*this)[wildcard];
        return curMO == memory_order_seq_cst;
    } else {
        return act->is_acquire();
    }
}


// Impose specific semantics on an action
bool MOAssignment::imposeRelease(const ModelAction *act) {
    memory_order mo = act->get_original_mo();
    if (is_wildcard(mo)) {
        int wildcard = get_wildcard_id(mo);
        memory_order curMO = (*this)[wildcard];
        if (curMO == memory_order_relaxed) {
            DPRINT("wildcard %d: relaxed -> release\n", wildcard);
            (*this)[wildcard] = memory_order_release;
        } else if (curMO == memory_order_acquire) {
            DPRINT("wildcard %d: acquire -> acq_rel\n", wildcard);
            (*this)[wildcard] = memory_order_acq_rel;
        }
        return true;
    } else {
        if (act->is_release())
            return true;
        else {
            model_print("**** The following action isn't wildcard (need release) ****\n");
            act->print();
            return false;
        }
    }
}

bool MOAssignment::imposeAcquire(const ModelAction *act) {
    memory_order mo = act->get_original_mo();
    if (is_wildcard(mo)) {
        int wildcard = get_wildcard_id(mo);
        memory_order curMO = (*this)[wildcard];
        if (curMO == memory_order_relaxed) {
            DPRINT("wildcard %d: relaxed -> acquire\n", wildcard);
            (*this)[wildcard] = memory_order_acquire;
        } else if (curMO == memory_order_release) {
            DPRINT("wildcard %d: release -> acq_rel\n", wildcard);
            (*this)[wildcard] = memory_order_acq_rel;
        }
        return true;
    } else {
        if (act->is_acquire())
            return true;
        else {
            model_print("**** The following action isn't wildcard (need acquire) ****\n");
            act->print();
            return false;
        }
    }
}

bool MOAssignment::imposeSC(const ModelAction *act) {
    memory_order mo = act->get_original_mo();
    if (is_wildcard(mo)) {
        int wildcard = get_wildcard_id(mo);
        memory_order curMO = (*this)[wildcard];
        if (curMO != memory_order_seq_cst) {
            DPRINT("wildcard %d: %s -> seq_cst\n", wildcard, get_mo_str(curMO));
            (*this)[wildcard] = memory_order_seq_cst;
        }
        return true;
    } else {
        if (act->is_seqcst())
            return true;
        else {
            model_print("**** The following action isn't wildcard (need seqcst) ****\n");
            act->print();
            return false;
        }
    }
    return true;
}


/** return value:
  * 0 -> mo1 == mo2;
  * 1 -> mo1 stronger than mo2;
  * -1 -> mo1 weaker than mo2;
  * 2 (i.e. ASSIGNMENT_INCOMPARABLE(x)) -> mo1 & mo2 are uncomparable.
*/
int MOAssignment::compareMemoryOrder(memory_order mo1, memory_order mo2) {
	if (mo1 == WILDCARD_NONEXIST)
		mo1 = memory_order_relaxed;
	if (mo2 == WILDCARD_NONEXIST)
		mo2 = memory_order_relaxed;
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



memory_order& MOAssignment::operator[](int idx) {
    ASSERT (idx > 0 && idx <= MAX_WILDCARD_NUM);
	if (idx > 0 && idx <= size)
		return orders[idx];
	else {
		resize(idx);
		orders[idx] = WILDCARD_NONEXIST;
		return orders[idx];
	}
}


/** @Return:
    1 -> 'this is stronger than assign';
    -1 -> 'this is weaker than assign'
    0 -> 'this == assignment'
    ASSIGNMENT_INCOMPARABLE(x) -> true means incomparable
 */
int MOAssignment::compareTo(const MOAssignment *assign) const {
	int result = size == assign->size ? 0 : (size > assign->size) ? 1 : -1;
	int smallerSize = size > assign->size ? assign->size : size;
	int subResult;

	for (int i = 0; i <= smallerSize; i++) {
		int mo1 = orders[i],
			mo2 = assign->orders[i];
		if ((mo1 == memory_order_acquire && mo2 == memory_order_release) ||
			(mo1 == memory_order_release && mo2 == memory_order_acquire)) {
			// Incomparable
			return -2;
		} else {
			if ((mo1 == WILDCARD_NONEXIST && mo2 != WILDCARD_NONEXIST)
				|| (mo1 == WILDCARD_NONEXIST && mo2 == memory_order_relaxed)
				|| (mo1 == memory_order_relaxed && mo2 == WILDCARD_NONEXIST)
				)
				subResult = 1;
			else if (mo1 != WILDCARD_NONEXIST && mo2 == WILDCARD_NONEXIST)
				subResult = -1;
			else
				subResult = mo1 > mo2 ? 1 : (mo1 == mo2) ? 0 : -1;

			if ((subResult > 0 && result < 0) || (subResult < 0 && result > 0)) {
				return -2;
			}
			if (subResult != 0)
				result = subResult;
		}
	}
	return result;
}

unsigned long MOAssignment::getHash() {
	unsigned long hash = 0;
	for (int i = 1; i <= size; i++) {
		memory_order mo = orders[i];
		if (mo == WILDCARD_NONEXIST) {
			mo = memory_order_relaxed;
		}
		hash *= 37;
		hash += (mo + 4096);
	}
	return hash;
}


void MOAssignment::print(bool hasHash) {
	ASSERT(size > 0 && size <= MAX_WILDCARD_NUM);
	if (hasHash)
		model_print("****  Hash: %lu  ****\n", getHash());
	for (int i = 1; i <= size; i++) {
		memory_order mo = orders[i];
		if (mo != WILDCARD_NONEXIST) {
			// Print the wildcard inference result
			model_print("wildcard %d -> memory_order_%s\n", i, get_mo_str(mo));
		}
	}
}


