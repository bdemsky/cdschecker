#ifndef _SCFENCE_H
#define _SCFENCE_H
#include "traceanalysis.h"
#include "scanalysis.h"
#include "hashtable.h"
#include "memoryorder.h"
#include "action.h"
#include "wildcard.h"
#include "model.h"

#include <sys/time.h>

#include <unordered_set>
#include <functional>
#include <utility>


#ifdef __cplusplus
using std::memory_order;
#endif

#define DEFAULT_REPETITIVE_READ_BOUND 20

#define FENCE_OUTPUT

#ifdef FENCE_OUTPUT

#define FENCE_PRINT model_print

#define ACT_PRINT(x) (x)->print()

#define CV_PRINT(x) (x)->print()

#define WILDCARD_ACT_PRINT(x)\
	FENCE_PRINT("Wildcard: %d\n", get_wildcard_id_zero((x)->get_original_mo()));\
	ACT_PRINT(x);

#else

#define FENCE_PRINT

#define ACT_PRINT(x)

#define CV_PRINT(x)

#define WILDCARD_ACT_PRINT(x)

#endif



/* Forward declaration */
class SCFence;
class Inference;

extern SCFence *scfence;

static const char* get_mo_str(memory_order order);

static bool isTheInference(Inference *infer);

typedef action_list_t path_t;
/** A list of load operations can represent the union of reads-from &
 * sequence-before edges; And we have a list of lists of load operations to
 * represent all the possible rfUsb paths between two nodes, defined as
 * syns_paths_t here
 */
typedef SnapList<path_t *> sync_paths_t;
typedef sync_paths_t paths_t;

