#include <stdio.h>
#include <algorithm>

#include "model.h"
#include "action.h"
#include "nodestack.h"
#include "schedule.h"
#include "snapshot-interface.h"
#include "common.h"
#include "clockvector.h"
#include "cyclegraph.h"
#include "promise.h"
#include "datarace.h"
#include "mutex.h"

#define INITIAL_THREAD_ID	0

ModelChecker *model;

/** @brief Constructor */
ModelChecker::ModelChecker(struct model_params params) :
	/* Initialize default scheduler */
	params(params),
	scheduler(new Scheduler()),
	num_executions(0),
	num_feasible_executions(0),
	diverge(NULL),
	earliest_diverge(NULL),
	action_trace(new action_list_t()),
	thread_map(new HashTable<int, Thread *, int>()),
	obj_map(new HashTable<const void *, action_list_t, uintptr_t, 4>()),
	lock_waiters_map(new HashTable<const void *, action_list_t, uintptr_t, 4>()),
	obj_thrd_map(new HashTable<void *, std::vector<action_list_t>, uintptr_t, 4 >()),
	promises(new std::vector<Promise *>()),
	futurevalues(new std::vector<struct PendingFutureValue>()),
	pending_acq_rel_seq(new std::vector<ModelAction *>()),
	thrd_last_action(new std::vector<ModelAction *>(1)),
	node_stack(new NodeStack()),
	mo_graph(new CycleGraph()),
	failed_promise(false),
	too_many_reads(false),
	asserted(false),
	bad_synchronization(false)
{
	/* Allocate this "size" on the snapshotting heap */
	priv = (struct model_snapshot_members *)calloc(1, sizeof(*priv));
	/* First thread created will have id INITIAL_THREAD_ID */
	priv->next_thread_id = INITIAL_THREAD_ID;
}

/** @brief Destructor */
ModelChecker::~ModelChecker()
{
	for (int i = 0; i < get_num_threads(); i++)
		delete thread_map->get(i);
	delete thread_map;

	delete obj_thrd_map;
	delete obj_map;
	delete lock_waiters_map;
	delete action_trace;

	for (unsigned int i = 0; i < promises->size(); i++)
		delete (*promises)[i];
	delete promises;

	delete pending_acq_rel_seq;

	delete thrd_last_action;
	delete node_stack;
	delete scheduler;
	delete mo_graph;
}

/**
 * Restores user program to initial state and resets all model-checker data
 * structures.
 */
void ModelChecker::reset_to_initial_state()
{
	DEBUG("+++ Resetting to initial state +++\n");
	node_stack->reset_execution();
	failed_promise = false;
	too_many_reads = false;
	bad_synchronization = false;
	reset_asserted();
	snapshotObject->backTrackBeforeStep(0);
}

/** @return a thread ID for a new Thread */
thread_id_t ModelChecker::get_next_id()
{
	return priv->next_thread_id++;
}

/** @return the number of user threads created during this execution */
int ModelChecker::get_num_threads()
{
	return priv->next_thread_id;
}

/** @return a sequence number for a new ModelAction */
modelclock_t ModelChecker::get_next_seq_num()
{
	return ++priv->used_sequence_numbers;
}

/**
 * @brief Choose the next thread to execute.
 *
 * This function chooses the next thread that should execute. It can force the
 * adjacency of read/write portions of a RMW action, force THREAD_CREATE to be
 * followed by a THREAD_START, or it can enforce execution replay/backtracking.
 * The model-checker may have no preference regarding the next thread (i.e.,
 * when exploring a new execution ordering), in which case this will return
 * NULL.
 * @param curr The current ModelAction. This action might guide the choice of
 * next thread.
 * @return The next thread to run. If the model-checker has no preference, NULL.
 */
Thread * ModelChecker::get_next_thread(ModelAction *curr)
{
	thread_id_t tid;

	if (curr!=NULL) {
		/* Do not split atomic actions. */
		if (curr->is_rmwr())
			return thread_current();
		/* The THREAD_CREATE action points to the created Thread */
		else if (curr->get_type() == THREAD_CREATE)
			return (Thread *)curr->get_location();
	}

	/* Have we completed exploring the preselected path? */
	if (diverge == NULL)
		return NULL;

	/* Else, we are trying to replay an execution */
	ModelAction *next = node_stack->get_next()->get_action();

	if (next == diverge) {
		if (earliest_diverge == NULL || *diverge < *earliest_diverge)
			earliest_diverge=diverge;

		Node *nextnode = next->get_node();
		/* Reached divergence point */
		if (nextnode->increment_promise()) {
			/* The next node will try to satisfy a different set of promises. */
			tid = next->get_tid();
			node_stack->pop_restofstack(2);
		} else if (nextnode->increment_read_from()) {
			/* The next node will read from a different value. */
			tid = next->get_tid();
			node_stack->pop_restofstack(2);
		} else if (nextnode->increment_future_value()) {
			/* The next node will try to read from a different future value. */
			tid = next->get_tid();
			node_stack->pop_restofstack(2);
		} else {
			/* Make a different thread execute for next step */
			Node *node = nextnode->get_parent();
			tid = node->get_next_backtrack();
			node_stack->pop_restofstack(1);
			if (diverge==earliest_diverge) {
				earliest_diverge=node->get_action();
			}
		}
		DEBUG("*** Divergence point ***\n");

		diverge = NULL;
	} else {
		tid = next->get_tid();
	}
	DEBUG("*** ModelChecker chose next thread = %d ***\n", tid);
	ASSERT(tid != THREAD_ID_T_NONE);
	return thread_map->get(id_to_int(tid));
}

/**
 * Queries the model-checker for more executions to explore and, if one
 * exists, resets the model-checker state to execute a new execution.
 *
 * @return If there are more executions to explore, return true. Otherwise,
 * return false.
 */
bool ModelChecker::next_execution()
{
	DBG();

	num_executions++;

	if (isfinalfeasible()) {
		printf("Earliest divergence point since last feasible execution:\n");
		if (earliest_diverge)
			earliest_diverge->print(false);
		else
			printf("(Not set)\n");

		earliest_diverge = NULL;
		num_feasible_executions++;
	}

	DEBUG("Number of acquires waiting on pending release sequences: %lu\n",
			pending_acq_rel_seq->size());

	if (isfinalfeasible() || DBG_ENABLED())
		print_summary();

	if ((diverge = get_next_backtrack()) == NULL)
		return false;

	if (DBG_ENABLED()) {
		printf("Next execution will diverge at:\n");
		diverge->print();
	}

	reset_to_initial_state();
	return true;
}

