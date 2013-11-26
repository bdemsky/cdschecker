#ifndef SPECANALYSIS_H
#define SPECANALYSIS_H

#include <stack>

#include "traceanalysis.h"
#include "hashtable.h"
#include "specannotation.h"
#include "mymemory.h"

struct commit_point_node;
struct commit_point_edge;


typedef struct potential_cp_info {
	int label_num;
	ModelAction *operation;

	SNAPSHOTALLOC
} potential_cp_info;

typedef HashTable<const ModelAction*, commit_point_node*, uintptr_t, 4> node_table_t;
typedef SnapList<ModelAction*> action_list_t;

typedef SnapList<commit_point_edge*> edge_list_t;
typedef SnapList<potential_cp_info*> pcp_list_t;
typedef SnapList<anno_hb_init*> hbrule_list_t;
typedef SnapList<anno_hb_condition*> hbcond_list_t;
typedef SnapList<commit_point_node*> node_list_t;

typedef HashTable<call_id_t, node_list_t*, uintptr_t, 4> hb_table_t;


typedef enum cp_edge_type {
	HB, MO, RF // Happens-before, Modification-order & Reads-from
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

typedef struct commit_point_node {
	const ModelAction *begin;
	ModelAction *operation;
	hbcond_list_t *hb_conds;
	call_id_t __ID__;
	int interface_num;
	void *info;
	const ModelAction *end;
	edge_list_t *edges;

	// For DFS
	int color;
	int finish_time_stamp;
	
	void addEdge(commit_point_node *next, cp_edge_type type) {
		if (edges == NULL) {
			edges = new edge_list_t();
		}
		edges->push_back(new commit_point_edge(type, next));
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
	anno_func_table_init *func_table;
	hbrule_list_t *hb_rules;

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
	ModelAction* getPrevAction(action_list_t *actions, action_list_t::iterator
		*iter, const ModelAction *anno);
	void buildEdges();
	node_list_t* sortCPGraph();
	bool check(node_list_t *sorted_commit_points);
	void freeCPNodes();
	void test();
	void dumpGraph();
	void traverseActions(action_list_t *actions);
};


#endif
