#ifndef _INFERENCE_H
#define _INFERENCE_H

#include "fence_common.h"
#include "wildcard.h"

/** Forward declaration */
class Inference;
class InferenceList;
class PatchUnit;
class Patch;

static const char* get_mo_str(memory_order order) {
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

class Inference {
	private:
	memory_order *orders;
	int size;

	/** Whether this inference will lead to a buggy execution */
	bool buggy;

	/** Whether this inference has been explored */
	bool explored;

	/** Whether this inference is the leaf node in the inference lattice, see
	 * inferset.h for more details */
	bool leaf;

	/** When this inference will have buggy executions, this indicates whether
	 * it has any fixes. */
	bool hasFixes;

	void resize(int newsize) {
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
	int strengthen(const ModelAction *act, memory_order mo) {
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

	public:
	Inference() {
		orders = (memory_order *) model_malloc((4 + 1) * sizeof(memory_order*));
		size = 4;
		for (int i = 0; i <= size; i++)
			orders[i] = WILDCARD_NONEXIST;
		buggy = false;
		hasFixes = false;
		leaf = false;
		explored = false;
	}

	Inference(Inference *infer) {
		ASSERT (infer->size > 0 && infer->size <= MAX_WILDCARD_NUM);
		orders = (memory_order *) model_malloc((infer->size + 1) * sizeof(memory_order*));
		this->size = infer->size;
		for (int i = 0; i <= size; i++)
			orders[i] = infer->orders[i];
		buggy = false;
		hasFixes = false;
		leaf = false;
		explored = false;
	}

	int getSize() {
		return size;
	}

	memory_order &operator[](int idx) {
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
	int strengthen(const ModelAction *act, memory_order mo, bool &canUpdate, bool &hasUpdated) {
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
	int compareTo(const Inference *infer) const {
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

	void setHasFixes(bool val) {
		hasFixes = val;
	}

	bool getHasFixes() {
		return hasFixes;
	}

	void setBuggy(bool val) {
		buggy = val;
	}

	bool getBuggy() {
		return buggy;
	}
	
	void setExplored(bool val) {
		explored = val;
	}

	bool isExplored() {
		return explored;
	}

	void setLeaf(bool val) {
		leaf = val;
	}

	bool isLeaf() {
		return leaf;
	}

	unsigned long getHash() {
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
	
	void print() {
		ASSERT(size > 0 && size <= MAX_WILDCARD_NUM);
		for (int i = 1; i <= size; i++) {
	        memory_order mo = orders[i];
			if (mo != WILDCARD_NONEXIST) {
				// Print the wildcard inference result
				FENCE_PRINT("wildcard %d -> memory_order_%s\n", i, get_mo_str(mo));
			}
		}
		FENCE_PRINT("Hash: %lu\n", getHash());
	}

	~Inference() {
		model_free(orders);
	}

	MEMALLOC
};

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
	static bool compareMemoryOrder(memory_order mo1, memory_order mo2) {
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

/** This class represents that the list of inferences that can fix the problem
 */
class InferenceList {
	private:
	ModelList<Inference*> *list;

	public:
	InferenceList() {
		list = new ModelList<Inference*>;
	}

	int getSize() {
		return list->size();
	}
	
	void pop_back() {
		list->pop_back();
	}

	Inference* back() {
		return list->back();
	}

	/** We should not call this function too often because we want a nicer
	 *  abstraction of the list of inferences. So far, it will only be called in
	 *  the functions in InferenceSet */
	ModelList<Inference*>* getList() {
		return list;
	}
	
	void push_back(Inference *infer) {
		list->push_back(infer);
	}

	bool applyPatch(Inference *curInfer, Inference *newInfer, Patch *patch) {
		bool canUpdate = true,
			hasUpdated = false,
			updateState = false;
		for (int i = 0; i < patch->getSize(); i++) {
			canUpdate = true;
			hasUpdated = false;
			PatchUnit *unit = patch->get(i);
			newInfer->strengthen(unit->getAct(), unit->getMO(), canUpdate, hasUpdated);
			if (!canUpdate) {
				// This is not a feasible patch, bail
				break;
			} else if (hasUpdated) {
				updateState = true;
			}
		}
		if (updateState) {
			model_print("New infer:\n");
			newInfer->print();
			return true;
		} else {
			return false;
		}
	}

	void applyPatch(Inference *curInfer, Patch* patch) {
		if (list->empty()) {
			Inference *newInfer = new Inference(curInfer);
			if (!applyPatch(curInfer, newInfer, patch)) {
				delete newInfer;
			} else {
				list->push_back(newInfer);
			}
		} else {
			ModelList<Inference*> *newList = new ModelList<Inference*>;
			for (ModelList<Inference*>::iterator it = list->begin(); it !=
				list->end(); it++) {
				Inference *oldInfer = *it;
				Inference *newInfer = new Inference(oldInfer);
				if (!applyPatch(curInfer, newInfer, patch)) {
					delete newInfer;
				} else {
					newList->push_back(newInfer);
				}
			}
			// Clean the old list
			for (ModelList<Inference*>::iterator it = list->begin(); it !=
				list->end(); it++) {
				delete *it;
			}
			delete list;
			list = newList;
		}	
	}

	void applyPatch(Inference *curInfer, SnapVector<Patch*> *patches) {
		if (list->empty()) {
			for (unsigned i = 0; i < patches->size(); i++) {
				Inference *newInfer = new Inference(curInfer);
				Patch *patch = (*patches)[i];
				if (!applyPatch(curInfer, newInfer, patch)) {
					delete newInfer;
				} else {
					list->push_back(newInfer);
				}
			}
		} else {
			ModelList<Inference*> *newList = new ModelList<Inference*>;
			for (ModelList<Inference*>::iterator it = list->begin(); it !=
				list->end(); it++) {
				Inference *oldInfer = *it;
				for (unsigned i = 0; i < patches->size(); i++) {
					Inference *newInfer = new Inference(oldInfer);
					Patch *patch = (*patches)[i];
					if (!applyPatch(curInfer, newInfer, patch)) {
						delete newInfer;
					} else {
						newList->push_back(newInfer);
					}
				}
			}
			// Clean the old list
			for (ModelList<Inference*>::iterator it = list->begin(); it !=
				list->end(); it++) {
				delete *it;
			}
			delete list;
			list = newList;
		}
	}
	
	/** Append another list to this list */
	bool append(InferenceList *inferList) {
		if (!inferList)
			return false;
		ModelList<Inference*> *l = inferList->list;
		list->insert(list->end(), l->begin(), l->end());
		return true;
	}

	/** Only choose the weakest existing candidates & they must be stronger than the
	 * current inference */
	void pruneCandidates(Inference *curInfer) {
		ModelList<Inference*> *newCandidates = new ModelList<Inference*>(),
			*candidates = list;

		ModelList<Inference*>::iterator it1, it2;
		int compVal;
		for (it1 = candidates->begin(); it1 != candidates->end(); it1++) {
			Inference *cand = *it1;
			compVal = cand->compareTo(curInfer);
			if (compVal == 0) {
				// If as strong as curInfer, bail
				delete cand;
				continue;
			}
			// Check if the cand is any stronger than those in the newCandidates
			for (it2 = newCandidates->begin(); it2 != newCandidates->end(); it2++) {
				Inference *addedInfer = *it2;
				compVal = addedInfer->compareTo(cand);
				if (compVal == 0 || compVal == 1) { // Added inference is stronger
					delete addedInfer;
					it2 = newCandidates->erase(it2);
					it2--;
				}
			}
			// Now push the cand to the list
			newCandidates->push_back(cand);
		}
		delete candidates;
		list = newCandidates;
	}
	
	void clearAll() {
		clearAll(list);
	}

	void clearList() {
		delete list;
	}

	static void clearAll(ModelList<Inference*> *l) {
		for (ModelList<Inference*>::iterator it = l->begin(); it !=
			l->end(); it++) {
			Inference *infer = *it;
			delete infer;
		}
		delete l;
	}

	static void clearAll(InferenceList *inferList) {
		clearAll(inferList->list);
	}
	
	void print(ModelList<Inference*> *inferList, const char *msg) {
		for (ModelList<Inference*>::iterator it = inferList->begin(); it !=
			inferList->end(); it++) {
			int idx = distance(inferList->begin(), it);
			Inference *infer = *it;
			model_print("%s %d:\n", msg, idx);
			infer->print();
			model_print("\n");
		}
	}

	void print() {
		print(list, "Inference");
	}

	void print(const char *msg) {
		print(list, msg);
	}

	MEMALLOC

};


#endif