ModelAction * ModelChecker::get_last_conflict(ModelAction *act)
{
	switch (act->get_type()) {
	case ATOMIC_READ:
	case ATOMIC_WRITE:
	case ATOMIC_RMW: {
		/* linear search: from most recent to oldest */
		action_list_t *list = obj_map->get_safe_ptr(act->get_location());
		action_list_t::reverse_iterator rit;
		for (rit = list->rbegin(); rit != list->rend(); rit++) {
			ModelAction *prev = *rit;
			if (act->is_synchronizing(prev))
				return prev;
		}
		break;
	}
	case ATOMIC_LOCK:
	case ATOMIC_TRYLOCK: {
		/* linear search: from most recent to oldest */
		action_list_t *list = obj_map->get_safe_ptr(act->get_location());
		action_list_t::reverse_iterator rit;
		for (rit = list->rbegin(); rit != list->rend(); rit++) {
			ModelAction *prev = *rit;
			if (act->is_conflicting_lock(prev))
				return prev;
		}
		break;
	}
	case ATOMIC_UNLOCK: {
		/* linear search: from most recent to oldest */
		action_list_t *list = obj_map->get_safe_ptr(act->get_location());
		action_list_t::reverse_iterator rit;
		for (rit = list->rbegin(); rit != list->rend(); rit++) {
			ModelAction *prev = *rit;
			if (!act->same_thread(prev)&&prev->is_failed_trylock())
				return prev;
		}
		break;
	}
	default:
		break;
	}
	return NULL;
}

/** This method find backtracking points where we should try to
 * reorder the parameter ModelAction against.
 *
 * @param the ModelAction to find backtracking points for.
 */
void ModelChecker::set_backtracking(ModelAction *act)
{
	Thread *t = get_thread(act);
	ModelAction * prev = get_last_conflict(act);
	if (prev == NULL)
		return;

	Node * node = prev->get_node()->get_parent();

	int low_tid, high_tid;
	if (node->is_enabled(t)) {
		low_tid = id_to_int(act->get_tid());
		high_tid = low_tid+1;
	} else {
		low_tid = 0;
		high_tid = get_num_threads();
	}

	for(int i = low_tid; i < high_tid; i++) {
		thread_id_t tid = int_to_id(i);
		if (!node->is_enabled(tid))
			continue;

		/* Check if this has been explored already */
		if (node->has_been_explored(tid))
			continue;

		/* See if fairness allows */
		if (model->params.fairwindow != 0 && !node->has_priority(tid)) {
			bool unfair=false;
			for(int t=0;t<node->get_num_threads();t++) {
				thread_id_t tother=int_to_id(t);
				if (node->is_enabled(tother) && node->has_priority(tother)) {
					unfair=true;
					break;
				}
			}
			if (unfair)
				continue;
		}

		/* Cache the latest backtracking point */
		if (!priv->next_backtrack || *prev > *priv->next_backtrack)
			priv->next_backtrack = prev;

		/* If this is a new backtracking point, mark the tree */
		if (!node->set_backtrack(tid))
			continue;
		DEBUG("Setting backtrack: conflict = %d, instead tid = %d\n",
					prev->get_tid(), t->get_id());
		if (DBG_ENABLED()) {
			prev->print();
			act->print();
		}
	}
}

/**
 * Returns last backtracking point. The model checker will explore a different
 * path for this point in the next execution.
 * @return The ModelAction at which the next execution should diverge.
 */
ModelAction * ModelChecker::get_next_backtrack()
{
	ModelAction *next = priv->next_backtrack;
	priv->next_backtrack = NULL;
	return next;
}

/**
 * Processes a read or rmw model action.
 * @param curr is the read model action to process.
 * @param second_part_of_rmw is boolean that is true is this is the second action of a rmw.
 * @return True if processing this read updates the mo_graph.
 */
bool ModelChecker::process_read(ModelAction *curr, bool second_part_of_rmw)
{
	uint64_t value;
	bool updated = false;
	while (true) {
		const ModelAction *reads_from = curr->get_node()->get_read_from();
		if (reads_from != NULL) {
			mo_graph->startChanges();

			value = reads_from->get_value();
			bool r_status = false;

			if (!second_part_of_rmw) {
				check_recency(curr, reads_from);
				r_status = r_modification_order(curr, reads_from);
			}


			if (!second_part_of_rmw&&!isfeasible()&&(curr->get_node()->increment_read_from()||curr->get_node()->increment_future_value())) {
				mo_graph->rollbackChanges();
				too_many_reads = false;
				continue;
			}

			curr->read_from(reads_from);
			mo_graph->commitChanges();
			updated |= r_status;
		} else if (!second_part_of_rmw) {
			/* Read from future value */
			value = curr->get_node()->get_future_value();
			modelclock_t expiration = curr->get_node()->get_future_value_expiration();
			curr->read_from(NULL);
			Promise *valuepromise = new Promise(curr, value, expiration);
			promises->push_back(valuepromise);
		}
		get_thread(curr)->set_return_value(value);
		return updated;
	}
}

/**
 * Processes a lock, trylock, or unlock model action.  @param curr is
 * the read model action to process.
 *
 * The try lock operation checks whether the lock is taken.  If not,
 * it falls to the normal lock operation case.  If so, it returns
 * fail.
 *
 * The lock operation has already been checked that it is enabled, so
 * it just grabs the lock and synchronizes with the previous unlock.
 *
 * The unlock operation has to re-enable all of the threads that are
 * waiting on the lock.
 *
 * @return True if synchronization was updated; false otherwise
 */
bool ModelChecker::process_mutex(ModelAction *curr) {
	std::mutex *mutex = (std::mutex *)curr->get_location();
	struct std::mutex_state *state = mutex->get_state();
	switch (curr->get_type()) {
	case ATOMIC_TRYLOCK: {
		bool success = !state->islocked;
		curr->set_try_lock(success);
		if (!success) {
			get_thread(curr)->set_return_value(0);
			break;
		}
		get_thread(curr)->set_return_value(1);
	}
		//otherwise fall into the lock case
	case ATOMIC_LOCK: {
		if (curr->get_cv()->getClock(state->alloc_tid) <= state->alloc_clock) {
			printf("Lock access before initialization\n");
			set_assert();
		}
		state->islocked = true;
		ModelAction *unlock = get_last_unlock(curr);
		//synchronize with the previous unlock statement
		if (unlock != NULL) {
			curr->synchronize_with(unlock);
			return true;
		}
		break;
	}
	case ATOMIC_UNLOCK: {
		//unlock the lock
		state->islocked = false;
		//wake up the other threads
		action_list_t *waiters = lock_waiters_map->get_safe_ptr(curr->get_location());
		//activate all the waiting threads
		for (action_list_t::iterator rit = waiters->begin(); rit != waiters->end(); rit++) {
			scheduler->wake(get_thread(*rit));
		}
		waiters->clear();
		break;
	}
	default:
		ASSERT(0);
	}
	return false;
}

/**
 * Process a write ModelAction
 * @param curr The ModelAction to process
 * @return True if the mo_graph was updated or promises were resolved
 */
