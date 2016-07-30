#ifndef SCGRAPH_H
#define SCGRAPH_H
#include "hashtable.h"
#include "execution.h"
#include "action.h"
#include "stl-model.h"

struct SCNode;
struct SCEdge;
struct SCPath;
struct SCGraph;
typedef ModelVector<SCEdge *> EdgeList;
typedef SnapList<SCNode *> node_list_t;
typedef SnapList<SCEdge *> edge_list_t;
typedef SnapList<SCPath *> path_list_t;

struct SCNode {
    const ModelAction *op;
    EdgeList *incoming;
    EdgeList *outgoing;

    SCNode(ModelAction *op);

    void addOutgoingEdge(SCEdge *e);

    void addIncomingEdge(SCEdge *e);

    void print();

	SNAPSHOTALLOC
};

enum SCEdgeType {SB, RF, RW, WW}; // SB includes the sb & synchronization edges
    // between thread create->start & finish->join
    // RW & WW are implied edges representing read-write & write-write edges

struct SCEdge {
    SCEdgeType type;
    SCNode *node;

    SCEdge(SCEdgeType type, SCNode *node);

	SNAPSHOTALLOC
};

/**
    This represents a path of the SC Graph. We use a list of edges to from
    the destination to the source. For example, a path "A -sb-> B -rf-> C" from A to C
    is represented as "(rf, B), (sb, A)".

    We also record the number of implied edges and reads-from edges in the path.
    For example, in the aformentiioned path, impliedCnt=0 && rfCnt=1.
*/
struct SCPath {
    unsigned impliedCnt; // The number of implied edges in this path
    unsigned rfCnt; // The number of reads-from edges in this path
    edge_list_t *edges;

    SCPath();
};

class SCGraph {
 public:
    SCGraph();
    SCGraph(ModelExecution *e, action_list_t *actions);
    ~SCGraph();

    // Print the graph 
	void print();
    
	SNAPSHOTALLOC
 private:
    // The SC execution 
    ModelExecution *execution;
   
    // The original SC trace
    action_list_t *actions;

    // A list of node representatives
    node_list_t nodes;
   
    // A map from an operation to a node
    HashTable<const ModelAction *, SCNode *, uintptr_t, 4 > nodeMap;

    // Find the paths from one node to another node
    path_list_t * findPaths(SCNode *from, SCNode *to);

    // Build the graph from the trace
	void buildGraph();

	void addEdge(SCNode *from, SCNode *to, SCEdgeType type);

	void computeCV();
	int buildVectors();
	bool processRead(ModelAction *read, ClockVector *cv);
	bool merge(ClockVector *cv, const ModelAction *act, const ModelAction *act2);
	HashTable<const ModelAction *, ClockVector *, uintptr_t, 4 > cvmap;
	SnapVector<action_list_t> threadlists;
	int maxthreads;
};
#endif
