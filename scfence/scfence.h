#ifndef _SCFENCE_H
#define _SCFENCE_H
#include "traceanalysis.h"
#include "scanalysis.h"
#include "hashtable.h"
#include "memoryorder.h"

#ifdef __cplusplus
using std::memory_order;
#endif

/* Forward declaration */
class SCFence;

extern SCFence *wildcard_plugin;

typedef SnapList<const ModelAction *> const_actions_t;

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
	action_list_t *actions;

	sc_graph() : node_map() {}

	void init(action_list_t *actions) {
		this->actions = actions;
		for (action_list_t::iterator it = actions->begin(); it != actions->end(); it++) {
			node_map.put(*it, new sc_node(*it));
		}
	}

	void clear() {
		for (action_list_t::iterator it = actions->begin(); it != actions->end(); it++) {
			const ModelAction *act = *it;
			sc_node *n = node_map.get(act);
			n->clear();
		}
		node_map.reset();
	}

	void addEdge(const ModelAction *act1, const ModelAction *act2) {
		sc_node *n1 = node_map.get(act1);
		n1->addEdge(act2);
	}

	void printGraph() {
		for (action_list_t::iterator it = actions->begin(); it !=
			actions->end(); it++) {
			sc_node *n = node_map.get(*it);
			const ModelAction *act = n->act;
			model_print("Node: %d:\n", act->get_seq_number());

			for (const_actions_t::iterator edge_it = n->edges->begin(); edge_it
				!= n->edges->end(); edge_it++) {
				const ModelAction *next = *edge_it;
				//model_print("%d --> %d\n", n->act->get_seq_number(),
					//next->get_seq_number());
			}
		}
	}

	/** Only call this function when you are sure there's a cycle involving act1
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
						actions->push_back(act2);
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

	SNAPSHOTALLOC
 private:
	void update_stats();
	void print_list(action_list_t *list);
	int buildVectors(action_list_t *);
	bool updateConstraints(ModelAction *act);
	void computeCV(action_list_t *);
	action_list_t * generateSC(action_list_t *);
	bool processRead(ModelAction *read, ClockVector *cv);
	int getNextActions(ModelAction **array);
	bool merge(ClockVector *cv, const ModelAction *act, const ModelAction *act2);
	void printCyclicChain(const ModelAction *act1, const ModelAction *act2);
	void check_rf(action_list_t *list);
	void reset(action_list_t *list);
	ModelAction* pruneArray(ModelAction**, int);

	int maxthreads;
	HashTable<const ModelAction *, ClockVector *, uintptr_t, 4 > cvmap;
	bool cyclic;
	HashTable<const ModelAction *, const ModelAction *, uintptr_t, 4 > badrfset;
	HashTable<void *, const ModelAction *, uintptr_t, 4 > lastwrmap;
	SnapVector<action_list_t> threadlists;
	ModelExecution *execution;
	bool print_always;
	bool print_buggy;
	bool print_nonsc;
	bool time;
	struct sc_statistics *stats;
	
	/** Mapping: a wildcard action -> the specifc wildcard */
	HashTable<ModelAction *, memory_order, uintptr_t, 4> actOrderMap;
	/** Mapping: a wildcard -> the specifc ordering */
	HashTable<memory_order, memory_order, memory_order, 4> wildcardMap;

	/** A map to remember the graph built so far*/
	sc_graph graph;

};
#endif
