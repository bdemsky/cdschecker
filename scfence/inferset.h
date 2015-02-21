#ifndef _INFERSET_H
#define _INFERSET_H

#include "inference.h"
#include "fence_common.h"

typedef struct inference_stat {
	int notAddedAtFirstPlace;

	inference_stat() :
		notAddedAtFirstPlace(0),
	{
	
	}

	void print() {
		model_print("Inference process statistics output:...\n");
		model_print("The number of inference not added at the first place: %d\n",
			notAddedAtFirstPlace);
	}
} inference_stat_t;


/** Essentially, we have to explore a big lattice of inferences, the bottom of
 *  which is the inference that has all relaxed orderings, and the top of which
 *  is the one that has all SC orderings. We define the partial ordering between
 *  inferences as in compareTo() function in the Infereence class. In another
 *  word, we compare ordering parameters one by one as a vector. Theoretically,
 *  we need to explore up to the number of 5^N inferences, where N denotes the
 *  number of wildcards (since we have 5 possible options for each memory order).

 *  We try to reduce the searching space by recording whether an inference has
 *  been discovered or not, and if so, we only need to explore from that
 *  inference for just once. We can use a set to record the inferences to be be
 *  explored, and insert new undiscovered fixes to that set iteratively until it
 *  gets empty.

 *  In detail, we use the InferenceList to represent that set, and use a
 *  LIFO-like actions in pushing and popping inferences. When we add an
 *  inference to the stack, we will set it to leaf or non-leaf node. For the
 *  current inference to be added, it is non-leaf node because it generates
 *  stronger inferences. On the other hands, for those generated inferences, we
 *  set them to be leaf node. So when we pop a leaf node, we know that it is
 *  not just discovered but thoroughly explored. Therefore, when we dicide
 *  whether an inference is discovered, we can first try to look up the
 *  discovered set and also derive those inferences that are stronger the
 *  explored ones to be discovered.

 ********** The main algorithm **********
 Initial:
 	InferenceSet set; // Store the candiates to explore in a set
	Set discoveredSet; // Store the discovered candidates. For each discovered
		// candidate, if it's explored (feasible or infeasible, meaning that
		// they are already known to work or not work), we set it to be
		// explored. With that, we can reduce the searching space by ignoring
		// those candidates that are stronger than the explored ones.
	Inference curInfer = RELAX; // Initialize the current inference to be all relaxed (RELAX)

 API Methods:
 	bool addInference(infer, bool isLeaf) {
		// Push back infer to the discovered when it's not discvoerd, and return
		// whether it's added or not
	}
	
	void commitInference(infer, isFeasible) {
		// Set the infer to be explored and add it to the result set if feasible 
	}

	Inference* getNextInference() {
		// Get the next unexplored inference so that we can contine searching
	}
*/

/* Forward declaration */
class PatchUnit;
class Patch;
class Inference;


class InferenceSet {
	private:
	/** The set of already discovered nodes in the tree */
	InferenceList *discoveredSet;

	/** The list of feasible inferences */
	InferenceList *results;

	/** The set of candidates */
	InferenceList *candidates;

	/** The staticstics of inference process */
	inference_stat_t stat;
	
	public:
	InferenceSet() {
		discoveredSet = new InferenceList;
		results = new InferenceList;
		candidates = new InferenceList;
	}

	/** Print the result of inferences  */
	void printResults() {
		results->print("Result");
		stat.print();
	}

	/** Print candidates of inferences */
	void printCandidates() {
		candidates->print("Candidate");
	}

	/** When we finish model checking or cannot further strenghen with the
	 * inference, we commit the inference to be explored; if it is feasible, we
	 * put it in the result list */
	void commitInference(Inference *infer, bool feasible) {
		ASSERT (infer);
		
		infer->setExplored(true);
		FENCE_PRINT("Explored %lu\n", infer->getHash());
		if (feasible) {
			addResult(infer);
		}
	}

	int getCandidatesSize() {
		return candidates->size();
	}

	int getResultsSize() {
		return results->size();
	}