bool ModelChecker::process_write(ModelAction *curr)
{
	bool updated_mod_order = w_modification_order(curr);
	bool updated_promises = resolve_promises(curr);

	if (promises->size() == 0) {
		for (unsigned int i = 0; i < futurevalues->size(); i++) {
			struct PendingFutureValue pfv = (*futurevalues)[i];
			if (pfv.act->get_node()->add_future_value(pfv.value, pfv.expiration) &&
					(!priv->next_backtrack || *pfv.act > *priv->next_backtrack))
				priv->next_backtrack = pfv.act;
		}
		futurevalues->resize(0);
	}

	mo_graph->commitChanges();
	get_thread(curr)->set_return_value(VALUE_NONE);
	return updated_mod_order || updated_promises;
}

/**
 * @brief Process the current action for thread-related activity
 *
 * Performs current-action processing for a THREAD_* ModelAction. Proccesses
 * may include setting Thread status, completing THREAD_FINISH/THREAD_JOIN
 * synchronization, etc.  This function is a no-op for non-THREAD actions
 * (e.g., ATOMIC_{READ,WRITE,RMW,LOCK}, etc.)
 *
 * @param curr The current action
 * @return True if synchronization was updated or a thread completed
 */
bool ModelChecker::process_thread_action(ModelAction *curr)
{
	bool updated = false;

	switch (curr->get_type()) {
	case THREAD_CREATE: {
		Thread *th = (Thread *)curr->get_location();
		th->set_creation(curr);
		break;
	}
	case THREAD_JOIN: {
		Thread *waiting, *blocking;
		waiting = get_thread(curr);
		blocking = (Thread *)curr->get_location();
		if (!blocking->is_complete()) {
			blocking->push_wait_list(curr);
			scheduler->sleep(waiting);
		} else {
			do_complete_join(curr);
			updated = true; /* trigger rel-seq checks */
		}
		break;
	}
	case THREAD_FINISH: {
		Thread *th = get_thread(curr);
		while (!th->wait_list_empty()) {
			ModelAction *act = th->pop_wait_list();
			Thread *wake = get_thread(act);
			scheduler->wake(wake);
			do_complete_join(act);
			updated = true; /* trigger rel-seq checks */
		}
		th->complete();
		updated = true; /* trigger rel-seq checks */
		break;
	}
	case THREAD_START: {
		check_promises(NULL, curr->get_cv());
		break;
	}
	default:
		break;
	}

	return updated;
}

/**
 * Initialize the current action by performing one or more of the following
 * actions, as appropriate: merging RMWR and RMWC/RMW actions, stepping forward
 * in the NodeStack, manipulating backtracking sets, allocating and
 * initializing clock vectors, and computing the promises to fulfill.
 *
 * @param curr The current action, as passed from the user context; may be
 * freed/invalidated after the execution of this function
 * @return The current action, as processed by the ModelChecker. Is only the
 * same as the parameter @a curr if this is a newly-explored action.
 */
ModelAction * ModelChecker::initialize_curr_action(ModelAction *curr)
{
	ModelAction *newcurr;

	if (curr->is_rmwc() || curr->is_rmw()) {
		newcurr = process_rmw(curr);
		delete curr;

		if (newcurr->is_rmw())
			compute_promises(newcurr);
		return newcurr;
	}

	curr->set_seq_number(get_next_seq_num());

	newcurr = node_stack->explore_action(curr, scheduler->get_enabled());
	if (newcurr) {
		/* First restore type and order in case of RMW operation */
		if (curr->is_rmwr())
			newcurr->copy_typeandorder(curr);

		ASSERT(curr->get_location() == newcurr->get_location());
		newcurr->copy_from_new(curr);

		/* Discard duplicate ModelAction; use action from NodeStack */
		delete curr;

		/* Always compute new clock vector */
		newcurr->create_cv(get_parent_action(newcurr->get_tid()));
	} else {
		newcurr = curr;

		/* Always compute new clock vector */
		newcurr->create_cv(get_parent_action(newcurr->get_tid()));
		/*
		 * Perform one-time actions when pushing new ModelAction onto
		 * NodeStack
		 */
		if (newcurr->is_write())
			compute_promises(newcurr);
	}
	return newcurr;
}

/**
 * This method checks whether a model action is enabled at the given point.
 * At this point, it checks whether a lock operation would be successful at this point.
 * If not, it puts the thread in a waiter list.
 * @param curr is the ModelAction to check whether it is enabled.
 * @return a bool that indicates whether the action is enabled.
 */
bool ModelChecker::check_action_enabled(ModelAction *curr) {
	if (curr->is_lock()) {
		std::mutex * lock = (std::mutex *)curr->get_location();
		struct std::mutex_state * state = lock->get_state();
		if (state->islocked) {
			//Stick the action in the appropriate waiting queue
			lock_waiters_map->get_safe_ptr(curr->get_location())->push_back(curr);
			return false;
		}
	}

	return true;
}

/**
 * This is the heart of the model checker routine. It performs model-checking
 * actions corresponding to a given "current action." Among other processes, it
 * calculates reads-from relationships, updates synchronization clock vectors,
 * forms a memory_order constraints graph, and handles replay/backtrack
 * execution when running permutations of previously-observed executions.
 *
 * @param curr The current action to process
 * @return The next Thread that must be executed. May be NULL if ModelChecker
 * makes no choice (e.g., according to replay execution, combining RMW actions,
 * etc.)
 */
Thread * ModelChecker::check_current_action(ModelAction *curr)
{
	ASSERT(curr);

	bool second_part_of_rmw = curr->is_rmwc() || curr->is_rmw();

	if (!check_action_enabled(curr)) {
		/* Make the execution look like we chose to run this action
		 * much later, when a lock is actually available to release */
		get_current_thread()->set_pending(curr);
		scheduler->sleep(get_current_thread());
		return get_next_thread(NULL);
	}

	ModelAction *newcurr = initialize_curr_action(curr);

	/* Add the action to lists before any other model-checking tasks */
	if (!second_part_of_rmw)
		add_action_to_lists(newcurr);

	/* Build may_read_from set for newly-created actions */
	if (curr == newcurr && curr->is_read())
		build_reads_from_past(curr);
	curr = newcurr;

	/* Initialize work_queue with the "current action" work */
	work_queue_t work_queue(1, CheckCurrWorkEntry(curr));

	while (!work_queue.empty()) {
		WorkQueueEntry work = work_queue.front();
		work_queue.pop_front();

		switch (work.type) {
		case WORK_CHECK_CURR_ACTION: {
			ModelAction *act = work.action;
			bool update = false; /* update this location's release seq's */
			bool update_all = false; /* update all release seq's */

			if (process_thread_action(curr))
				update_all = true;

			if (act->is_read() && process_read(act, second_part_of_rmw))
				update = true;

			if (act->is_write() && process_write(act))
				update = true;

			if (act->is_mutex_op() && process_mutex(act))
				update_all = true;

			if (update_all)
				work_queue.push_back(CheckRelSeqWorkEntry(NULL));
			else if (update)
				work_queue.push_back(CheckRelSeqWorkEntry(act->get_location()));
			break;
		}
		case WORK_CHECK_RELEASE_SEQ:
			resolve_release_sequences(work.location, &work_queue);
			break;
		case WORK_CHECK_MO_EDGES: {
			/** @todo Complete verification of work_queue */
			ModelAction *act = work.action;
			bool updated = false;

			if (act->is_read()) {
				const ModelAction *rf = act->get_reads_from();
				if (rf != NULL && r_modification_order(act, rf))
					updated = true;
			}
			if (act->is_write()) {
				if (w_modification_order(act))
					updated = true;
			}
			mo_graph->commitChanges();

			if (updated)
				work_queue.push_back(CheckRelSeqWorkEntry(act->get_location()));
			break;
		}
		default:
			ASSERT(false);
			break;
		}
	}

	check_curr_backtracking(curr);

	set_backtracking(curr);

	return get_next_thread(curr);
}

