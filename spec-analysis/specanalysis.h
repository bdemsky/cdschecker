#ifndef _SPECANALYSIS_H
#define _SPECANALYSIS_H

#include <stack>

#include "traceanalysis.h"
#include "hashtable.h"
#include "specannotation.h"
#include "mymemory.h"
#include "modeltypes.h"
#include "action.h"
#include "common.h"

struct commit_point_node;
struct commit_point_edge;

// Record the a potential commit point information, including the potential
// commit point label number and the corresponding operation
typedef struct potential_cp_info {
	int label_num;
	ModelAction *operation;

	SNAPSHOTALLOC
} potential_cp_info;

typedef HashTable<ModelAction*, commit_point_node*, uintptr_t, 4> node_table_t;
//typedef SnapList<ModelAction*> action_list_t;

typedef SnapList<commit_point_edge*> edge_list_t;
typedef SnapList<potential_cp_info*> pcp_list_t;
typedef SnapList<anno_hb_init*> hbrule_list_t;
typedef SnapList<anno_hb_condition*> hbcond_list_t;
typedef SnapList<commit_point_node*> node_list_t;

typedef HashTable<call_id_t, node_list_t*, uintptr_t, 4> hb_table_t;


typedef enum cp_edge_type {
	HB, MO, RF, // Happens-before, Modification-order & Reads-from
	RBW // Read-before-write(for RF)
} edge_type;

typedef struct commit_point_edge {
	cp_edge_type type;
	commit_point_node *next_node;

	commit_point_edge(cp_edge_type t, commit_point_node *next) {
		type = t;
		next_node = next;
	}

	SNAPSHOTALLOC

} commit_point_edge;


/**
	Modify the commit_point_node to allow multipe commit points; basically it
	stores a list of ModelAction* and the list to reorder interface calls
*/
typedef struct commit_point_node {
	ModelAction *begin; // Interface begin annotation
	action_list_t *operations; // List of Commit point operation
	hbcond_list_t *hb_conds;
	call_id_t __ID__;
	int interface_num; // Interface number
	//int cp_label_num; // Commit point label number
	void *info;
	ModelAction *end; // Interface end annotation
	edge_list_t *edges;

	// For DFS
	int color; // 0 -> undiscovered; 1 -> discovered; 2 -> visited
	int finish_time_stamp;
	
	void addEdge(commit_point_node *next, cp_edge_type type) {
		if (edges == NULL) {
			edges = new edge_list_t();
		}
		for (edge_list_t::iterator iter = edges->begin(); iter != edges->end();
			iter++) {
			commit_point_edge *e = *iter;
			if (e->next_node == next && e->type == type)
				return;
		}

		//model_print("hey\n");
		commit_point_edge *new_edge = new commit_point_edge(type, next);
		edges->push_back(new_edge);
		/*
		ModelAction *op = next->operation;
		// Special routine for RF
		if (type == RF) {
			if (op->get_type() == ATOMIC_READ) {
				edges->push_back(new commit_point_edge(type, next));
				//model_print("Read %d: RF read\n", op->get_seq_number());
			} else {
				edges->push_front(new commit_point_edge(type, next));
				//model_print("NonRead %d: RF read\n", op->get_seq_number());
			}
		} else {
			//model_print("non-RF\n");
			//model_print("NonRF %d\n", op->get_seq_number());
			edges->push_front(new commit_point_edge(type, next));
		}
		
		for (edge_list_t::iterator iter = edges->begin(); iter != edges->end();
			iter++) {
			commit_point_edge *e = *iter;
			int index = distance(edges->begin(), iter);
			model_print("Edge%d: %d\n", index, e->next_node->operation->get_seq_number());
		}
		*/
	}

	SNAPSHOTALLOC

} commit_point_node;

static int trace_num_cnt = 0;

class SPECAnalysis : public TraceAnalysis {
 public:
	SPECAnalysis();
	~SPECAnalysis();
	virtual void setExecution(ModelExecution * execution);
	virtual void analyze(action_list_t *actions);
	virtual const char * name();
	virtual bool option(char *);
	virtual void finish();

	SNAPSHOTALLOC
 private:
	ModelExecution *execution;
	void **func_table;
	hbrule_list_t *hb_rules;
	bool isBrokenExecution;


	/**
	 * Hashtable that contains all the commit point nodes, the reason to use the
	 * hashtable is to make building edge information faster.
	*/
	node_table_t *cpGraph;
	/**
	 * A list of ModelAction's of the commit points used to make iterating the
	 * commit points faster
	*/
	action_list_t *cpActions;

	void buildCPGraph(action_list_t *actions);
	commit_point_node* getCPNode(action_list_t *actions, action_list_t::iterator
		*iter);
	bool hasAnEdge(const ModelAction *act1, const ModelAction *act2);
	ModelAction* getPrevAction(action_list_t *actions, action_list_t::iterator
		*iter, ModelAction *anno);
	void buildEdges();
	node_list_t* sortCPGraph();
	bool isCyclic();
	bool check(node_list_t *sorted_commit_points);
	void freeCPNodes();
	void test();
	void dumpGraph(node_list_t *sorted_commit_points);
	void dumpDotGraph();
	void dumpNode(commit_point_node *node);
	void traverseActions(action_list_t *actions);

	void deletePcpList(pcp_list_t *pcp_list);
};


#endif
