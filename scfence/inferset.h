#ifndef _INFERSET_H
#define _INFERSET_H

#include "inference.h"
#include "fence_common.h"

typedef struct inference_stat {
	int notAddedAtFirstPlace;
	int getNextNotAdded;

	inference_stat() :
		notAddedAtFirstPlace(0),
		getNextNotAdded(0) {}

	void print() {
		model_print("Inference process statistics output:...\n");
		model_print("The number of inference not added at the first place: %d\n",
			notAddedAtFirstPlace);
		model_print("The number of inference not added when getting next: %d\n",
			getNextNotAdded);
	}
} inference_stat_t;


/** This is a set of the inferences. We are exploring possible
 *  inferences in a DFS-like way. Also, we can do an state-based like
 *  optimization to reduce the explored space by recording the explored
 *  inferences.

 ********** The main algorithm **********
 Initial:
 	InferenceSet set; // Store the candiates to explore in a set
	Set exploredSet; // Store the explored candidates (feasible or infeasible)
	Inference curInfer = RELAX; // Initialize the current inference to be all relaxed (RELAX)

 Methods:
 	bool addInference(infer) {
		// Push back infer to the set when it's neither discvoerd nor
		// explored, and return whether it's added or not
	}

	void commitExplored(infer) {
		// Add infer to the exploredSet
	}

	Inference* getNextInference() {
		
	}
*/

/* Forward declaration */
class PatchUnit;
class Patch;
class Inference;


class InferenceSet {
	private:
	/** The set of already explored nodes in the tree */
	ModelList<Inference*> *exploredSet;

	/** The set of already discovered nodes in the tree */
	ModelList<Inference*> *discoveredSet;

	/** The list of feasible inferences */
	ModelList<Inference*> *results;

	/** The set of candidates */
	ModelList<Inference*> *candidates;

	/** The staticstics of inference process */
	inference_stat_t stat;
	
	public:
	InferenceSet() {
		exploredSet = new ModelList<Inference*>;
		discoveredSet = new ModelList<Inference*>;
		results = new ModelList<Inference*>;
		candidates = new ModelList<Inference*>;
	}


	int exploredSetSize() {
		return exploredSet->size();
	}
	
	/** Print the result of inferences  */
	void printResults() {
		for (ModelList<Inference*>::iterator it = results->begin(); it !=
			results->end(); it++) {
			Inference *infer = *it;
			int idx = distance(results->begin(), it);
			model_print("Result %d:\n", idx);
			infer->print();
		}

		stat.print();
	}

	/** Print candidates of inferences */
	void printCandidates() {
		printCandidates(candidates);
	}

	/** Print the candidates of inferences  */
	void printCandidates(ModelList<Inference*> *cands) {
		for (ModelList<Inference*>::iterator it = cands->begin(); it !=
			cands->end(); it++) {
			Inference *infer = *it;
			int idx = distance(cands->begin(), it);
			model_print("Candidate %d:\n", idx);
			infer->print();
		}
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
