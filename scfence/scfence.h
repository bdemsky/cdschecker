#ifndef _SCFENCE_H
#define _SCFENCE_H
#include "traceanalysis.h"
#include "scanalysis.h"
#include "hashtable.h"
#include "memoryorder.h"
#include "action.h"
#include "wildcard.h"
#include "model.h"

#include <unordered_set>
#include <functional>
#include <utility>

#ifdef __cplusplus
using std::memory_order;
#endif

#define DEFAULT_REPETITIVE_READ_BOUND 10 

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

extern SCFence *scfence;

typedef action_list_t path_t;
/** A list of load operations can represent the union of reads-from &
 * sequence-before edges; And we have a list of lists of load operations to
 * represent all the possible rfUsb paths between two nodes, defined as
 * syns_paths_t here
 */
typedef SnapList<action_list_t *> sync_paths_t;
typedef sync_paths_t paths_t;

static const char * get_mo_str(memory_order order);

typedef struct Inference {
	memory_order *orders;
	int size;

	bool explored;

	
	Inference() {
		orders = (memory_order *) model_malloc((4 + 1) * sizeof(memory_order*));
		size = 4;
		for (int i = 0; i <= size; i++)
			orders[i] = WILDCARD_NONEXIST;
		explored = false;
	}

	Inference(Inference *infer) {
		ASSERT (infer->size > 0 && infer->size <= MAX_WILDCARD_NUM);
		orders = (memory_order *) model_malloc((infer->size + 1) * sizeof(memory_order*));
		this->size = infer->size;
		for (int i = 0; i <= size; i++)
			orders[i] = infer->orders[i];
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
		int wildcardID = get_wildcard_id(wildcard);
		if (!is_wildcard(wildcard)) {
			FENCE_PRINT("We cannot make this update!\n");
			FENCE_PRINT("wildcard: %d --> mo: %d\n", wildcardID, mo);
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
		/*
		FENCE_PRINT("\n");
		FENCE_PRINT("Strengthened wildcard:%d\n", wildcardID);
		WILDCARD_ACT_PRINT(act);
		FENCE_PRINT("-> %s\n", get_mo_str(mo));
		FENCE_PRINT("\n");
		*/
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
				//FENCE_PRINT("!=\n");
				return -2;
			} else {
				if (mo1 == WILDCARD_NONEXIST && mo2 != WILDCARD_NONEXIST)
					subResult = 1;
				else if (mo1 != WILDCARD_NONEXIST && mo2 == WILDCARD_NONEXIST)
					subResult = -1;
				else
					subResult = mo1 > mo2 ? 1 : (mo1 == mo2) ? 0 : -1;

				//FENCE_PRINT("subResult: %d\n", subResult);
				if ((subResult > 0 && result < 0) || (subResult < 0 && result > 0)) {
					//FENCE_PRINT("!=\n");
					//FENCE_PRINT("in result: %d\n", result);
					//FENCE_PRINT("in result: mo1 & mo2 %d & %d\n", mo1, mo2);
					return -2;
				}
				if (subResult != 0)
					result = subResult;
				/*
				if (subResult == 1) {
					FENCE_PRINT(">\n");
				} else if (subResult == 0) {
					FENCE_PRINT("=\n");
				} else if (subResult == -1) {
					FENCE_PRINT("<\n");
				}
				*/
			}
		}
		//FENCE_PRINT("result: %d\n", result);
		return result;
	}

	void setExplored(bool val) {
		explored = val;
	}

	bool getExplored() {
		return explored;
	}

	void debug_print() {
		ASSERT(size > 0 && size <= MAX_WILDCARD_NUM);
		model_print("Explored: %d\n", explored);
		/*
		for (int i = 1; i <= size; i++) {
	        memory_order mo = orders[i];
			if (mo != WILDCARD_NONEXIST) {
				// Print the wildcard inference result
				FENCE_PRINT("wildcard %d -> memory_order_%s\n", i, get_mo_str(mo));
			}
		}
		*/
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
 * inferences */
typedef struct InferenceStack {
	InferenceStack() {
		exploredSet = new inference_list_t();
		results = new inference_list_t();
		candidates = new inference_list_t();
	}

	/** The set of already explored nodes in the tree */
	inference_list_t *exploredSet;

	/** The list of feasible inferences */
	inference_list_t *results;

	/** The stack of candidates */
	inference_list_t *candidates;

	/** The staticstics of inference process */
	inference_stat_t stat;

	int exploredSetSize() {
		return exploredSet->size();
	}
	
	/** Actions to take when we find node is thoroughly explored */
	void commitExploredInference(Inference *infer) {
		if (!inExploredSet(infer))
			exploredSet->push_back(infer);
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

	/** Print the candidates of inferences  */
	void printCandidates() {
		for (inference_list_t::iterator it = candidates->begin(); it !=
			candidates->end(); it++) {
			Inference *infer = *it;
			int idx = distance(candidates->begin(), it);
			model_print("Candidate %d:\n", idx);
			infer->print();
		}
	}

	/** When we finish model checking or cannot further strenghen with the
	 * current inference, we commit the current inference (the node at the back
	 * of the stack) to be explored, pop it out of the stack; if it is feasible,
	 * we put it in the result list */
	void commitCurInference(bool feasible) {
		Inference *infer = candidates->back();
		if (!infer) {
			return;
		}
		candidates->pop_back();
		commitExploredInference(infer);
		if (feasible) {
			results->push_back(infer);
		}
	}

	/** Get the next available unexplored node; @Return NULL 
	 * if we don't have next, meaning that we are done with exploring */
	Inference* getNextInference() {
		Inference *infer = NULL;
		while (candidates->size() > 0) {
			infer = candidates->back();
			bool inExplored = inExploredSet(infer);
			if (inExplored) {
				stat.getNextNotAdded++;
			}
			if (infer->getExplored() || inExplored) {
				// Finish exploring this node
				// Remove the node from the stack
				candidates->pop_back();
				if (!infer->getExplored()) {
					FENCE_PRINT("Popped inferences not because of being explored:\n");
					infer->print();
					FENCE_PRINT("\n");
				}
				// Record this in the exploredSet
				commitExploredInference(infer);
			} else {
				infer->setExplored(true);
				return infer;
			}
		}
		return NULL;
	}

	
	/** Add one possible node that represents a fix for the current inference;
	 * @Return true if the node to add has not been explored yet
	 */
	bool addInference(Inference *infer) {
		if (!inExploredSet(infer)) {
			// We haven't explored this inference yet
			candidates->push_back(infer);
			return true;
		} else {
			stat.notAddedAtFirstPlace++;
			return false;
		}
	}

	/** Return false if we don't have that inference in the explored set */
	bool inExploredSet(Inference *infer) {
		for (inference_list_t::iterator it = exploredSet->begin(); it !=
			exploredSet->end(); it++) {
			Inference *exploredInfer = *it;
			// When we already have any equal or stronger explored inferences,
			// we can say that infer is in the exploredSet
			int compVal = exploredInfer->compareTo(infer);
			if (compVal == 0 || compVal == -1) {
				/*
				if (compVal == 0)
					FENCE_PRINT("Equal inferences:\n");
				else
					FENCE_PRINT("Stronger inference:\n");
				FENCE_PRINT("Explored inference:\n");
				exploredInfer->print();
				FENCE_PRINT("\n");
				FENCE_PRINT("Param (infer):\n");
				infer->print();
				FENCE_PRINT("\n");
				*/
				return true;
			} else {
				return false;
			}
		}
		/*
		inference_list_t::iterator it = exploredSet->find(infer);
		if (it == exploredSet->end()) {
			return false;
		} else {
			return true;
		}
		*/
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

	MEMALLOC
} scfence_priv;

typedef enum fix_type {
	BUGGY_EXECUTION,
	IMPLICIT_MO,
	NON_SC
} fix_type_t;

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
	bool processRead(ModelAction *read, ClockVector *cv);
	int getNextActions(ModelAction **array);
	bool merge(ClockVector *cv, const ModelAction *act, const ModelAction *act2);
	void check_rf(action_list_t *list);
	void reset(action_list_t *list);
	ModelAction* pruneArray(ModelAction**, int);

	int maxthreads;
	HashTable<const ModelAction *, ClockVector *, uintptr_t, 4 > cvmap;
	bool cyclic;
	HashTable<const ModelAction *, const ModelAction *, uintptr_t, 4 > badrfset;
	HashTable<void *, const ModelAction *, uintptr_t, 4 > lastwrmap;
	SnapVector<action_list_t> threadlists;
	SnapVector<action_list_t> dup_threadlists;
	ModelExecution *execution;
	bool print_always;
	bool print_buggy;
	bool print_nonsc;
	bool time;
	struct sc_statistics *stats;

	/********************** SCF-related stuff (beginning) **********************/

	


	/********************** SCFence-related stuff (beginning) **********************/
	
	/** The non-snapshotting private compound data structure that has the
	 * necessary stuff for the scfence analysis */
	static scfence_priv *priv;

	/** The function to parse the SCFence plugin options */
	bool parseOption(char *opt);

	/** Helper function for option parsing */
	char* parseOptionHelper(char *opt, int *optIdx);

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

	/** Add candidates with a list of inferences; returns false if nothing is
	 * added */
	bool addCandidates(ModelList<Inference*> *candidates);

	/** Check whether a chosen reads-from path is a release sequence */
	bool isReleaseSequence(path_t *path);

	/** Impose synchronization to one inference (infer) according to path.  If
	 * infer is NULL, we first create a new Inference by copying the current
	 * inference; otherwise we copy a new inference by infer. We then try to
	 * impose path to the newly created inference or the passed-in infer. If we
	 * cannot strengthen the inference by the path, we return NULL, otherwise we
	 * return the newly created inference */
	Inference* imposeSyncToInference(Inference *infer, path_t *path);

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
	ModelList<Inference*>* imposeSync(ModelList<Inference*> *partialCandidates, paths_t *paths);

	/** Recycle the allcated memory to the list of inferences */
	void clearCandidates(ModelList<Inference*> *candidates);

	/** Impose some paths of SC to an already existing candidates of inferences;
	 * if partialCandidates is NULL, it imposes the the corrsponding SC
	 * parameters (act1 & act2) to the current inference. It returns the newly
	 * strengthened list of inferences */
	ModelList<Inference*>* imposeSC(ModelList<Inference*> *partialCandidates, const ModelAction *act1, const ModelAction *act2);

	const char* get_mo_str(memory_order order);

	/** When we finish model checking or cannot further strenghen with the
	 * current inference, we commit the current inference (the node at the back
	 * of the stack) to be explored, pop it out of the stack; if it is feasible,
	 * we put it in the result list */
	void commitCurInference(bool feasible) {
		getStack()->commitCurInference(feasible);	
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

	InferenceStack* getStack() {
		return priv->inferenceStack;
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

	/********************** SCFence-related stuff (end) **********************/
};
#endif
