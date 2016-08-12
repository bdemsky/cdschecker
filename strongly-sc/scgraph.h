#ifndef SCGRAPH_H
#define SCGRAPH_H
#include "wildcard.h"
#include "hashtable.h"
#include "execution.h"
#include "action.h"
#include "stl-model.h"
#include "scinference.h"

struct SCNode;
struct SCEdge;
struct SCPath;
class SCGraph;
typedef ModelVector<SCEdge *> EdgeList;
typedef SnapList<SCNode *> node_list_t;
typedef SnapList<SCEdge *> edge_list_t;
typedef SnapList<SCPath *> path_list_t;
typedef SnapList<action_list_t *> obj_list_list_t;

struct SCNode {
    const ModelAction *op;
    EdgeList *incoming;
    EdgeList *outgoing;

    // The clock vector for SB edges; we can simply use the operation's clock
    // vector for the "sb U rf" relation
    ClockVector *sbCV;

    // This clock vector is used to optimize calculating the ordering between
    // W->W && W->R. If W1 -mo-> W2, synchronize the clock vector;
    // If W1-sc|hb->R1, synchronize the clock vector.
    ClockVector *writeReadCV;

    // R->W && W->W. If W1 -mo-> W2, synchronize the clock vector;
    // If R1-hb->W1, synchronize the clock vector.
    ClockVector *readWriteCV;

    SCNode(ModelAction *op);

    // For SB reachability
    bool mergeSB(SCNode *dest);
    bool sbSynchronized(SCNode *dest);

    // For write-read ordering reachability
    bool mergeWriteRead(SCNode *dest);
    bool writeReadSynchronized(SCNode *dest);
    // For read-write ordering reachability
    bool mergeReadWrite(SCNode *dest);
    bool readWriteSynchronized(SCNode *dest);

    // For rfUsb reachability 
    bool sbRFSynchronized(SCNode *dest);

    bool earlier(SCNode *another);
    static bool earlier(SCNode *n1, SCNode *n2);
    static bool earlier(const ModelAction *act1, const ModelAction *act2);

    bool is_seqcst();
    bool is_release();
    bool is_acquire();

    void addOutgoingEdge(SCEdge *e);

    void addIncomingEdge(SCEdge *e);

    void printAction();
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
    // Copy constructor
    SCPath(SCPath &p);
    
    void addEdgeFromFront(SCEdge *e);
    void print(SCNode *tailNode, bool lineBreak = true);
    void print(bool lineBreak = true);
    void printWithOrder(SCNode *tailNode, SCInference *infer, bool lineBreak = true);

	SNAPSHOTALLOC
};

// Used for the function to find all the paths between two nodes
struct EdgeChoice {
    SCNode *n; // The node the current edge points to
    unsigned i; // The index of the incoming edge

    int finished; // Whether this choice is finished exploring

    EdgeChoice(SCNode *n, int i, int finished = 0);
	
    SNAPSHOTALLOC
};

class SCGraph {
public:
    SCGraph();
    SCGraph(ModelExecution *e, action_list_t *actions, SCInference *infer);
    ~SCGraph();

    // Print the graph 
	void print();
    
	SNAPSHOTALLOC
private:
    // The SC execution 
    ModelExecution *execution;
   
    // The original SC trace
    action_list_t *actions;

    // The per location list
    obj_list_list_t objLists;
    // The location map
    HashTable<const void *, const void *, uintptr_t, 4 > objSet;

    // A list of node representatives
    node_list_t nodes;
   
    // A map from an operation to a node
    HashTable<const ModelAction *, SCNode *, uintptr_t, 4 > nodeMap;
    
    HashTable<const ModelAction *, const ModelAction *, uintptr_t, 4 > locSet;

	void printInfoPerLoc();

    // Currently we just infer one set of parameters
    SCInference *inference;

    // Check whether the property holds
    bool checkStrongSC();

    bool imposeStrongPathWriteWrite(SCNode *w1, SCNode *w2);
    bool imposeStrongPathWriteRead(SCNode *w, SCNode *r);
    bool imposeStrongPathReadWrite(SCNode *r, SCNode *w);

    bool computeLocCV(action_list_t *objList);
    bool checkStrongSCPerLoc(action_list_t *objList);


    // Used for paths from read to write
    bool imposeSynchronizablePath(SCNode *from, SCNode *to, path_list_t *paths);
    bool imposeStrongPath(SCNode *from, SCNode *to, path_list_t *paths);
    bool imposeOneStrongPath(SCNode *from, SCNode *to, SCPath *p, bool
        justCheck = false);
    bool imposeSyncPath(edge_list_t::iterator begin, edge_list_t::iterator
        end, SCNode *from, SCNode *to, edge_list_t *edges, bool justCheck =
            false);
    
    SCEdge * removeRFEdge(SCNode *read);
    SCEdge * removeIncomingEdge(SCNode *from, SCNode *to, SCEdgeType type);

    // Find the paths from one node to another node
    path_list_t * findPaths(SCNode *from, SCNode *to, int depth = 1);
    path_list_t * findSynchronizablePathsIteratively(SCNode *from, SCNode *to);
    path_list_t * findPathsIteratively(SCNode *from, SCNode *to);

    // Given an existing list of subpaths from A to B1, and an edge from B1 to
    // B, and a result set of edges from A to B, this method attached the edge
    // (B1, B) to the subpaths, and then add them to the result set.
    void addPathsFromSubpaths(path_list_t *result, path_list_t *subpaths, SCEdge
    *e);

    // Sort the edges in order of SB, RF and (RW|WW)
    void sortEdges();

    // Build the graph from the trace
	void buildGraph();

	void addEdge(SCNode *from, SCNode *to, SCEdgeType type);

    // Compute the SB clock vectors
	void computeSBCV();
	void computeCV();
	int buildVectors();
	bool processRead(ModelAction *read, ClockVector *cv);
	bool merge(ClockVector *cv, const ModelAction *act, const ModelAction *act2);
	HashTable<const ModelAction *, ClockVector *, uintptr_t, 4 > cvmap;
	SnapVector<action_list_t> threadlists;
	int maxthreads;
};
#endif