	/** Be careful that if the candidate is not added, it will be deleted in this
	 *  function. Therefore, caller of this function should just delete the list when
	 *  finishing calling this function. */
	bool addCandidates(Inference *curInfer, InferenceList *inferList) {
		if (!inferList)
			return false;
		// First prune the list of inference
		inferList->pruneCandidates(curInfer);

		ModelList<Inference*> *cands = inferList->getList();

		// For the purpose of debugging, record all those inferences added here
		ModelList<Inference*> *addedCandidates = new ModelList<Inference*>();
		FENCE_PRINT("Explored size: %d.\n", exploredSetSize());
		FENCE_PRINT("List size: %d.\n", cands->size());
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
		printCandidates(addedCandidates);
		FENCE_PRINT("\n");
		
		// Clean up the candidates
		inferList->clearList();
		FENCE_PRINT("potential results size: %d.\n", candidates->size());
		return added;
	}


	/** Check if we have stronger or equal inferences in the current result
	 * list; if we do, we remove them and add the passed-in parameter infer */
	 void addResult(Inference *infer) {
		for (ModelList<Inference*>::iterator it = results->begin(); it !=
			results->end(); it++) {
			Inference *existResult = *it;
			int compVal = existResult->compareTo(infer);
			if (compVal == 0 || compVal == 1) {
				// The existing result is equal or stronger, remove it
				FENCE_PRINT("We are dumping the follwing inference because it's either too weak or the same:\n");
				existResult->print();
				FENCE_PRINT("\n");
				it = results->erase(it);
				it--;
			}
		}
		results->push_back(infer);
	 }

	/** Get the next available unexplored node; @Return NULL 
	 * if we don't have next, meaning that we are done with exploring */
	Inference* getNextInference() {
		Inference *infer = NULL;
		while (candidates->size() > 0) {
			infer = candidates->back();
			candidates->pop_back();
			if (!infer->getIsLeaf()) {
				commitInference(infer, false);
				continue;
			}
			if (hasBeenExplored(infer)) {
				// Finish exploring this node
				// Remove the node from the set 
				FENCE_PRINT("Explored inference:\n");
				infer->print();
				FENCE_PRINT("\n");
			} else {
				return infer;
			}
		}
		return NULL;
	}

	/** Add the current inference to the set before adding fixes to it; in
	 * this case, fixes will be added afterwards, and infer should've been
	 * discovered */
	void addCurInference(Inference *infer) {
		infer->setLeaf(false);
		candidates->push_back(infer);
	}
	
	/** Add one possible node that represents a fix for the current inference;
	 * @Return true if the node to add has not been explored yet
	 */
	bool addInference(Inference *infer) {
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
	bool hasBeenDiscovered(Inference *infer) {
		for (ModelList<Inference*>::iterator it = discoveredSet->begin(); it !=
			discoveredSet->end(); it++) {
			Inference *discoveredInfer = *it;
			// When we already have an equal inferences in the candidates list
			int compVal = discoveredInfer->compareTo(infer);
			if (compVal == 0) {
				FENCE_PRINT("%lu has beend discovered.\n",
					infer->getHash());
				return true;
			}
			// Or the discoveredInfer is explored and infer is strong than it is
			/*
			if (compVal == -1 && discoveredInfer->isExplored()) {
				return true;
			}
			*/
		}
		return false;
	}

	/** Return false if we don't have that inference in the explored set.
	 * Invariance: if an infrence has been explored, it must've been discovered */
	bool hasBeenExplored(Inference *infer) {
		for (ModelList<Inference*>::iterator it = discoveredSet->begin(); it !=
			discoveredSet->end(); it++) {
			Inference *discoveredInfer = *it;
			if (discoveredInfer->isExplored()) {
				// When we already have any equal or stronger explored inferences,
				// we can say that infer is in the exploredSet
				int compVal = discoveredInfer->compareTo(infer);
				//if (compVal == 0 || compVal == -1) {
				if (compVal == 0) {
					FENCE_PRINT("%lu has beend explored.\n",
						infer->getHash());
					return true;
				}
			}
		}
		return false;
	}

	MEMALLOC
};

#endif
