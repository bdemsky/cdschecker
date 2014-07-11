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

/* Forward declaration */
class SCFence;

extern SCFence *scfence;

typedef struct {
	memory_order wildcard;
	memory_order order;
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

typedef struct sc_node {
	const ModelAction *act;
	const_actions_t *edges;

	// For traversal
	int color;

	sc_node(const ModelAction *act) {
		this->act = act;
		edges = new const_actions_t(); 
	}

	void clear() {
		edges->clear();
	}

	void addEdge(const ModelAction *act_next) {
		for (const_actions_t::iterator it = edges->begin(); it != edges->end(); it++) {
			const ModelAction *act = *it;
			if (act == act_next)
				return;
		}
		edges->push_back(act_next);
	}

	SNAPSHOTALLOC
} sc_node;


typedef SnapList<sc_node *> sc_nodes_t;

typedef struct sc_graph {
	HashTable<const ModelAction *, sc_node *, uintptr_t, 4> node_map;
	const_actions_t actions;

	sc_graph(action_list_t *actions) :
		node_map(),
		actions()
	{
		for (action_list_t::iterator it = actions->begin(); it != actions->end(); it++) {
			this->actions.push_back(*it);
			this->node_map.put(*it, new sc_node(*it));
		}
	}

	void clear() {
		// Clear up nodes, actions & the node_map
		for (const_actions_t::iterator it = this->actions.begin(); it !=
			this->actions.end(); it++) {
			const ModelAction *act = *it;
			sc_node *n = this->node_map.get(act);
			n->clear();
		}
		this->actions.clear();
		this->node_map.reset();
	}

	void reset(action_list_t *actions) {
		clear();		
		// And then we build the new graph
		// We keep a action->node mapping
		for (action_list_t::iterator it = actions->begin(); it != actions->end(); it++) {
			this->actions.push_back(*it);
			this->node_map.put(*it, new sc_node(*it));
		}
	}

	void addEdge(const ModelAction *act1, const ModelAction *act2) {
		sc_node *n1 = node_map.get(act1);
		n1->addEdge(act2);
	}

	/** @Brief Printing out the graph in a plain way */
	void printGraph() {
		for (const_actions_t::iterator it = actions.begin(); it !=
			actions.end(); it++) {
			sc_node *n = node_map.get(*it);
			const ModelAction *act = n->act;
			if (act->get_seq_number() == 0)
				continue;
			model_print("Node: %d:\n", act->get_seq_number());

			for (const_actions_t::iterator edge_it = n->edges->begin(); edge_it
				!= n->edges->end(); edge_it++) {
				const ModelAction *next = *edge_it;
				model_print("%d --> %d\n", n->act->get_seq_number(),
					next->get_seq_number());
			}
		}
	}

	void printCyclicChain(const ModelAction *act1, const ModelAction *act2) {
		model_print("From -> to: %d -> %d:\n", act1->get_seq_number(), act2->get_seq_number());
		const_actions_t *cyclic_actions = getCycleActions(act2, act1);
		if (cyclic_actions == NULL) {
			model_print("Cannot find the cycle of actions!\n");
			return;
		}
		/*
		// Build the vector
		SnapVector<action_list_t> threadlist;
		int thread_num = 0;
		int action_num = buildVectors(&threadlist, &thread_num, cyclic_actions);
		model_print("Number of threads: %d\n", thread_num);
		*/
		for (const_actions_t::iterator it = cyclic_actions->begin(); it !=
			cyclic_actions->end();
			it++) {
			const ModelAction *act = *it;
			if (is_wildcard(act->get_original_mo())) {
				model_print("wildcard: %d\n", get_wildcard_id(act->get_original_mo()));
			}
			act->print();
		}
		delete cyclic_actions;
	}

	/** @Brief Only call this function when you are sure there's a cycle involving act1
	 * and act2; It traverse the graph from act1 with DFS to find a path to
	 * act2.
	 */
	const_actions_t* getCycleActions(const ModelAction *act1, const ModelAction *act2) {
		sc_nodes_t *stack = new sc_nodes_t();
		const_actions_t *actions = new const_actions_t();
		sc_node *n1 = node_map.get(act1),
			*n2 = node_map.get(act2), *n;
		stack->push_back(n1);
		while (stack->size() != 0) {
			n = stack->back();
			stack->pop_back();
			if (n->color == 0) {
				n->color = 1;
				stack->push_back(n);
				// Push back the 'next' nodes
				for (const_actions_t::iterator edge_it = n->edges->begin();
					edge_it != n->edges->end(); edge_it++) {
					sc_node *tmp_node = node_map.get(*edge_it);
					if (tmp_node == n2) { // Found it
						for (sc_nodes_t::iterator it = stack->begin(); it !=
							stack->end(); it++) {
							actions->push_back((*it)->act);
						}
						actions->push_back(n2->act);
						return actions;
					}
					if (tmp_node->color == 0)
						stack->push_back(tmp_node);
				}
			} else if (n->color == 1) { // Just "visit" it, do nothing
				n->color = 2;
			}
		}
		return NULL;
	}

	SNAPSHOTALLOC
} sc_graph;

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

	/** Functions that work for infering the parameters */
	sync_paths_t *get_rf_sb_paths(const ModelAction *act1, const ModelAction *act2);
	void printPatternFixes(action_list_t *list);
	void print_rf_sb_path(action_list_t *path);

	void breakCycle(const ModelAction *act1, const ModelAction *act2);
	const char* get_mo_str(memory_order order);
	void printWildcardResult(inference_list_t *result);

	int maxthreads;
	HashTable<const ModelAction *, ClockVector *, uintptr_t, 4 > cvmap;
	bool cyclic;
	HashTable<const ModelAction *, const ModelAction *, uintptr_t, 4 > badrfset;
	HashTable<void *, const ModelAction *, uintptr_t, 4 > lastwrmap;
	SnapVector<action_list_t> threadlists;
	SnapVector<action_list_t> dupthreadlists;
	ModelExecution *execution;
	bool print_always;
	bool print_buggy;
	bool print_nonsc;
	bool time;
	struct sc_statistics *stats;
	
	/** A map to remember the graph built so far */
	sc_graph *graph;
	/** The two modelAction from which we encounter a cycle */
	const ModelAction *cycle_act1, *cycle_act2;
	/** Current wildcard mapping: a wildcard -> the specifc ordering */
	wildcard_table_t *curWildcardMap;
	/** Current wildcards */
	inference_list_t *curInference;

	/** A list of possible results */
	SnapList<inference_list_t *> potentialResults;

};
#endif
