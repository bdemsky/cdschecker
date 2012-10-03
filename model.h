/** @file model.h
 *  @brief Core model checker.
 */

#ifndef __MODEL_H__
#define __MODEL_H__

#include <list>
#include <vector>
#include <cstddef>
#include <ucontext.h>

#include "schedule.h"
#include "mymemory.h"
#include "libthreads.h"
#include "threads.h"
#include "action.h"
#include "clockvector.h"
#include "hashtable.h"
#include "workqueue.h"
#include "config.h"

/* Forward declaration */
class NodeStack;
class CycleGraph;
class Promise;

/** @brief Shorthand for a list of release sequence heads */
typedef std::vector< const ModelAction *, MyAlloc<const ModelAction *> > rel_heads_list_t;

/**
 * Model checker parameter structure. Holds run-time configuration options for
 * the model checker.
 */
struct model_params {
	int maxreads;
	int maxfuturedelay;
	unsigned int fairwindow;
	unsigned int enabledcount;
};

struct PendingFutureValue {
	uint64_t value;
	modelclock_t expiration;
	ModelAction * act;
};

/**
 * Structure for holding small ModelChecker members that should be snapshotted
 */
struct model_snapshot_members {
	ModelAction *current_action;
	int next_thread_id;
	modelclock_t used_sequence_numbers;
	Thread *nextThread;
	ModelAction *next_backtrack;
};

/** @brief The central structure for model-checking */
class ModelChecker {
public:
	ModelChecker(struct model_params params);
	~ModelChecker();

	/** @returns the context for the main model-checking system thread */
	ucontext_t * get_system_context() { return &system_context; }

	/** Prints an execution summary with trace information. */
	void print_summary();
#if SUPPORT_MOD_ORDER_DUMP
	void dumpGraph(char *filename);
#endif

	void add_thread(Thread *t);
	void remove_thread(Thread *t);
	Thread * get_thread(thread_id_t tid) { return thread_map->get(id_to_int(tid)); }
	Thread * get_thread(ModelAction *act) { return get_thread(act->get_tid()); }

	thread_id_t get_next_id();
	int get_num_threads();

	/** @return The currently executing Thread. */
	Thread * get_current_thread() { return scheduler->get_current_thread(); }

	int switch_to_master(ModelAction *act);
	ClockVector * get_cv(thread_id_t tid);
	ModelAction * get_parent_action(thread_id_t tid);
	bool next_execution();
	bool isfeasible();
	bool isfeasibleotherthanRMW();
	bool isfinalfeasible();
	void check_promises(ClockVector *old_cv, ClockVector * merge_cv);
	void get_release_seq_heads(ModelAction *act, rel_heads_list_t *release_heads);
	void finish_execution();
	bool isfeasibleprefix();
	void set_assert() {asserted=true;}

	/** @brief Alert the model-checker that an incorrectly-ordered
	 * synchronization was made */
	void set_bad_synchronization() { bad_synchronization = true; }

	const model_params params;

	MEMALLOC
private:
	/** The scheduler to use: tracks the running/ready Threads */
	Scheduler *scheduler;

	bool thin_air_constraint_may_allow(const ModelAction * writer, const ModelAction *reader);
	bool has_asserted() {return asserted;}
	void reset_asserted() {asserted=false;}
	int num_executions;
	int num_feasible_executions;
	bool promises_expired();

	modelclock_t get_next_seq_num();

	/**
	 * Stores the ModelAction for the current thread action.  Call this
	 * immediately before switching from user- to system-context to pass
	 * data between them.
	 * @param act The ModelAction created by the user-thread action
	 */
	void set_current_action(ModelAction *act) { priv->current_action = act; }
	Thread * check_current_action(ModelAction *curr);
	ModelAction * initialize_curr_action(ModelAction *curr);
	bool process_read(ModelAction *curr, bool second_part_of_rmw);
	bool process_write(ModelAction *curr);
	bool process_mutex(ModelAction *curr);
	bool process_thread_action(ModelAction *curr);
	bool check_action_enabled(ModelAction *curr);

	bool take_step();

	void check_recency(ModelAction *curr, const ModelAction *rf);
	ModelAction * get_last_conflict(ModelAction *act);
	void set_backtracking(ModelAction *act);
	Thread * get_next_thread(ModelAction *curr);
	ModelAction * get_next_backtrack();
	void reset_to_initial_state();
	bool resolve_promises(ModelAction *curr);
	void compute_promises(ModelAction *curr);

	void check_curr_backtracking(ModelAction * curr);
	void add_action_to_lists(ModelAction *act);
	ModelAction * get_last_action(thread_id_t tid) const;
	ModelAction * get_last_seq_cst(ModelAction *curr) const;
	ModelAction * get_last_unlock(ModelAction *curr) const;
	void build_reads_from_past(ModelAction *curr);
	ModelAction * process_rmw(ModelAction *curr);
	void post_r_modification_order(ModelAction *curr, const ModelAction *rf);
	bool r_modification_order(ModelAction *curr, const ModelAction *rf);
	bool w_modification_order(ModelAction *curr);
	bool release_seq_head(const ModelAction *rf, rel_heads_list_t *release_heads) const;
	bool resolve_release_sequences(void *location, work_queue_t *work_queue);
	void do_complete_join(ModelAction *join);

	ModelAction *diverge;
	ModelAction *earliest_diverge;

	ucontext_t system_context;
	action_list_t *action_trace;
	HashTable<int, Thread *, int> *thread_map;

	/** Per-object list of actions. Maps an object (i.e., memory location)
	 * to a trace of all actions performed on the object. */
	HashTable<const void *, action_list_t, uintptr_t, 4> *obj_map;

	/** Per-object list of actions. Maps an object (i.e., memory location)
	 * to a trace of all actions performed on the object. */
	HashTable<const void *, action_list_t, uintptr_t, 4> *lock_waiters_map;

	HashTable<void *, std::vector<action_list_t>, uintptr_t, 4 > *obj_thrd_map;
	std::vector<Promise *> *promises;
	std::vector<struct PendingFutureValue> *futurevalues;

	/**
	 * List of acquire actions that might synchronize with one or more
	 * release sequence. Release sequences might be determined lazily as
	 * promises are fulfilled and modification orders are established. Each
	 * ModelAction in this list must be an acquire operation.
	 */
	std::vector<ModelAction *> *pending_acq_rel_seq;

	std::vector<ModelAction *> *thrd_last_action;
	NodeStack *node_stack;

	/** Private data members that should be snapshotted. They are grouped
	 * together for efficiency and maintainability. */
	struct model_snapshot_members *priv;

	/**
	 * @brief The modification order graph
	 *
	 * A directed acyclic graph recording observations of the modification
	 * order on all the atomic objects in the system. This graph should
	 * never contain any cycles, as that represents a violation of the
	 * memory model (total ordering). This graph really consists of many
	 * disjoint (unconnected) subgraphs, each graph corresponding to a
	 * separate ordering on a distinct object.
	 *
	 * The edges in this graph represent the "ordered before" relation,
	 * such that <tt>a --> b</tt> means <tt>a</tt> was ordered before
	 * <tt>b</tt>.
	 */
	CycleGraph *mo_graph;
	bool failed_promise;
	bool too_many_reads;
	bool asserted;
	/** @brief Incorrectly-ordered synchronization was made */
	bool bad_synchronization;
};

extern ModelChecker *model;

#endif /* __MODEL_H__ */
