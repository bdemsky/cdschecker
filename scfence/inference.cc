#include "fence_common.h"
#include "wildcard.h"
#include "patch.h"
#include "inferlist.h"
#include "inference.h"

/** Forward declaration */
class PatchUnit;
class Patch;
class Inference;
class InferenceList;

bool isTheInference(Inference *infer) {
	for (int i = 0; i < infer->getSize(); i++) {
		memory_order mo1 = (*infer)[i], mo2;
		if (mo1 == WILDCARD_NONEXIST)
			mo1 = relaxed;
		switch (i) {
			case 3:
				mo2 = acquire;
			break;
			case 11:
				mo2 = release;
			break;
			default:
				mo2 = relaxed;
			break;
		}
		if (mo1 != mo2)
			return false;
	}
	return true;
}

const char* get_mo_str(memory_order order) {
	switch (order) {
		case std::memory_order_relaxed: return "relaxed";
		case std::memory_order_acquire: return "acquire";
		case std::memory_order_release: return "release";
		case std::memory_order_acq_rel: return "acq_rel";
		case std::memory_order_seq_cst: return "seq_cst";
		default: 
			model_print("Weird memory order, a bug or something!\n");
			model_print("memory_order: %d\n", order);
			return "unknown";
	}
}


void Inference::resize(int newsize) {
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

/** Return the state of how we update a specific mo; If we have to make an
 * uncompatible inference or that inference cannot be imposed because it's
 * not a wildcard, it returns -1; if it is a compatible memory order but the
 * current memory order is no weaker than mo, it returns 0; otherwise, it
 * does strengthen the order, and returns 1 */
int Inference::strengthen(const ModelAction *act, memory_order mo) {
	memory_order wildcard = act->get_original_mo();
	int wildcardID = get_wildcard_id_zero(wildcard);
	if (!is_wildcard(wildcard)) {
		FENCE_PRINT("We cannot make this update to %s!\n", get_mo_str(mo));
		ACT_PRINT(act);
		return -1;
	}
	if (wildcardID > size)
		resize(wildcardID);
	ASSERT (is_normal_mo(mo));
	//model_print("wildcardID -> order: %d -> %d\n", wildcardID, orders[wildcardID]);
	ASSERT (is_normal_mo_infer(orders[wildcardID]));
	switch (orders[wildcardID]) {
		case memory_order_seq_cst:
			return 0;
		case memory_order_relaxed:
			if (mo == memory_order_relaxed)
				return 0;
			orders[wildcardID] = mo;
			break;
		case memory_order_acquire:
			if (mo == memory_order_acquire || mo == memory_order_relaxed)
				return 0;
			if (mo == memory_order_release)
				orders[wildcardID] = memory_order_acq_rel;
			else if (mo >= memory_order_acq_rel && mo <=
				memory_order_seq_cst)
				orders[wildcardID] = mo;
			break;
		case memory_order_release:
			if (mo == memory_order_release || mo == memory_order_relaxed)
				return 0;
			if (mo == memory_order_acquire)
				orders[wildcardID] = memory_order_acq_rel;
			else if (mo >= memory_order_acq_rel)
				orders[wildcardID] = mo;
			break;
		case memory_order_acq_rel:
			if (mo == memory_order_seq_cst)
				orders[wildcardID] = mo;
			else
				return 0;
			break;
		default:
			orders[wildcardID] = mo;
			break;
	}
	return 1;
}

Inference::Inference() {
	orders = (memory_order *) model_malloc((4 + 1) * sizeof(memory_order*));
	size = 4;
	for (int i = 0; i <= size; i++)
		orders[i] = WILDCARD_NONEXIST;
	buggy = false;
	hasFixes = false;
	leaf = false;
	explored = false;
	shouldFix = true;
}

Inference::Inference(Inference *infer) {
	ASSERT (infer->size > 0 && infer->size <= MAX_WILDCARD_NUM);
	orders = (memory_order *) model_malloc((infer->size + 1) * sizeof(memory_order*));
	this->size = infer->size;
	for (int i = 0; i <= size; i++)
		orders[i] = infer->orders[i];
	buggy = false;
	hasFixes = false;
	leaf = false;
	explored = false;
	shouldFix = true;
}

/** return value:
  * 0 -> mo1 == mo2;
  * 1 -> mo1 stronger than mo2;
  * -1 -> mo1 weaker than mo2;
  * 2 -> mo1 & mo2 are uncomparable.
 */
int Inference::compareMemoryOrder(memory_order mo1, memory_order mo2) {
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


/** Try to calculate the set of inferences that are weaker than this, but
 *  still stronger than infer */
InferenceList* Inference::getWeakerInferences(Inference *infer) {
	// An array of strengthened wildcards
	SnapVector<int> *strengthened = new SnapVector<int>;
	for (int i = 1; i <= size; i++) {
		memory_order mo1 = orders[i],
			mo2 = (*infer)[i];
		int compVal = compareMemoryOrder(mo1, mo2);
		ASSERT(compVal == 0 || compVal == 1);
		if (compVal == 0) // Same
			continue;
		strengthened->push_back(i);
	}

	// Got the strengthened wildcards, find out weaker inferences
	// First get a volatile copy of this inference
	Inference *volatileInfer = new Inference(this);
	InferenceList *res = new InferenceList;
	getWeakerInferences(res, volatileInfer, infer, strengthened, 0);
	return res;
}

void Inference::getWeakerInferences(InferenceList* list, Inference *infer1,
	Inference *infer2, SnapVector<int> *strengthened, int idx) {
	//int wildcard = (*strengthened)[idx]; // The wildcard
	//memory_order mo1 = (*infer1)[wildcard],
	//	mo2 = (*infer2)[wildcard];
}

memory_order& Inference::operator[](int idx) {
	if (idx > 0 && idx <= size)
		return orders[idx];
	else {
		resize(idx);
		orders[idx] = WILDCARD_NONEXIST;
		return orders[idx];
	}
}

/** A simple overload, which allows caller to pass two boolean refrence, and
 * we will set those two variables indicating whether we can update the
 * order (copatible or not) and have updated the order */
int Inference::strengthen(const ModelAction *act, memory_order mo, bool &canUpdate, bool &hasUpdated) {
	int res = strengthen(act, mo);
	if (res == -1)
		canUpdate = false;
	if (res == 1)
		hasUpdated = true;

	return res;
}

/** @Return:
	1 -> 'this> infer';
	-1 -> 'this < infer'
	0 -> 'this == infer'
	INFERENCE_INCOMPARABLE(x) -> true means incomparable
 */
int Inference::compareTo(const Inference *infer) const {
	int result = size == infer->size ? 0 : (size > infer->size) ? 1 : -1;
	int smallerSize = size > infer->size ? infer->size : size;
	int subResult;

	for (int i = 0; i <= smallerSize; i++) {
		int mo1 = orders[i],
			mo2 = infer->orders[i];
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

unsigned long Inference::getHash() {
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

void Inference::print() {
	ASSERT(size > 0 && size <= MAX_WILDCARD_NUM);
	for (int i = 1; i <= size; i++) {
		memory_order mo = orders[i];
		if (mo != WILDCARD_NONEXIST) {
			// Print the wildcard inference result
			FENCE_PRINT("wildcard %d -> memory_order_%s\n", i, get_mo_str(mo));
		}
	}
	//FENCE_PRINT("Hash: %lu\n", getHash());
}
