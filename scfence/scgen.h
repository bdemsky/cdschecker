#ifndef _SCGEN_H
#define _SCGEN_H
#include "hashtable.h"
#include "memoryorder.h"
#include "action.h"
#include "model.h"

#include <sys/time.h>

typedef struct SCGeneratorOption {
	bool print_always;
	bool print_buggy;
	bool print_nonsc;
	bool time;
	bool annotationMode;
} SCGeneratorOption;


class SCGenerator {
 public:
	SCGenerator(action_list_t *actions);
	~SCGenerator();

	action_list_t * generateSC(action_list_t *);
	void print_list(action_list_t *list);
	void printCV(action_list_t *);

	SCGeneratorOption* getOption();
	
	SNAPSHOTALLOC
 private:
	/********************** SC-related stuff (beginning) **********************/
	SCGeneratorOption *option;
	ModelExecution *execution;

	bool fastVersion;
	bool allowNonSC;

	void update_stats();
	int buildVectors(SnapVector<action_list_t> *threadlist, int *maxthread, action_list_t *);
	bool updateConstraints(ModelAction *act);
	void computeCV(action_list_t *);


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

	/** A set of actions that should be ignored in the partially SC analysis */
	HashTable<const ModelAction*, const ModelAction*, uintptr_t, 4> ignoredActions;
	int ignoredActionSize;
};
#endif