typedef struct Inference {
	memory_order *orders;
	int size;

	/** Whether this inference will lead to a buggy execution */
	bool buggy;

	/** Whether this inference has been explored */
	bool explored;

	/** Whether this inference is a leaf of the search tree */
	bool isLeaf;
	
	/** When this inference will have buggy executions, this indicates whether
	 * it has any fixes. */
	bool hasFixes;

	Inference() {
		orders = (memory_order *) model_malloc((4 + 1) * sizeof(memory_order*));
		size = 4;
		for (int i = 0; i <= size; i++)
			orders[i] = WILDCARD_NONEXIST;
		buggy = false;
		hasFixes = false;
		isLeaf = false;
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
		isLeaf = false;
		explored = false;
	}

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

	memory_order &operator[](int idx) {
		if (idx > 0 && idx <= size)
			return orders[idx];
		else {
			resize(idx);
			orders[idx] = WILDCARD_NONEXIST;
			return orders[idx];
		}
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

	void setLeaf(bool val) {
		isLeaf = val;
	}

	bool getIsLeaf() {
		return isLeaf;
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
} Inference;


/** Define a HashSet that supports customized equals_to() & hash() functions so
 * that we can use it in our analysis
*/
template<class _Key, class _Hash, class _KeyEqual>
class ModelSet : public std::unordered_set<_Key, _Hash, _KeyEqual, ModelAlloc<_Key> >
{
 public:
	MEMALLOC
};


class InferenceHash : public std::hash<Inference*>
{
	public:
	size_t operator()(Inference* const X) const {
		size_t hash = X->orders[1] + 4096;
		for (int i = 2; i < X->size; i++) {
			hash *= 37;
			hash += (X->orders[i] + 4096);
		}
		return hash;
	}

	MEMALLOC
};

class InferenceEquals : public std::equal_to<Inference*>
{
	public:
	bool operator()(Inference* const lhs, Inference* const rhs) const {
		return lhs->compareTo(rhs) == 0;
	}

	MEMALLOC
};

/** Type-define the inference_set_t we need throughout the analysis here */
typedef ModelSet<Inference*, InferenceHash, InferenceEquals> inference_set_t;

typedef ModelList<Inference*> inference_list_t;

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

/** This is a stack of those inferences. We are exploring possible
 * inferences in a DFS-like way. Also, we can do an state-based like
 * optimization to reduce the explored space by recording the explored
 * inferences.

 ********** The main algorithm **********
 Initial:
 	InferenceStack stack; // Store the candiates to explore in a stack
	Set exploredSet; // Store the explored candidates (feasible or infeasible)
	Inference curInfer = RELAX; // Initialize the current inference to be all relaxed (RELAX)

 Methods:
 	bool addInference(infer) {
		// Push back infer to the stack when it's neither discvoerd nor
		// explored, and return whether it's added or not
	}

	void commitExplored(infer) {
		// Add infer to the exploredSet
	}

	Inference* getNextInference() {
		
	}
 
 
 
 */
typedef struct InferenceStack {
	InferenceStack() {
		exploredSet = new inference_list_t();
		discoveredSet = new inference_list_t();
		results = new inference_list_t();
		candidates = new inference_list_t();
	}

	/** The set of already explored nodes in the tree */
	inference_list_t *exploredSet;

	/** The set of already discovered nodes in the tree */
	inference_list_t *discoveredSet;

	/** The list of feasible inferences */
	inference_list_t *results;

	/** The stack of candidates */
	inference_list_t *candidates;

	/** The staticstics of inference process */
	inference_stat_t stat;

	int exploredSetSize() {
		return exploredSet->size();
	}
	
	/** Print the result of inferences  */
	void printResults() {
		for (inference_list_t::iterator it = results->begin(); it !=
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
	void printCandidates(inference_list_t *cands) {
		for (inference_list_t::iterator it = cands->begin(); it !=
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

	/** Check if we have stronger or equal inferences in the current result
	 * list; if we do, we remove them and add the passed-in parameter infer */
	 bool addResult(Inference *infer) {
		for (inference_list_t::iterator it = results->begin(); it !=
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
				// Remove the node from the stack
				FENCE_PRINT("Explored inference:\n");
				infer->print();
				FENCE_PRINT("\n");
			} else {
				return infer;
			}
		}
		return NULL;
	}

	/** Add the current inference to the stack before adding fixes to it; in
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
		for (inference_list_t::iterator it = discoveredSet->begin(); it !=
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
		for (inference_list_t::iterator it = discoveredSet->begin(); it !=
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
} InferenceStack;

typedef ModelList<Inference*> candidates_t;

typedef struct scfence_priv {
	scfence_priv() {
		inferenceStack = new InferenceStack();
		curInference = new Inference();
		candidateFile = NULL;
		inferImplicitMO = false;
		hasRestarted = false;
		implicitMOReadBound = DEFAULT_REPETITIVE_READ_BOUND;
		timeout = 0;
		gettimeofday(&lastRecordedTime, NULL);
	}

	/** The stack of the InferenceNode we maintain for exploring */
	InferenceStack *inferenceStack;

	/** The current inference */
	Inference *curInference;

	/** The file which provides a list of candidate wilcard inferences */
	char *candidateFile;

	/** Whether we consider the repetitive read to infer mo (_m) */
	bool inferImplicitMO;
	
	/** The bound above which we think that write should be the last write (_b) */
	int implicitMOReadBound;

	/** Whether we have restarted the model checker */
	bool hasRestarted;

	/** The amount of time (in second) set to force the analysis to backtrack */
	int timeout;

	/** The time we recorded last time */
	struct timeval lastRecordedTime;

	MEMALLOC
} scfence_priv;

typedef enum fix_type {
	BUGGY_EXECUTION,
	IMPLICIT_MO,
	NON_SC
} fix_type_t;

struct PatchUnit {
	int wildcard;
	memory_order mo;

	PatchUnit(int w, memory_order mo) {
		this->wildcard = w;
		this->mo = mo;
	}
};

struct Patch {
	SnapVector<PatchUnit*> *units;

	Patch(int wildcard, memory_order mo) {
		PatchUnit *unit = new PatchUnit(wildcard, mo);
		units = new SnapVector<PatchUnit*>;
		units->push_back(unit);
	}

	Patch(int wildcard1, memory_order mo1, int wildcard2, memory_order mo2) {
		units = new SnapVector<PatchUnit*>;
		PatchUnit *unit = new PatchUnit(wildcard1, mo1);
		units->push_back(unit);
		unit = new PatchUnit(wildcard2, mo2);
		units->push_back(unit);
	}

	Patch() {
		unit = new PatchUnit(wildcard2, mo2);
	}

	void addPatchUnit(int wildcard, memory_order mo) {
		PatchUnit *unit = new PatchUnit(wildcard, mo);
		units->push_back(unit);
	}

	getSize() {
		return units->size();
	}

	get(int i) {
		return (*units)[i];
	}
};


/** This struct represents that the list of inferences that can fix the problem
 * */
struct InferenceList {
	ModelList<Inference*> *list;
	Inference *curInfer;

	void addFixes(SnapVector<Patch*> pathers) {

	}

	bool applyPatch(Inference *newInfer, Patch *patch) {
		bool updateState = false,
			canUpdate = true,
			hasUpdated = false;
		for (int i = 0; i < patch->getSize(); i++) {
			canUpdate = true;
			hasUpdated = false;
			PatchUnit *unit = patch->get(i);
			newInfer->strengthen(unit->wildcard, unit->mo, canUpdate, hasUpdated);
			if (!canUpdate) {
				// This is not a feasible patch, bail
				break;
			} else if (hasUpdated) {
				updateState = true;
			}
		}
		if (canUpdate && hasUpdated) {
			list->push_back(newInfer);
			return true;
		} else {
			return false;
		}
	}

	void addFixes(Patch* patch) {
		if (list->empty()) {
			bool updateState = false,
				canUpdate = true,
				hasUpdated = false;
			Inference *newInfer = new Inference(curInfer);
			if (!applyPatch(newInfer, patch)) {
				delete newInfer;
			}
		} else {
			ModelList<Inference*> *newList = new ModelList<Inference*>;
			for (ModelList<Inference*>::iterator it = list->begin(); it !=
				list->end(); it++) {
				Inference *oldInfer = *it;
				Inference *newInfer = new Inference(oldInfer);
				if (!applyPatch(newInfer, patch)) {
					delete newInfer;
				}
			}
			// Clean the old list
			for (int i = 0; i < list->size(); i++) {
				delete list[i];
			}
			delete list;
			list = newList;
		}	
	}

	void addFixes(SnapVector<Patch*> patches) {
		if (list->empty()) {
			bool updateState = false,
				canUpdate = true,
				hasUpdated = false;
			Inference *newInfer = new Inference(curInfer);
			for (int i = 0; i < patches->size(); i++) {
				canUpdate = true;
				hasUpdated = false;
				Patch *p = patches[i];
				newInfer->strengthen(p->wildcard, p->mo, canUpdate, hasUpdated);
				if (!canUpdate) {
					// This is not a feasible patch
					delete newInfer;
					continue;
				} else if (hasUpdated) {
					list->push_back(newInfer);
				}
			}
			
		} else {
			ModelList<Inference*> *newList = new ModelList<Inference*>;
			for (ModelList<Inference*>::iterator it = list->begin(); it !=
				list->end(); it++) {
				Inference *oldInfer = *it;
				Inference *newInfer = new Inference(oldInfer);
				bool updateState = false,
					canUpdate = true,
					hasUpdated = false;
				for (int i = 0; i < patches->size(); i++) {
					canUpdate = true;
					hasUpdated = false;
					Patch *p = patches[i];
					newInfer->strengthen(p->wildcard, p->mo, canUpdate, hasUpdated);
					if (!canUpdate) {
						// This is not a feasible patch, bail
						break;
					} else if (hasUpdated) {
						updateState = true;
					}
				}
				if (canUpdate && hasUpdated) {
					newList->push_back(newInfer);
				} else {
					delete newInfer;
				}
			}
			// Clean the old list
			for (int i = 0; i < list->size(); i++) {
				delete list[i];
			}
			delete list;
			list = newList;
		}
	}
};

class SCFence : public TraceAnalysis {
 public:
	SCFence();
	~SCFence();
	virtual void setExecution(ModelExecution * execution);
	virtual void analyze(action_list_t *);
	virtual const char * name();
	virtual bool option(char *);
	virtual void finish();

	virtual void inspectModelAction(ModelAction *ac);
	virtual void actionAtInstallation();
	virtual void actionAtModelCheckingFinish();

	SNAPSHOTALLOC

 private:
	/********************** SC-related stuff (beginning) **********************/

	void update_stats();
	void print_list(action_list_t *list);
	int buildVectors(SnapVector<action_list_t> *threadlist, int *maxthread, action_list_t *);
	bool updateConstraints(ModelAction *act);
	void computeCV(action_list_t *);
	action_list_t * generateSC(action_list_t *);

	bool fastVersion;
	bool allowNonSC;

	bool processReadFast(ModelAction *read, ClockVector *cv);
	bool processReadSlow(ModelAction *read, ClockVector *cv, bool *updateFuture);
	bool processAnnotatedReadSlow(ModelAction *read, ClockVector *cv, bool *updateFuture);
	int getNextActions(ModelAction **array);
	bool merge(ClockVector *cv, const ModelAction *act, const ModelAction *act2);
	void check_rf(action_list_t *list);
	void reset(action_list_t *list);
	ModelAction* pruneArray(ModelAction**, int);

	int maxthreads;
	HashTable<const ModelAction *, ClockVector *, uintptr_t, 4 > cvmap;
	bool cyclic;
	HashTable<const ModelAction *, const ModelAction *, uintptr_t, 4 > badrfset;
	int badrfsetSize;
	HashTable<void *, const ModelAction *, uintptr_t, 4 > lastwrmap;
	SnapVector<action_list_t> threadlists;
	SnapVector<action_list_t> dup_threadlists;
	ModelExecution *execution;
	bool print_always;
	bool print_buggy;
	bool print_nonsc;
	bool time;
	struct sc_statistics *stats;

	/** The set of read actions that are annotated to be special and will
	 *  receive special treatment */
	HashTable<const ModelAction *, const ModelAction *, uintptr_t, 4 > annotatedReadSet;
	int annotatedReadSetSize;
	bool annotationMode;
	bool annotationError;
	/** This routine is operated based on the built threadlists */
	void collectAnnotatedReads();

	/********************** SC-related stuff (end) **********************/

	

	/********************** SCFence-related stuff (beginning) **********************/

	/** A set of actions that should be ignored in the partially SC analysis */
	HashTable<const ModelAction*, const ModelAction*, uintptr_t, 4> ignoredActions;
	int ignoredActionSize;

	/** The non-snapshotting private compound data structure that has the
	 * necessary stuff for the scfence analysis */
	static scfence_priv *priv;

	/** The function to parse the SCFence plugin options */
	bool parseOption(char *opt);

	/** Helper function for option parsing */
	char* parseOptionHelper(char *opt, int *optIdx);

	/** A pass through the original actions to extract the ignored actions
	 * (partially SC); it must be called after the threadlist has been built */
	void extractIgnoredActions();

	/** Functions that work for infering the parameters by impsing
	 * synchronization */
	paths_t *get_rf_sb_paths(const ModelAction *act1, const ModelAction *act2);
	
	/** Printing function for those paths imposed by happens-before; only for
	 * the purpose of debugging */
	void print_rf_sb_paths(paths_t *paths, const ModelAction *start, const ModelAction *end);

	/** Whether there's an edge between from and to actions */
	bool isSCEdge(const ModelAction *from, const ModelAction *to) {
		return from->is_seqcst() && to->is_seqcst();
	}
	
	bool isConflicting(const ModelAction *act1, const ModelAction *act2) {
		return act1->get_location() == act2->get_location() ? (act1->is_write()
			|| act2->is_write()) : false;
	}
	
	/** Initialize the search with a file with a list of potential candidates */
	void initializeByFile();

	/** The two important routine when we find or cannot find a fix for the
	 * current inference */
	void routineAfterAddFixes();
	bool routineBacktrack(bool feasible);

	/** A subroutine to find candidates for pattern (a) */
	ModelList<Inference*>* getFixesFromPatternA(action_list_t *list, action_list_t::iterator readIter, action_list_t::iterator writeIter);

	/** A subroutine to find candidates for pattern (b) */
	ModelList<Inference*>* getFixesFromPatternB(action_list_t *list, action_list_t::iterator readIter, action_list_t::iterator writeIter);

	/** When getting a non-SC execution, find potential fixes and add it to the
	 * stack */
	bool addFixesNonSC(action_list_t *list);

	/** When getting a buggy execution (we only target the uninitialized loads
	 * here), find potential fixes and add it to the stack */
	bool addFixesBuggyExecution(action_list_t *list);

	/** When getting an SC and bug-free execution, we check whether we should
	 * fix the implicit mo problems. If so, find potential fixes and add it to
	 * the stack */
	bool addFixesImplicitMO(action_list_t *list);

	/** General fixes wrapper */
	bool addFixes(action_list_t *list, fix_type_t type);

	/** Only choose the weakest existing candidates & they must be stronger than
	 * the current inference */
	ModelList<Inference*>* pruneCandidates(ModelList<Inference*> *candidates);

	/** Add candidates with a list of inferences; returns false if nothing is
	 * added */
	bool addCandidates(ModelList<Inference*> *candidates);

	/** Check whether a chosen reads-from path is a release sequence */
	bool isReleaseSequence(path_t *path);

	/** Impose synchronization to one inference (infer) according to path, begin
	 *  is the beginning operation, and end is the ending operation.  If infer
	 *  is NULL, we first create a new Inference by copying the current
	 *  inference; otherwise we copy a new inference by infer. We then try to
	 *  impose path to the newly created inference or the passed-in infer. If we
	 *  cannot strengthen the inference by the path, we return NULL, otherwise
	 *  we return the newly created inference */
	//Inference* imposeSyncToInference(Inference *infer, path_t *path, bool &canUpdate, bool &hasUpdated);
	ModelList<Inference*>* imposeSyncToInference(Inference *infer, path_t *path, const
		ModelAction *begin, const ModelAction *end);

	ModelList<Inference*>* imposeSyncToInferenceAtReadsFrom(ModelList<Inference*>* inferList, path_t
		*path, const ModelAction *read, const ModelAction *readBound, const
		ModelAction *write, const ModelAction *writeBound);

	/** For a specific path, try to see if there is a possible pair of acq/rel
	 * fences that can impose hb */
	void getAcqRelFences(path_t *path, const ModelAction *read, const
		ModelAction *readBound, const ModelAction *write, const ModelAction
		*writeBound, const ModelAction *&acqFence, const ModelAction *&relFence);

	/** Impose SC to one inference (infer) by action1 & action2.  If infer is
	 * NULL, we first create a new Inference by copying the current inference;
	 * otherwise we copy a new inference by infer. We then try to impose SC to
	 * the newly created inference or the passed-in infer. If we cannot
	 * strengthen the inference, we return NULL, otherwise we return the newly
	 * created or original inference */
	Inference* imposeSCToInference(Inference *infer, const ModelAction *act1, const ModelAction *act2);

	/** Impose some paths of synchronization to an already existing candidates
	 * of inferences; if partialCandidates is NULL, it imposes the paths to the
	 * current inference. It returns the newly strengthened list of inferences
	 * */
	ModelList<Inference*>* imposeSync(ModelList<Inference*> *partialCandidates,
		paths_t *paths, const ModelAction *begin, const ModelAction *end);

	/** Recycle the allcated memory to the list of inferences */
	void clearCandidates(ModelList<Inference*> *candidates);

	/** Impose some paths of SC to an already existing candidates of inferences;
	 * if partialCandidates is NULL, it imposes the the corrsponding SC
	 * parameters (act1 & act2) to the current inference. It returns the newly
	 * strengthened list of inferences */
	ModelList<Inference*>* imposeSC(ModelList<Inference*> *partialCandidates, const ModelAction *act1, const ModelAction *act2);

	/** When we finish model checking or cannot further strenghen with the
	 * current inference, we commit the current inference (the node at the back
	 * of the stack) to be explored, pop it out of the stack; if it is feasible,
	 * we put it in the result list */
	void commitInference(Inference *infer, bool feasible) {
		getStack()->commitInference(infer, feasible);	
	}

	/** Get the next available unexplored node; @Return NULL 
	 * if we don't have next, meaning that we are done with exploring */
	Inference* getNextInference() {
		return getStack()->getNextInference();
	}

	/** Add one possible node that represents a fix for the current inference;
	 * @Return true if the node to add has not been explored yet
	 */
	bool addInference(Inference *infer) {
		return getStack()->addInference(infer);
	}

	void addCurInference() {
		getStack()->addCurInference(getCurInference());
	}

	int exploredSetSize() {
		return getStack()->exploredSetSize();
	}

	/** Print the result of inferences  */
	void printResults() {
		getStack()->printResults();
	}

	void printCandidates(ModelList<Inference*> *list) {
		for (ModelList<Inference*>::iterator it = list->begin(); it !=
			list->end(); it++) {
			int idx = distance(list->begin(), it);
			Inference *infer = *it;
			model_print("Candidate %d:\n", idx);
			infer->print();
			model_print("\n");
		}
	}

	/** Print the candidates of inferences  */
	void printCandidates() {
		getStack()->printCandidates();
	}
			

	/** The number of nodes in the stack (including those parent nodes (set as
	 * explored) */
	 int stackSize() {
		return getStack()->candidates->size();
	 }

	/** Set the restart flag of the model checker in order to restart the
	 * checking process */
	void restartModelChecker();
	
	/** Set the exit flag of the model checker in order to exit the whole
	 * process */
	void exitModelChecker();

	bool modelCheckerAtExitState();

	const char* net_mo_str(memory_order order);

	InferenceStack* getStack() {
		return priv->inferenceStack;
	}

	void setHasFixes(bool val) {
		getCurInference()->setHasFixes(val);
	}

	bool hasFixes() {
		return getCurInference()->getHasFixes();
	}

	bool isBuggy() {
		return getCurInference()->getBuggy();
	}

	bool setBuggy(bool val) {
		getCurInference()->setBuggy(val);
	}

	Inference* getCurInference() {
		return priv->curInference;
	}

	void setCurInference(Inference* infer) {
		priv->curInference = infer;
	}

	char* getCandidateFile() {
		return priv->candidateFile;
	}

	void setCandidateFile(char* file) {
		priv->candidateFile = file;
	}

	bool getInferImplicitMO() {
		return priv->inferImplicitMO;
	}

	void setInferImplicitMO(bool val) {
		priv->inferImplicitMO = val;
	}

	int getImplicitMOReadBound() {
		return priv->implicitMOReadBound;
	}

	void setImplicitMOReadBound(int bound) {
		priv->implicitMOReadBound = bound;
	}

	int getHasRestarted() {
		return priv->hasRestarted;
	}

	void setHasRestarted(int val) {
		priv->hasRestarted = val;
	}

	void setTimeout(int timeout) {
		priv->timeout = timeout;
	}

	int getTimeout() {
		return priv->timeout;
	}

	bool isTimeout() {
		struct timeval now;
		gettimeofday(&now, NULL);
		// Check if it should be timeout
		struct timeval *lastRecordedTime = &priv->lastRecordedTime;
		unsigned long long elapsedTime = (now.tv_sec*1000000 + now.tv_usec) -
			(lastRecordedTime->tv_sec*1000000 + lastRecordedTime->tv_usec);

		// Update the lastRecordedTime
		priv->lastRecordedTime = now;
		if (elapsedTime / 1000000.0 > priv->timeout)
			return true;
		else
			return false;
	}

	/********************** SCFence-related stuff (end) **********************/
};
#endif