/**
 * Complete a THREAD_JOIN operation, by synchronizing with the THREAD_FINISH
 * operation from the Thread it is joining with. Must be called after the
 * completion of the Thread in question.
 * @param join The THREAD_JOIN action
 */
void ModelChecker::do_complete_join(ModelAction *join)
{
	Thread *blocking = (Thread *)join->get_location();
	ModelAction *act = get_last_action(blocking->get_id());
	join->synchronize_with(act);
}

void ModelChecker::check_curr_backtracking(ModelAction * curr) {
	Node *currnode = curr->get_node();
	Node *parnode = currnode->get_parent();

	if ((!parnode->backtrack_empty() ||
			 !currnode->read_from_empty() ||
			 !currnode->future_value_empty() ||
			 !currnode->promise_empty())
			&& (!priv->next_backtrack ||
					*curr > *priv->next_backtrack)) {
		priv->next_backtrack = curr;
	}
}

bool ModelChecker::promises_expired() {
	for (unsigned int promise_index = 0; promise_index < promises->size(); promise_index++) {
		Promise *promise = (*promises)[promise_index];
		if (promise->get_expiration()<priv->used_sequence_numbers) {
			return true;
		}
	}
	return false;
}

/** @return whether the current partial trace must be a prefix of a
 * feasible trace. */
bool ModelChecker::isfeasibleprefix() {
	return promises->size() == 0 && pending_acq_rel_seq->size() == 0;
}

/** @return whether the current partial trace is feasible. */
bool ModelChecker::isfeasible() {
	if (DBG_ENABLED() && mo_graph->checkForRMWViolation())
		DEBUG("Infeasible: RMW violation\n");

	return !mo_graph->checkForRMWViolation() && isfeasibleotherthanRMW();
}

/** @return whether the current partial trace is feasible other than
 * multiple RMW reading from the same store. */
bool ModelChecker::isfeasibleotherthanRMW() {
	if (DBG_ENABLED()) {
		if (mo_graph->checkForCycles())
			DEBUG("Infeasible: modification order cycles\n");
		if (failed_promise)
			DEBUG("Infeasible: failed promise\n");
		if (too_many_reads)
			DEBUG("Infeasible: too many reads\n");
		if (bad_synchronization)
			DEBUG("Infeasible: bad synchronization ordering\n");
		if (promises_expired())
			DEBUG("Infeasible: promises expired\n");
	}
	return !mo_graph->checkForCycles() && !failed_promise && !too_many_reads && !bad_synchronization && !promises_expired();
}

/** Returns whether the current completed trace is feasible. */
bool ModelChecker::isfinalfeasible() {
	if (DBG_ENABLED() && promises->size() != 0)
		DEBUG("Infeasible: unrevolved promises\n");

	return isfeasible() && promises->size() == 0;
}

/** Close out a RMWR by converting previous RMWR into a RMW or READ. */
ModelAction * ModelChecker::process_rmw(ModelAction *act) {
	int tid = id_to_int(act->get_tid());
	ModelAction *lastread = get_last_action(tid);
	lastread->process_rmw(act);
	if (act->is_rmw() && lastread->get_reads_from()!=NULL) {
		mo_graph->addRMWEdge(lastread->get_reads_from(), lastread);
		mo_graph->commitChanges();
	}
	return lastread;
}

/**
 * Checks whether a thread has read from the same write for too many times
 * without seeing the effects of a later write.
 *
 * Basic idea:
 * 1) there must a different write that we could read from that would satisfy the modification order,
 * 2) we must have read from the same value in excess of maxreads times, and
 * 3) that other write must have been in the reads_from set for maxreads times.
 *
 * If so, we decide that the execution is no longer feasible.
 */
void ModelChecker::check_recency(ModelAction *curr, const ModelAction *rf) {
	if (params.maxreads != 0) {

		if (curr->get_node()->get_read_from_size() <= 1)
			return;
		//Must make sure that execution is currently feasible...  We could
		//accidentally clear by rolling back
		if (!isfeasible())
			return;
		std::vector<action_list_t> *thrd_lists = obj_thrd_map->get_safe_ptr(curr->get_location());
		int tid = id_to_int(curr->get_tid());

		/* Skip checks */
		if ((int)thrd_lists->size() <= tid)
			return;
		action_list_t *list = &(*thrd_lists)[tid];

		action_list_t::reverse_iterator rit = list->rbegin();
		/* Skip past curr */
		for (; (*rit) != curr; rit++)
			;
		/* go past curr now */
		rit++;

		action_list_t::reverse_iterator ritcopy = rit;
		//See if we have enough reads from the same value
		int count = 0;
		for (; count < params.maxreads; rit++,count++) {
			if (rit==list->rend())
				return;
			ModelAction *act = *rit;
			if (!act->is_read())
				return;
			
			if (act->get_reads_from() != rf)
				return;
			if (act->get_node()->get_read_from_size() <= 1)
				return;
		}
		for (int i = 0; i<curr->get_node()->get_read_from_size(); i++) {
			//Get write
			const ModelAction * write = curr->get_node()->get_read_from_at(i);

			//Need a different write
			if (write==rf)
				continue;

			/* Test to see whether this is a feasible write to read from*/
			mo_graph->startChanges();
			r_modification_order(curr, write);
			bool feasiblereadfrom = isfeasible();
			mo_graph->rollbackChanges();

			if (!feasiblereadfrom)
				continue;
			rit = ritcopy;

			bool feasiblewrite = true;
			//new we need to see if this write works for everyone

			for (int loop = count; loop>0; loop--,rit++) {
				ModelAction *act=*rit;
				bool foundvalue = false;
				for (int j = 0; j<act->get_node()->get_read_from_size(); j++) {
					if (act->get_node()->get_read_from_at(i)==write) {
						foundvalue = true;
						break;
					}
				}
				if (!foundvalue) {
					feasiblewrite = false;
					break;
				}
			}
			if (feasiblewrite) {
				too_many_reads = true;
				return;
			}
		}
	}
}

/**
 * Updates the mo_graph with the constraints imposed from the current
 * read.
 *
 * Basic idea is the following: Go through each other thread and find
 * the lastest action that happened before our read.  Two cases:
 *
 * (1) The action is a write => that write must either occur before
 * the write we read from or be the write we read from.
 *
 * (2) The action is a read => the write that that action read from
 * must occur before the write we read from or be the same write.
 *
 * @param curr The current action. Must be a read.
 * @param rf The action that curr reads from. Must be a write.
 * @return True if modification order edges were added; false otherwise
 */
