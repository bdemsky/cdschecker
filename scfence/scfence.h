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
#else
#define FENCE_PRINT
#define ACT_PRINT(x)
#define CV_PRINT(x)
#endif


#define WILDCARD_ACT_PRINT(x)\
	FENCE_PRINT("Wildcard: %d\n", get_wildcard_id_zero((x)->get_original_mo()));\
	ACT_PRINT(x);

/* Forward declaration */
class SCFence;

extern SCFence *scfence;


/** A list of load operations can represent the union of reads-from &
 * sequence-before edges; And we have a list of lists of load operations to
 * represent all the possible rfUsb paths between two nodes, defined as
 * syns_paths_t here
 */
typedef SnapList<action_list_t *> sync_paths_t;

static const char * get_mo_str(memory_order order);

typedef struct Inference {
	memory_order *orders;
	int size;
	
	Inference() {
		orders = (memory_order *) model_malloc((4 + 1) * sizeof(memory_order*));
		this->size = 4;
		for (int i = 0; i <= size; i++)
			orders[i] = WILDCARD_NONEXIST;
	}

	Inference(Inference *infer) {
		ASSERT (infer->size > 0 && infer->size <= MAX_WILDCARD_NUM);
		orders = (memory_order *) model_malloc((infer->size + 1) * sizeof(memory_order*));
		this->size = infer->size;
		for (int i = 0; i <= size; i++)
			orders[i] = infer->orders[i];
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

	bool strengthen(const ModelAction *act, memory_order mo) {
		memory_order wildcard = act->get_original_mo();
		int wildcardID = get_wildcard_id(wildcard);
		if (!is_wildcard(wildcard)) {
			model_print("We cannot make this update!\n");
			model_print("wildcard: %d --> mo: %d\n", wildcardID, mo);
			act->print();
			return false;
		}
		if (wildcardID > size)
			resize(wildcardID);
		ASSERT (is_normal_mo(mo));
		//model_print("wildcardID -> order: %d -> %d\n", wildcardID, orders[wildcardID]);
		ASSERT (is_normal_mo_infer(orders[wildcardID]));
		switch (orders[wildcardID]) {
			case memory_order_seq_cst:
				break;
			case memory_order_relaxed:
				orders[wildcardID] = mo;
				break;
			case memory_order_acquire:
				if (mo == memory_order_release)
					orders[wildcardID] = memory_order_acq_rel;
				else if (mo >= memory_order_acq_rel)
					orders[wildcardID] = mo;
				break;
			case memory_order_release:
				if (mo == memory_order_acquire)
					orders[wildcardID] = memory_order_acq_rel;
				else if (mo >= memory_order_acq_rel)
					orders[wildcardID] = mo;
				break;
			case memory_order_acq_rel:
				if (mo == memory_order_seq_cst)
					orders[wildcardID] = mo;
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
		return true;
	}

	/** @Return:
		1 -> 'this' > one of the inferences in the result list
		0 -> 'this' == one of the inferences in the result list
		-1 -> 'this' is weaker or uncomparable to any of the inferences
	*/
	int compareTo(ModelList<Inference*> *list) {
		for (ModelList<Inference*>::iterator it = list->begin(); it !=
			list->end(); it++) {
			Inference *res = *it;
			int compVal = compareTo(res);
			if (compVal == 1) {
				/*
				FENCE_PRINT("Comparing reulsts: %d!\n", compVal);
				res->print();
				FENCE_PRINT("\n");
				print();
				FENCE_PRINT("\n");
				*/
				return 1;
			} else if (compVal == 0) {
				/*
				FENCE_PRINT("Comparing reulsts: %d!\n", compVal);
				res->print();
				FENCE_PRINT("\n");
				print();
				FENCE_PRINT("\n");
				*/
				return 0;
			}
		}
		return -1;
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


/** This represents an inference node in the inference stack. It is associated
 * with one inference we might or might not finish exploring */
typedef struct InferenceNode {
	/** The inference assoicates with this node */
	Inference *infer;

	/** Indicate whether this node has been thoroughly explored */
	bool explored;
	
	InferenceNode(Inference *infer, bool explored) :
		infer(infer),
		explored(explored) {}

	InferenceNode(Inference *infer) :
		infer(infer),
		explored(false) {}

	Inference* getInference() const { return infer; }
	
	void setInference(Inference* infer) { this->infer = infer; }

	bool getExplored() const { return explored; }

	void setExplored(bool explored) { this->explored = explored; }

	MEMALLOC
} InferenceNode;


/** Define a HashSet that supports customized equals_to() & hash() functions so
 * that we can use it in our analysis
*/
template<class _Key, class _Hash, class _KeyEqual>
class ModelSet : public std::unordered_set<_Key, _Hash, _KeyEqual, ModelAlloc<_Key> >
{
 public:
	MEMALLOC
};

class InferenceNodeHash : public std::hash<InferenceNode*>
{
	public:
	size_t operator()(InferenceNode* const X) const {
		Inference *infer = X->getInference();
		size_t hash = infer->orders[1] + 4096;
		for (int i = 2; i < infer->size; i++) {
			hash *= 37;
			hash += (infer->orders[i] + 4096);
		}
		return hash;
	}

	MEMALLOC
};

class InferenceNodeEquals : public std::equal_to<InferenceNode*>
{
	public:
	bool operator()(InferenceNode* const lhs, InferenceNode* const rhs) const {
		Inference *x = lhs->getInference(),
			*y = rhs->getInference();
		return x->compareTo(y) == 0;
	}

	MEMALLOC
};

/** Type-define the inference_set_t we need throughout the analysis here */
typedef ModelSet<InferenceNode*, InferenceNodeHash, InferenceNodeEquals> node_set_t;

typedef ModelList<InferenceNode*> inference_node_list_t;

/** This is a stack of those inference nodes. We are exploring possible
 * inferences in a DFS-like way. Also, we can do an state-based like
 * optimization to reduce the explored space by recording the explored
 * inferences */
typedef struct InferenceStack {
	InferenceStack() {
	}

	/** The set of already explored nodes in the tree */
	node_set_t *exploredSet;

	/** The list of feasible inferences */
	inference_node_list_t *results;

	/** The stack of nodes */
	inference_node_list_t *nodes;
	
	/** Actions to take when we find node is thoroughly explored */
	void commitNodeExplored(InferenceNode *node) {
		exploredSet->insert(node);
	}

	/** When we finish model checking or cannot further strenghen with the
	 * current inference, we commit the current inference (the node at the back
	 * of the stack) to be explored, pop it out of the stack; if it is feasible,
	 * we put it in the result list */
	void commitCurInference(bool feasible) {
		InferenceNode *node = nodes->back();
		nodes->pop_back();
		commitNodeExplored(node);
		if (feasible) {
			results->push_back(node);
		}
	}

	/** Get the next available unexplored node; @Return NULL 
	 * if we don't have next, meaning that we are done with exploring */
	Inference* getNextInference() {
		InferenceNode *node = NULL;
		while (nodes->size() > 0) {
			node = nodes->back();
			if (node->getExplored()) {
				// Finish exploring this node
				// Remove the node from the stack
				nodes->pop_back();
				// Record this in the exploredSet
				commitNodeExplored(node);
			} else {
				node->setExplored(true);
				return node->getInference();
			}
		}
		return NULL;
	}

	
	/** Add one possible node that represents a fix for the current inference;
	 * @Return true if the node to add has not been explored yet
	 */
	bool addInference(Inference *infer) {
		InferenceNode *node = new InferenceNode(infer);
		node_set_t::iterator it = exploredSet->find(node);
		if (it == exploredSet->end()) {
			// We haven't explored this inference yet
			nodes->push_back(node);
			return true;
		} else {
			return false;
		}
	}

	MEMALLOC
} InferenceStack;


typedef struct scfence_priv {
	scfence_priv() {
		curInference = new Inference();
		candidateFile = NULL;
		inferImplicitMO = false;
		hasRestarted = false;
		implicitMOReadBound = DEFAULT_REPETITIVE_READ_BOUND;
	}

	/** The current inference */
	Inference *curInference;

	/** The stack of the InferenceNode we maintain for exploring */
	InferenceStack *stack;

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

	bool parseOption(char *opt);
	char* parseOptionHelper(char *opt, int *optIdx);
	/** Functions that work for infering the parameters */
	sync_paths_t *get_rf_sb_paths(const ModelAction *act1, const ModelAction *act2);
	void print_rf_sb_paths(sync_paths_t *paths, const ModelAction *start, const ModelAction *end);
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
	 * potentialResults list
	 */
	void addPotentialFixes(action_list_t *list);
	bool addFixesBuggyExecution(action_list_t *list);
	bool addFixesImplicitMO(action_list_t *list);
	bool addMoreCandidates(ModelList<Inference*> *existCandidates, ModelList<Inference*> *newCandidates, bool addStronger);
	bool addMoreCandidate(ModelList<Inference*> *existCandidates, Inference *newCandidate, bool addStronger);
	/** Get next inference from the potential result list */
	bool getNextCandidate();
	
	ModelList<Inference*>* imposeSync(ModelList<Inference*> *partialCandidates, sync_paths_t *paths);
	ModelList<Inference*>* imposeSC(ModelList<Inference*> *partialCandidates, const ModelAction *act1, const ModelAction *act2);
	const char* get_mo_str(memory_order order);
	void printWildcardResults(ModelList<Inference*> *results);
	void pruneResults();

	void restartModelChecker();
	void exitModelChecker();

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
	
	static scfence_priv *priv;

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

	static Inference *curInference;
	/** A list of possible results */
	static ModelList<Inference*> *potentialResults;
	/** A list of correct inference results */
	static ModelList<Inference*> *results;
	/** The file which provides a list of candidate wilcard inferences */
	static char *candidateFile;
	/** The swich of whether we consider the repetitive read to infer mo (_m) */
	static bool inferImplicitMO;
	/** Whether we have restarted the model checker */
	static bool hasRestarted;
	/** The bound above which we think that write should be the last write (_b) */
	static int implicitMOReadBound;
};
#endif
