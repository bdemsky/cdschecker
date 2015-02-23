#ifndef _SCFENCE_H
#define _SCFENCE_H
#include "traceanalysis.h"
#include "scanalysis.h"
#include "hashtable.h"
#include "memoryorder.h"
#include "action.h"
#include "wildcard.h"
#include "inference.h"
#include "inferset.h"
#include "model.h"
#include "cyclegraph.h"

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
class InferenceList;
class PatchUnit;
class Patch;

extern SCFence *scfence;

typedef action_list_t path_t;
/** A list of load operations can represent the union of reads-from &
 * sequence-before edges; And we have a list of lists of load operations to
 * represent all the possible rfUsb paths between two nodes, defined as
 * syns_paths_t here
 */
typedef SnapList<path_t *> sync_paths_t;
typedef sync_paths_t paths_t;

typedef struct scfence_priv {
	scfence_priv() {
		inferenceSet = new InferenceSet();
		curInference = new Inference();
		candidateFile = NULL;
		inferImplicitMO = false;
		hasRestarted = false;
		implicitMOReadBound = DEFAULT_REPETITIVE_READ_BOUND;
		timeout = 0;
		gettimeofday(&lastRecordedTime, NULL);
	}

	/** The set of the InferenceNode we maintain for exploring */
	InferenceSet *inferenceSet;

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

	const CycleGraph *mo_graph;

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

	/** Initialize the search with a file with a list of potential candidates */
	void initializeByFile();

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

	/** The two important routine when we find or cannot find a fix for the
	 * current inference */
	void routineAfterAddFixes();

	bool routineBacktrack(bool feasible);

	/** A subroutine to find candidates for pattern (a) */
	InferenceList* getFixesFromPatternA(action_list_t *list, action_list_t::iterator readIter, action_list_t::iterator writeIter);

	/** A subroutine to find candidates for pattern (b) */
	InferenceList* getFixesFromPatternB(action_list_t *list, action_list_t::iterator readIter, action_list_t::iterator writeIter);

	/** When getting a non-SC execution, find potential fixes and add it to the
	 *  set */
	bool addFixesNonSC(action_list_t *list);

	/** When getting a buggy execution (we only target the uninitialized loads
	 * here), find potential fixes and add it to the set */
	bool addFixesBuggyExecution(action_list_t *list);

	/** When getting an SC and bug-free execution, we check whether we should
	 * fix the implicit mo problems. If so, find potential fixes and add it to
	 * the set */
	bool addFixesImplicitMO(action_list_t *list);

	/** General fixes wrapper */
	bool addFixes(action_list_t *list, fix_type_t type);

	/** Check whether a chosen reads-from path is a release sequence */
	bool isReleaseSequence(path_t *path);

	/** Impose synchronization to the existing list of inferences (inferList)
	 *  according to path, begin is the beginning operation, and end is the
	 *  ending operation. */
	//Inference* imposeSyncToInference(Inference *infer, path_t *path, bool &canUpdate, bool &hasUpdated);
	bool imposeSync(InferenceList* inferList, paths_t *paths, const
		ModelAction *begin, const ModelAction *end);
	
	bool imposeSync(InferenceList* inferList, path_t *path, const
		ModelAction *begin, const ModelAction *end);

	/** For a specific pair of write and read actions, figure out the possible
	 *  acq/rel fences that can impose hb plus the read & write sync pair */
	SnapVector<Patch*>* getAcqRel(const ModelAction *read,
		const ModelAction *readBound, const ModelAction *write,
		const ModelAction *writeBound);

	/** Impose SC to the existing list of inferences (inferList) by action1 &
	 *  action2. */
	bool imposeSC(InferenceList *inferList, const ModelAction *act1,
		const ModelAction *act2);

	/** When we finish model checking or cannot further strenghen with the
	 * current inference, we commit the current inference (the node at the back
	 * of the set) to be explored, pop it out of the set; if it is feasible,
	 * we put it in the result list */
	void commitInference(Inference *infer, bool feasible) {
		getSet()->commitInference(infer, feasible);	
	}

	/** Get the next available unexplored node; @Return NULL 
	 * if we don't have next, meaning that we are done with exploring */
	Inference* getNextInference() {
		return getSet()->getNextInference();
	}

	/** Add one possible node that represents a fix for the current inference;
	 * @Return true if the node to add has not been explored yet
	 */
	bool addInference(Inference *infer) {
		return getSet()->addInference(infer);
	}

	/** Add the list of fixes to the inference set. We will have to pass in the
	 *  current inference.;
	 * @Return true if the node to add has not been explored yet
	 */
	bool addCandidates(InferenceList *candidates) {
		return getSet()->addCandidates(getCurInference(), candidates);
	}

	void addCurInference() {
		getSet()->addCurInference(getCurInference());
	}

	/** Print the result of inferences  */
	void printResults() {
		getSet()->printResults();
	}

	/** Print the candidates of inferences  */
	void printCandidates() {
		getSet()->printCandidates();
	}

	/** The number of nodes in the set (including those parent nodes (set as
	 * explored) */
	 int setSize() {
		return getSet()->getCandidatesSize();
	 }

	/** Set the restart flag of the model checker in order to restart the
	 * checking process */
	void restartModelChecker();
	
	/** Set the exit flag of the model checker in order to exit the whole
	 * process */
	void exitModelChecker();

	bool modelCheckerAtExitState();

	const char* net_mo_str(memory_order order);

	InferenceSet* getSet() {
		return priv->inferenceSet;
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

	void setBuggy(bool val) {
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