bool ModelChecker::r_modification_order(ModelAction *curr, const ModelAction *rf)
{
	std::vector<action_list_t> *thrd_lists = obj_thrd_map->get_safe_ptr(curr->get_location());
	unsigned int i;
	bool added = false;
	ASSERT(curr->is_read());

	/* Iterate over all threads */
	for (i = 0; i < thrd_lists->size(); i++) {
		/* Iterate over actions in thread, starting from most recent */
		action_list_t *list = &(*thrd_lists)[i];
		action_list_t::reverse_iterator rit;
		for (rit = list->rbegin(); rit != list->rend(); rit++) {
			ModelAction *act = *rit;

			/*
			 * Include at most one act per-thread that "happens
			 * before" curr. Don't consider reflexively.
			 */
			if (act->happens_before(curr) && act != curr) {
				if (act->is_write()) {
					if (rf != act) {
						mo_graph->addEdge(act, rf);
						added = true;
					}
				} else {
					const ModelAction *prevreadfrom = act->get_reads_from();
					if (prevreadfrom != NULL && rf != prevreadfrom) {
						mo_graph->addEdge(prevreadfrom, rf);
						added = true;
					}
				}
				break;
			}
		}
	}

	return added;
}

/** This method fixes up the modification order when we resolve a
 *  promises.  The basic problem is that actions that occur after the
 *  read curr could not property add items to the modification order
 *  for our read.
 *
 *  So for each thread, we find the earliest item that happens after
 *  the read curr.  This is the item we have to fix up with additional
 *  constraints.  If that action is write, we add a MO edge between
 *  the Action rf and that action.  If the action is a read, we add a
 *  MO edge between the Action rf, and whatever the read accessed.
 *
 * @param curr is the read ModelAction that we are fixing up MO edges for.
 * @param rf is the write ModelAction that curr reads from.
 *
 */
void ModelChecker::post_r_modification_order(ModelAction *curr, const ModelAction *rf)
{
	std::vector<action_list_t> *thrd_lists = obj_thrd_map->get_safe_ptr(curr->get_location());
	unsigned int i;
	ASSERT(curr->is_read());

	/* Iterate over all threads */
	for (i = 0; i < thrd_lists->size(); i++) {
		/* Iterate over actions in thread, starting from most recent */
		action_list_t *list = &(*thrd_lists)[i];
		action_list_t::reverse_iterator rit;
		ModelAction *lastact = NULL;

		/* Find last action that happens after curr that is either not curr or a rmw */
		for (rit = list->rbegin(); rit != list->rend(); rit++) {
			ModelAction *act = *rit;
			if (curr->happens_before(act) && (curr != act || curr->is_rmw())) {
				lastact = act;
			} else
				break;
		}

			/* Include at most one act per-thread that "happens before" curr */
		if (lastact != NULL) {
			if (lastact==curr) {
				//Case 1: The resolved read is a RMW, and we need to make sure
				//that the write portion of the RMW mod order after rf

				mo_graph->addEdge(rf, lastact);
			} else if (lastact->is_read()) {
				//Case 2: The resolved read is a normal read and the next
				//operation is a read, and we need to make sure the value read
				//is mod ordered after rf

				const ModelAction *postreadfrom = lastact->get_reads_from();
				if (postreadfrom != NULL&&rf != postreadfrom)
					mo_graph->addEdge(rf, postreadfrom);
			} else {
				//Case 3: The resolved read is a normal read and the next
				//operation is a write, and we need to make sure that the
				//write is mod ordered after rf
				if (lastact!=rf)
					mo_graph->addEdge(rf, lastact);
			}
			break;
		}
	}
}

/**
 * Updates the mo_graph with the constraints imposed from the current write.
 *
 * Basic idea is the following: Go through each other thread and find
 * the lastest action that happened before our write.  Two cases:
 *
 * (1) The action is a write => that write must occur before
 * the current write
 *
 * (2) The action is a read => the write that that action read from
 * must occur before the current write.
 *
 * This method also handles two other issues:
 *
 * (I) Sequential Consistency: Making sure that if the current write is
 * seq_cst, that it occurs after the previous seq_cst write.
 *
 * (II) Sending the write back to non-synchronizing reads.
 *
 * @param curr The current action. Must be a write.
 * @return True if modification order edges were added; false otherwise
 */
bool ModelChecker::w_modification_order(ModelAction *curr)
{
	std::vector<action_list_t> *thrd_lists = obj_thrd_map->get_safe_ptr(curr->get_location());
	unsigned int i;
	bool added = false;
	ASSERT(curr->is_write());

	if (curr->is_seqcst()) {
		/* We have to at least see the last sequentially consistent write,
			 so we are initialized. */
		ModelAction *last_seq_cst = get_last_seq_cst(curr);
		if (last_seq_cst != NULL) {
			mo_graph->addEdge(last_seq_cst, curr);
			added = true;
		}
	}

	/* Iterate over all threads */
	for (i = 0; i < thrd_lists->size(); i++) {
		/* Iterate over actions in thread, starting from most recent */
		action_list_t *list = &(*thrd_lists)[i];
		action_list_t::reverse_iterator rit;
		for (rit = list->rbegin(); rit != list->rend(); rit++) {
			ModelAction *act = *rit;
			if (act == curr) {
				/*
				 * If RMW, we already have all relevant edges,
				 * so just skip to next thread.
				 * If normal write, we need to look at earlier
				 * actions, so continue processing list.
				 */
				if (curr->is_rmw())
					break;
				else
					continue;
			}

			/*
			 * Include at most one act per-thread that "happens
			 * before" curr
			 */
			if (act->happens_before(curr)) {
				/*
				 * Note: if act is RMW, just add edge:
				 *   act --mo--> curr
				 * The following edge should be handled elsewhere:
				 *   readfrom(act) --mo--> act
				 */
				if (act->is_write())
					mo_graph->addEdge(act, curr);
				else if (act->is_read() && act->get_reads_from() != NULL)
					mo_graph->addEdge(act->get_reads_from(), curr);
				added = true;
				break;
			} else if (act->is_read() && !act->is_synchronizing(curr) &&
			                             !act->same_thread(curr)) {
				/* We have an action that:
				   (1) did not happen before us
				   (2) is a read and we are a write
				   (3) cannot synchronize with us
				   (4) is in a different thread
				   =>
				   that read could potentially read from our write.
				 */
				if (thin_air_constraint_may_allow(curr, act)) {
					if (isfeasible() ||
							(curr->is_rmw() && act->is_rmw() && curr->get_reads_from() == act->get_reads_from() && isfeasibleotherthanRMW())) {
						struct PendingFutureValue pfv = {curr->get_value(),curr->get_seq_number()+params.maxfuturedelay,act};
						futurevalues->push_back(pfv);
					}
				}
			}
		}
	}

	return added;
}

