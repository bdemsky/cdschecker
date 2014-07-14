#ifndef _SCFENCE_H
#define _SCFENCE_H
#include "traceanalysis.h"
#include "scanalysis.h"
#include "hashtable.h"
#include "memoryorder.h"
#include "action.h"
#include "wildcard.h"

#ifdef __cplusplus
using std::memory_order;
#endif

//#define FENCE_OUTPUT

#ifdef FENCE_OUTPUT
#define FENCE_PRINT model_print
#define ACT_PRINT(x) (x)->print()
#define CV_PRINT(x) (x)->print()
#else
#define FENCE_PRINT print_nothing
#define ACT_PRINT(x) (x)
#define CV_PRINT(x) (x)
#endif

/* Forward declaration */
class SCFence;

extern SCFence *scfence;

void print_nothing(const char *str, ...);

typedef struct {
	memory_order wildcard;
	memory_order order;

	MEMALLOC
} InferencePair;

/** A list of load operations can represent the union of reads-from &
 * sequence-before edges; And we have a list of lists of load operations to
 * represent all the possible rfUsb paths between two nodes, defined as
 * syns_paths_t here
 */
typedef SnapList<action_list_t *> sync_paths_t;

typedef SnapList<const ModelAction *> const_actions_t;
typedef HashTable<memory_order, memory_order, memory_order, 4> wildcard_table_t;
typedef SnapList<InferencePair> inference_list_t;
typedef SnapList<memory_order> wildcard_list_t;

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

	/* Should NOT use SNAPSHOTALLOC here */
	SNAPSHOTALLOC
	//MEMALLOC

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

	/** Functions that work for infering the parameters */
	sync_paths_t *get_rf_sb_paths(const ModelAction *act1, const ModelAction *act2);
	void printPatternFixes(action_list_t *list);
	void print_rf_sb_paths(sync_paths_t *path, const ModelAction *start, const ModelAction *end);
	bool isSCEdge(const ModelAction *from, const ModelAction *to) {
		return from->is_seqcst() && to->is_seqcst();
	}

	bool isConflicting(const ModelAction *act1, const ModelAction *act2) {
		return act1->get_location() == act2->get_location() ? (act1->is_write()
			|| act2->is_write()) : false;
	}


	void breakCycle(const ModelAction *act1, const ModelAction *act2);
	const char* get_mo_str(memory_order order);
	void printWildcardResult(inference_list_t *result);

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
	
	static int restartCnt;

	/** Current wildcard mapping: a wildcard -> the specifc ordering */
	wildcard_table_t *curWildcardMap;
	/** Current wildcards */
	inference_list_t *curInference;

	/** A list of possible results */
	SnapList<inference_list_t *> potentialResults;

};
#endif
