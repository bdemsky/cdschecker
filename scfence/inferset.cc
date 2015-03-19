#include "patch.h"
#include "inference.h"
#include "inferset.h"

InferenceSet::InferenceSet() {
	initialInfer = new Inference;
	discoveredSet = new InferenceList;
	results = new InferenceList;
	candidates = new InferenceList;
}

void InferenceSet::setInitialInfer(Inference *initial) {
	this->initialInfer = initial;
}

Inference* InferenceSet::getInitialInfer() {
	return this->initialInfer;
}

/** Print the result of inferences  */
void InferenceSet::printResults() {
	results->print("Result");
	stat.print();
}

/** Print candidates of inferences */
void InferenceSet::printCandidates() {
	candidates->print("Candidate");
}

/** When we finish model checking or cannot further strenghen with the
 * inference, we commit the inference to be explored; if it is feasible, we
 * put it in the result list */
void InferenceSet::commitInference(Inference *infer, bool feasible) {
	ASSERT (infer);
	
	infer->setExplored(true);
	FENCE_PRINT("Explored %lu\n", infer->getHash());
	if (feasible) {
		addResult(infer);
	}
}

int InferenceSet::getCandidatesSize() {
	return candidates->getSize();
}

int InferenceSet::getResultsSize() {
	return results->getSize();
}

/** Be careful that if the candidate is not added, it will be deleted in this
 *  function. Therefore, caller of this function should just delete the list when
 *  finishing calling this function. */
bool InferenceSet::addCandidates(Inference *curInfer, InferenceList *inferList) {
	if (!inferList)
		return false;
	// First prune the list of inference
	inferList->pruneCandidates(curInfer);

	ModelList<Inference*> *cands = inferList->getList();

	// For the purpose of debugging, record all those inferences added here
	InferenceList *addedCandidates = new InferenceList;
	FENCE_PRINT("List size: %ld.\n", cands->size());
	bool added = false;

	/******** addCurInference ********/
	// Add the current inference to the set, but specifially it marks it as
	// non-leaf node so that when it gets popped, we just need to commit it as
	// explored
	addCurInference(curInfer);

	ModelList<Inference*>::iterator it;
	for (it = cands->begin(); it != cands->end(); it++) {
		Inference *candidate = *it;
		bool tmpAdded = false;
		/******** addInference ********/
		tmpAdded = addInference(candidate);
		if (tmpAdded) {
			added = true;
			it = cands->erase(it);
			it--;
			addedCandidates->push_back(candidate); 
		}
	}

	// For debugging, print the list of candidates for this iteration
	FENCE_PRINT("Current inference:\n");
	curInfer->print();
	FENCE_PRINT("\n");
	FENCE_PRINT("The added inferences:\n");
	addedCandidates->print("Candidates");
	FENCE_PRINT("\n");
	
	// Clean up the candidates
	inferList->clearList();
	FENCE_PRINT("potential results size: %d.\n", candidates->getSize());
	return added;
}


/** Check if we have stronger or equal inferences in the current result
 * list; if we do, we remove them and add the passed-in parameter infer */
 void InferenceSet::addResult(Inference *infer) {
	ModelList<Inference*> *list = results->getList();
	for (ModelList<Inference*>::iterator it = list->begin(); it !=
		list->end(); it++) {
		Inference *existResult = *it;
		int compVal = existResult->compareTo(infer);
		if (compVal == 0 || compVal == 1) {
			// The existing result is equal or stronger, remove it
			FENCE_PRINT("We are dumping the follwing inference because it's either too weak or the same:\n");
			existResult->print();
			FENCE_PRINT("\n");
			it = list->erase(it);
			it--;
		}
	}
	list->push_back(infer);
 }

/** Get the next available unexplored node; @Return NULL 
 * if we don't have next, meaning that we are done with exploring */
Inference* InferenceSet::getNextInference() {
	Inference *infer = NULL;
	while (candidates->getSize() > 0) {
		infer = candidates->back();
		candidates->pop_back();
		if (!infer->isLeaf()) {
			commitInference(infer, false);
			continue;
		}
		if (hasBeenExplored(infer)) {
			// Finish exploring this node
			// Remove the node from the set 
			FENCE_PRINT("Explored inference:\n");
			infer->print();
			FENCE_PRINT("\n");
			continue;
		} else {
			return infer;
		}
	}
	return NULL;
}

/** Add the current inference to the set before adding fixes to it; in
 * this case, fixes will be added afterwards, and infer should've been
 * discovered */
void InferenceSet::addCurInference(Inference *infer) {
	infer->setLeaf(false);
	candidates->push_back(infer);
}

/** Add one possible node that represents a fix for the current inference;
 * @Return true if the node to add has not been explored yet
 */
bool InferenceSet::addInference(Inference *infer) {
	if (!hasBeenDiscovered(infer)) {
		// We haven't discovered this inference yet

		// Newly added nodes are leaf by default
		infer->setLeaf(true);
		candidates->push_back(infer);
		discoveredSet->push_back(infer);
		FENCE_PRINT("Discovered %lu\n", infer->getHash());
		return true;
	} else {
		stat.notAddedAtFirstPlace++;
		return false;
	}
}

/** Return false if we haven't discovered that inference yet. Basically we
 * search the candidates list */
bool InferenceSet::hasBeenDiscovered(Inference *infer) {
	ModelList<Inference*> *list = discoveredSet->getList();
	for (ModelList<Inference*>::iterator it = list->begin(); it !=
		list->end(); it++) {
		Inference *discoveredInfer = *it;
		// When we already have an equal inferences in the candidates list
		int compVal = discoveredInfer->compareTo(infer);
		if (compVal == 0) {
			FENCE_PRINT("%lu has beend discovered.\n",
				infer->getHash());
			return true;
		}
		// Or the discoveredInfer is explored and infer is strong than it is
		if (compVal == -1 && discoveredInfer->isExplored()) {
			return true;
		}
	}
	return false;
}

/** Return true if we have explored this inference yet. Basically we
 * search the candidates list */
bool InferenceSet::hasBeenExplored(Inference *infer) {
	ModelList<Inference*> *list = discoveredSet->getList();
	for (ModelList<Inference*>::iterator it = list->begin(); it !=
		list->end(); it++) {
		Inference *discoveredInfer = *it;
		if (!discoveredInfer->isExplored())
			continue;
		// When we already have an equal inferences in the candidates list
		int compVal = discoveredInfer->compareTo(infer);
		if (compVal == 0 || compVal == -1) {
			return true;
		}
	}
	return false;
}