/** Arbitrary reads from the future are not allowed.  Section 29.3
 * part 9 places some constraints.  This method checks one result of constraint
 * constraint.  Others require compiler support. */
bool ModelChecker::thin_air_constraint_may_allow(const ModelAction * writer, const ModelAction *reader) {
	if (!writer->is_rmw())
		return true;

	if (!reader->is_rmw())
		return true;

	for (const ModelAction *search = writer->get_reads_from(); search != NULL; search = search->get_reads_from()) {
		if (search == reader)
			return false;
		if (search->get_tid() == reader->get_tid() &&
				search->happens_before(reader))
			break;
	}

	return true;
}

/**
 * Finds the head(s) of the release sequence(s) containing a given ModelAction.
 * The ModelAction under consideration is expected to be taking part in
 * release/acquire synchronization as an object of the "reads from" relation.
 * Note that this can only provide release sequence support for RMW chains
 * which do not read from the future, as those actions cannot be traced until
 * their "promise" is fulfilled. Similarly, we may not even establish the
 * presence of a release sequence with certainty, as some modification order
 * constraints may be decided further in the future. Thus, this function
 * "returns" two pieces of data: a pass-by-reference vector of @a release_heads
 * and a boolean representing certainty.
 *
 * @todo Finish lazy updating, when promises are fulfilled in the future
 * @param rf The action that might be part of a release sequence. Must be a
 * write.
 * @param release_heads A pass-by-reference style return parameter.  After
 * execution of this function, release_heads will contain the heads of all the
 * relevant release sequences, if any exists
 * @return true, if the ModelChecker is certain that release_heads is complete;
 * false otherwise
 */
bool ModelChecker::release_seq_head(const ModelAction *rf, rel_heads_list_t *release_heads) const
{
	/* Only check for release sequences if there are no cycles */
	if (mo_graph->checkForCycles())
		return false;

	while (rf) {
		ASSERT(rf->is_write());

		if (rf->is_release())
			release_heads->push_back(rf);
		if (!rf->is_rmw())
			break; /* End of RMW chain */

		/** @todo Need to be smarter here...  In the linux lock
		 * example, this will run to the beginning of the program for
		 * every acquire. */
		/** @todo The way to be smarter here is to keep going until 1
		 * thread has a release preceded by an acquire and you've seen
		 *	 both. */

		/* acq_rel RMW is a sufficient stopping condition */
		if (rf->is_acquire() && rf->is_release())
			return true; /* complete */

		rf = rf->get_reads_from();
	};
	if (!rf) {
		/* read from future: need to settle this later */
		return false; /* incomplete */
	}

	if (rf->is_release())
		return true; /* complete */

	/* else relaxed write; check modification order for contiguous subsequence
	 * -> rf must be same thread as release */
	int tid = id_to_int(rf->get_tid());
	std::vector<action_list_t> *thrd_lists = obj_thrd_map->get_safe_ptr(rf->get_location());
	action_list_t *list = &(*thrd_lists)[tid];
	action_list_t::const_reverse_iterator rit;

	/* Find rf in the thread list */
	rit = std::find(list->rbegin(), list->rend(), rf);
	ASSERT(rit != list->rend());

	/* Find the last write/release */
	for (; rit != list->rend(); rit++)
		if ((*rit)->is_release())
			break;
	if (rit == list->rend()) {
		/* No write-release in this thread */
		return true; /* complete */
	}
	ModelAction *release = *rit;

	ASSERT(rf->same_thread(release));

	bool certain = true;
	for (unsigned int i = 0; i < thrd_lists->size(); i++) {
		if (id_to_int(rf->get_tid()) == (int)i)
			continue;
		list = &(*thrd_lists)[i];

		/* Can we ensure no future writes from this thread may break
		 * the release seq? */
		bool future_ordered = false;

		ModelAction *last = get_last_action(int_to_id(i));
		if (last && (rf->happens_before(last) ||
				last->get_type() == THREAD_FINISH))
			future_ordered = true;

		for (rit = list->rbegin(); rit != list->rend(); rit++) {
			const ModelAction *act = *rit;
			/* Reach synchronization -> this thread is complete */
			if (act->happens_before(release))
				break;
			if (rf->happens_before(act)) {
				future_ordered = true;
				continue;
			}

			/* Only writes can break release sequences */
			if (!act->is_write())
				continue;

			/* Check modification order */
			if (mo_graph->checkReachable(rf, act)) {
				/* rf --mo--> act */
				future_ordered = true;
				continue;
			}
			if (mo_graph->checkReachable(act, release))
				/* act --mo--> release */
				break;
			if (mo_graph->checkReachable(release, act) &&
			              mo_graph->checkReachable(act, rf)) {
				/* release --mo-> act --mo--> rf */
				return true; /* complete */
			}
			certain = false;
		}
		if (!future_ordered)
			return false; /* This thread is uncertain */
	}

	if (certain)
		release_heads->push_back(release);
	return certain;
}

/**
 * A public interface for getting the release sequence head(s) with which a
 * given ModelAction must synchronize. This function only returns a non-empty
 * result when it can locate a release sequence head with certainty. Otherwise,
 * it may mark the internal state of the ModelChecker so that it will handle
 * the release sequence at a later time, causing @a act to update its
 * synchronization at some later point in execution.
 * @param act The 'acquire' action that may read from a release sequence
 * @param release_heads A pass-by-reference return parameter. Will be filled
 * with the head(s) of the release sequence(s), if they exists with certainty.
 * @see ModelChecker::release_seq_head
 */
void ModelChecker::get_release_seq_heads(ModelAction *act, rel_heads_list_t *release_heads)
{
	const ModelAction *rf = act->get_reads_from();
	bool complete;
	complete = release_seq_head(rf, release_heads);
	if (!complete) {
		/* add act to 'lazy checking' list */
		pending_acq_rel_seq->push_back(act);
	}
}

/**
 * Attempt to resolve all stashed operations that might synchronize with a
 * release sequence for a given location. This implements the "lazy" portion of
 * determining whether or not a release sequence was contiguous, since not all
 * modification order information is present at the time an action occurs.
 *
 * @param location The location/object that should be checked for release
 * sequence resolutions. A NULL value means to check all locations.
 * @param work_queue The work queue to which to add work items as they are
 * generated
 * @return True if any updates occurred (new synchronization, new mo_graph
 * edges)
 */
