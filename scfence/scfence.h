#ifndef _SCFENCE_H
#define _SCFENCE_H
#include "traceanalysis.h"
#include "scanalysis.h"
#include "hashtable.h"
#include "memoryorder.h"
#include "action.h"
#include "wildcard.h"
#include "model.h"

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
#define FENCE_PRINT print_nothing
#define ACT_PRINT(x) (x)
#define CV_PRINT(x) (x)
#endif


#define WILDCARD_ACT_PRINT(x)\
	FENCE_PRINT("Wildcard: %d\n", get_wildcard_id((x)->get_original_mo()));\
	ACT_PRINT(x);

/* Forward declaration */
class SCFence;

extern SCFence *scfence;

void print_nothing(const char *str, ...);

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
		ASSERT (infer->size > 0);
		orders = (memory_order *) model_malloc((infer->size + 1) * sizeof(memory_order*));
		this->size = infer->size;
		for (int i = 0; i <= size; i++)
			orders[i] = infer->orders[i];
	}

	void resize(int newsize) {
		ASSERT (newsize > size);
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
		FENCE_PRINT("\n");
		FENCE_PRINT("Strengthened wildcard:%d\n", wildcardID);
		WILDCARD_ACT_PRINT(act);
		FENCE_PRINT("-> %s\n", get_mo_str(mo));
		FENCE_PRINT("\n");
		return true;
	}


	/** @Return:
		1 -> 'this> infer';
		-1 -> 'this < infer'
		0 -> 'this == infer'
		INFERENCE_INCOMPARABLE(x) -> true means incomparable
	 */
	int compareTo(Inference *infer) {
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

typedef HashTable<memory_order, memory_order, memory_order, 4, model_malloc, model_calloc> wildcard_table_t;

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

	void parseOption(char *opt);
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
	bool addMoreCandidates(ModelList<Inference*> *existCandidates, ModelList<Inference*> *newCandidates);
	bool addMoreCandidate(ModelList<Inference*> *existCandidates, Inference *newCandidate);
	
	ModelList<Inference*>* imposeSync(ModelList<Inference*> *partialCandidates, sync_paths_t *paths);
	ModelList<Inference*>* imposeSC(ModelList<Inference*> *partialCandidates, const ModelAction *act1, const ModelAction *act2);
	const char* get_mo_str(memory_order order);
	void printWildcardResults(ModelList<Inference*> *results);
	void pruneResults();

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
	
	static Inference *curInference;
	/** A list of possible results */
	static ModelList<Inference*> *potentialResults;
	/** A list of correct inference results */
	static ModelList<Inference*> *results;
	/** The file which provides a list of candidate wilcard inferences */
	static char *candidateFile;
	/** The swich of whether we consider the repetitive read to infer mo (_m) */
	static bool inferImplicitMO;
	/** The bound above which we think that write should be the last write (_b) */
	static int implicitMOReadBound;
};
#endif