bool ModelChecker::resolve_release_sequences(void *location, work_queue_t *work_queue)
{
	bool updated = false;
	std::vector<ModelAction *>::iterator it = pending_acq_rel_seq->begin();
	while (it != pending_acq_rel_seq->end()) {
		ModelAction *act = *it;

		/* Only resolve sequences on the given location, if provided */
		if (location && act->get_location() != location) {
			it++;
			continue;
		}

		const ModelAction *rf = act->get_reads_from();
		rel_heads_list_t release_heads;
		bool complete;
		complete = release_seq_head(rf, &release_heads);
		for (unsigned int i = 0; i < release_heads.size(); i++) {
			if (!act->has_synchronized_with(release_heads[i])) {
				if (act->synchronize_with(release_heads[i]))
					updated = true;
				else
					set_bad_synchronization();
			}
		}

		if (updated) {
			/* Re-check all pending release sequences */
			work_queue->push_back(CheckRelSeqWorkEntry(NULL));
			/* Re-check act for mo_graph edges */
			work_queue->push_back(MOEdgeWorkEntry(act));

			/* propagate synchronization to later actions */
			action_list_t::reverse_iterator rit = action_trace->rbegin();
			for (; (*rit) != act; rit++) {
				ModelAction *propagate = *rit;
				if (act->happens_before(propagate)) {
					propagate->synchronize_with(act);
					/* Re-check 'propagate' for mo_graph edges */
					work_queue->push_back(MOEdgeWorkEntry(propagate));
				}
			}
		}
		if (complete)
			it = pending_acq_rel_seq->erase(it);
		else
			it++;
	}

	// If we resolved promises or data races, see if we have realized a data race.
	if (checkDataRaces()) {
		set_assert();
	}

	return updated;
}

/**
 * Performs various bookkeeping operations for the current ModelAction. For
 * instance, adds action to the per-object, per-thread action vector and to the
 * action trace list of all thread actions.
 *
 * @param act is the ModelAction to add.
 */
void ModelChecker::add_action_to_lists(ModelAction *act)
{
	int tid = id_to_int(act->get_tid());
	action_trace->push_back(act);

	obj_map->get_safe_ptr(act->get_location())->push_back(act);

	std::vector<action_list_t> *vec = obj_thrd_map->get_safe_ptr(act->get_location());
	if (tid >= (int)vec->size())
		vec->resize(priv->next_thread_id);
	(*vec)[tid].push_back(act);

	if ((int)thrd_last_action->size() <= tid)
		thrd_last_action->resize(get_num_threads());
	(*thrd_last_action)[tid] = act;
}

/**
 * @brief Get the last action performed by a particular Thread
 * @param tid The thread ID of the Thread in question
 * @return The last action in the thread
 */
ModelAction * ModelChecker::get_last_action(thread_id_t tid) const
{
	int threadid = id_to_int(tid);
	if (threadid < (int)thrd_last_action->size())
		return (*thrd_last_action)[id_to_int(tid)];
	else
		return NULL;
}

/**
 * Gets the last memory_order_seq_cst write (in the total global sequence)
 * performed on a particular object (i.e., memory location), not including the
 * current action.
 * @param curr The current ModelAction; also denotes the object location to
 * check
 * @return The last seq_cst write
 */
ModelAction * ModelChecker::get_last_seq_cst(ModelAction *curr) const
{
	void *location = curr->get_location();
	action_list_t *list = obj_map->get_safe_ptr(location);
	/* Find: max({i in dom(S) | seq_cst(t_i) && isWrite(t_i) && samevar(t_i, t)}) */
	action_list_t::reverse_iterator rit;
	for (rit = list->rbegin(); rit != list->rend(); rit++)
		if ((*rit)->is_write() && (*rit)->is_seqcst() && (*rit) != curr)
			return *rit;
	return NULL;
}

/**
 * Gets the last unlock operation performed on a particular mutex (i.e., memory
 * location). This function identifies the mutex according to the current
 * action, which is presumed to perform on the same mutex.
 * @param curr The current ModelAction; also denotes the object location to
 * check
 * @return The last unlock operation
 */
ModelAction * ModelChecker::get_last_unlock(ModelAction *curr) const
{
	void *location = curr->get_location();
	action_list_t *list = obj_map->get_safe_ptr(location);
	/* Find: max({i in dom(S) | isUnlock(t_i) && samevar(t_i, t)}) */
	action_list_t::reverse_iterator rit;
	for (rit = list->rbegin(); rit != list->rend(); rit++)
		if ((*rit)->is_unlock())
			return *rit;
	return NULL;
}

ModelAction * ModelChecker::get_parent_action(thread_id_t tid)
{
	ModelAction *parent = get_last_action(tid);
	if (!parent)
		parent = get_thread(tid)->get_creation();
	return parent;
}

/**
 * Returns the clock vector for a given thread.
 * @param tid The thread whose clock vector we want
 * @return Desired clock vector
 */
ClockVector * ModelChecker::get_cv(thread_id_t tid)
{
	return get_parent_action(tid)->get_cv();
}

/**
 * Resolve a set of Promises with a current write. The set is provided in the
 * Node corresponding to @a write.
 * @param write The ModelAction that is fulfilling Promises
 * @return True if promises were resolved; false otherwise
 */
bool ModelChecker::resolve_promises(ModelAction *write)
{
	bool resolved = false;

	for (unsigned int i = 0, promise_index = 0; promise_index < promises->size(); i++) {
		Promise *promise = (*promises)[promise_index];
		if (write->get_node()->get_promise(i)) {
			ModelAction *read = promise->get_action();
			if (read->is_rmw()) {
				mo_graph->addRMWEdge(write, read);
			}
			read->read_from(write);
			//First fix up the modification order for actions that happened
			//before the read
			r_modification_order(read, write);
			//Next fix up the modification order for actions that happened
			//after the read.
			post_r_modification_order(read, write);
			//Make sure the promise's value matches the write's value
			ASSERT(promise->get_value() == write->get_value());

			delete(promise);
			promises->erase(promises->begin() + promise_index);
			resolved = true;
		} else
			promise_index++;
	}
	return resolved;
}

/**
 * Compute the set of promises that could potentially be satisfied by this
 * action. Note that the set computation actually appears in the Node, not in
 * ModelChecker.
 * @param curr The ModelAction that may satisfy promises
 */
void ModelChecker::compute_promises(ModelAction *curr)
{
	for (unsigned int i = 0; i < promises->size(); i++) {
		Promise *promise = (*promises)[i];
		const ModelAction *act = promise->get_action();
		if (!act->happens_before(curr) &&
				act->is_read() &&
				!act->is_synchronizing(curr) &&
				!act->same_thread(curr) &&
				promise->get_value() == curr->get_value()) {
			curr->get_node()->set_promise(i);
		}
	}
}

/** Checks promises in response to change in ClockVector Threads. */
void ModelChecker::check_promises(ClockVector *old_cv, ClockVector *merge_cv)
{
	for (unsigned int i = 0; i < promises->size(); i++) {
		Promise *promise = (*promises)[i];
		const ModelAction *act = promise->get_action();
		if ((old_cv == NULL || !old_cv->synchronized_since(act)) &&
				merge_cv->synchronized_since(act)) {
			//This thread is no longer able to send values back to satisfy the promise
			int num_synchronized_threads = promise->increment_threads();
			if (num_synchronized_threads == get_num_threads()) {
				//Promise has failed
				failed_promise = true;
				return;
			}
		}
	}
}

/**
 * Build up an initial set of all past writes that this 'read' action may read
 * from. This set is determined by the clock vector's "happens before"
 * relationship.
 * @param curr is the current ModelAction that we are exploring; it must be a
 * 'read' operation.
 */
void ModelChecker::build_reads_from_past(ModelAction *curr)
{
	std::vector<action_list_t> *thrd_lists = obj_thrd_map->get_safe_ptr(curr->get_location());
	unsigned int i;
	ASSERT(curr->is_read());

	ModelAction *last_seq_cst = NULL;

	/* Track whether this object has been initialized */
	bool initialized = false;

	if (curr->is_seqcst()) {
		last_seq_cst = get_last_seq_cst(curr);
		/* We have to at least see the last sequentially consistent write,
			 so we are initialized. */
		if (last_seq_cst != NULL)
			initialized = true;
	}

	/* Iterate over all threads */
	for (i = 0; i < thrd_lists->size(); i++) {
		/* Iterate over actions in thread, starting from most recent */
		action_list_t *list = &(*thrd_lists)[i];
		action_list_t::reverse_iterator rit;
		for (rit = list->rbegin(); rit != list->rend(); rit++) {
			ModelAction *act = *rit;

			/* Only consider 'write' actions */
			if (!act->is_write() || act == curr)
				continue;

			/* Don't consider more than one seq_cst write if we are a seq_cst read. */
			if (!curr->is_seqcst() || (!act->is_seqcst() && (last_seq_cst == NULL || !act->happens_before(last_seq_cst))) || act == last_seq_cst) {
				DEBUG("Adding action to may_read_from:\n");
				if (DBG_ENABLED()) {
					act->print();
					curr->print();
				}
				curr->get_node()->add_read_from(act);
			}

			/* Include at most one act per-thread that "happens before" curr */
			if (act->happens_before(curr)) {
				initialized = true;
				break;
			}
		}
	}

	if (!initialized) {
		/** @todo Need a more informative way of reporting errors. */
		printf("ERROR: may read from uninitialized atomic\n");
	}

	if (DBG_ENABLED() || !initialized) {
		printf("Reached read action:\n");
		curr->print();
		printf("Printing may_read_from\n");
		curr->get_node()->print_may_read_from();
		printf("End printing may_read_from\n");
	}

	ASSERT(initialized);
}

static void print_list(action_list_t *list)
{
	action_list_t::iterator it;

	printf("---------------------------------------------------------------------\n");
	printf("Trace:\n");

	for (it = list->begin(); it != list->end(); it++) {
		(*it)->print();
	}
	printf("---------------------------------------------------------------------\n");
}

#if SUPPORT_MOD_ORDER_DUMP
void ModelChecker::dumpGraph(char *filename) {
	char buffer[200];
  sprintf(buffer, "%s.dot",filename);
  FILE *file=fopen(buffer, "w");
  fprintf(file, "digraph %s {\n",filename);
	mo_graph->dumpNodes(file);
	ModelAction ** thread_array=(ModelAction **)model_calloc(1, sizeof(ModelAction *)*get_num_threads());
	
	for (action_list_t::iterator it = action_trace->begin(); it != action_trace->end(); it++) {
		ModelAction *action=*it;
		if (action->is_read()) {
			fprintf(file, "N%u [label=\"%u, T%u\"];\n", action->get_seq_number(),action->get_seq_number(), action->get_tid());
			fprintf(file, "N%u -> N%u[label=\"rf\", color=red];\n", action->get_seq_number(), action->get_reads_from()->get_seq_number());
		}
		if (thread_array[action->get_tid()] != NULL) {
			fprintf(file, "N%u -> N%u[label=\"sb\", color=blue];\n", thread_array[action->get_tid()]->get_seq_number(), action->get_seq_number());
		}
		
		thread_array[action->get_tid()]=action;
	}
  fprintf(file,"}\n");
	model_free(thread_array);
  fclose(file);	
}
#endif

void ModelChecker::print_summary()
{
	printf("\n");
	printf("Number of executions: %d\n", num_executions);
	printf("Number of feasible executions: %d\n", num_feasible_executions);
	printf("Total nodes created: %d\n", node_stack->get_total_nodes());

#if SUPPORT_MOD_ORDER_DUMP
	scheduler->print();
	char buffername[100];
	sprintf(buffername, "exec%04u", num_executions);
	mo_graph->dumpGraphToFile(buffername);
	sprintf(buffername, "graph%04u", num_executions);
  dumpGraph(buffername);
#endif

	if (!isfinalfeasible())
		printf("INFEASIBLE EXECUTION!\n");
	print_list(action_trace);
	printf("\n");
}

/**
 * Add a Thread to the system for the first time. Should only be called once
 * per thread.
 * @param t The Thread to add
 */
void ModelChecker::add_thread(Thread *t)
{
	thread_map->put(id_to_int(t->get_id()), t);
	scheduler->add_thread(t);
}

/**
 * Removes a thread from the scheduler. 
 * @param the thread to remove.
 */
void ModelChecker::remove_thread(Thread *t)
{
	scheduler->remove_thread(t);
}

/**
 * Switch from a user-context to the "master thread" context (a.k.a. system
 * context). This switch is made with the intention of exploring a particular
 * model-checking action (described by a ModelAction object). Must be called
 * from a user-thread context.
 * @param act The current action that will be explored. Must not be NULL.
 * @return Return status from the 'swap' call (i.e., success/fail, 0/-1)
 */
int ModelChecker::switch_to_master(ModelAction *act)
{
	DBG();
	Thread *old = thread_current();
	set_current_action(act);
	old->set_state(THREAD_READY);
	return Thread::swap(old, &system_context);
}

/**
 * Takes the next step in the execution, if possible.
 * @return Returns true (success) if a step was taken and false otherwise.
 */
bool ModelChecker::take_step() {
	if (has_asserted())
		return false;

	Thread * curr = thread_current();
	if (curr) {
		if (curr->get_state() == THREAD_READY) {
			ASSERT(priv->current_action);

			priv->nextThread = check_current_action(priv->current_action);
			priv->current_action = NULL;

			if (curr->is_blocked() || curr->is_complete())
				scheduler->remove_thread(curr);
		} else {
			ASSERT(false);
		}
	}
	Thread * next = scheduler->next_thread(priv->nextThread);

	/* Infeasible -> don't take any more steps */
	if (!isfeasible())
		return false;

	if (next)
		next->set_state(THREAD_RUNNING);
	DEBUG("(%d, %d)\n", curr ? curr->get_id() : -1, next ? next->get_id() : -1);

	/* next == NULL -> don't take any more steps */
	if (!next)
		return false;

	if ( next->get_pending() != NULL ) {
		//restart a pending action
		set_current_action(next->get_pending());
		next->set_pending(NULL);
		next->set_state(THREAD_READY);
		return true;
	}

	/* Return false only if swap fails with an error */
	return (Thread::swap(&system_context, next) == 0);
}

/** Runs the current execution until threre are no more steps to take. */
void ModelChecker::finish_execution() {
	DBG();

	while (take_step());
}
