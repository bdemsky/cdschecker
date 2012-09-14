/** @file cyclegraph.h @brief Data structure to track ordering
 *  constraints on modification order.  The idea is to see whether a
 *  total order exists that satisfies the ordering constriants.*/

#ifndef CYCLEGRAPH_H
#define CYCLEGRAPH_H

#include "hashtable.h"
#include <vector>
#include <inttypes.h>
#include "config.h"
#include "mymemory.h"

class CycleNode;
class ModelAction;

/** @brief A graph of Model Actions for tracking cycles. */
class CycleGraph {
 public:
	CycleGraph();
	~CycleGraph();
	void addEdge(const ModelAction *from, const ModelAction *to);
	bool checkForCycles();
	bool checkForRMWViolation();
	void addRMWEdge(const ModelAction *from, const ModelAction *rmw);

	bool checkReachable(const ModelAction *from, const ModelAction *to);
	void startChanges();
	void commitChanges();
	void rollbackChanges();
#if SUPPORT_MOD_ORDER_DUMP
	void dumpGraphToFile(const char * filename);
#endif

	SNAPSHOTALLOC
 private:
	CycleNode * getNode(const ModelAction *);

	/** @brief A table for mapping ModelActions to CycleNodes */
	HashTable<const ModelAction *, CycleNode *, uintptr_t, 4> actionToNode;
#if SUPPORT_MOD_ORDER_DUMP
	std::vector<CycleNode *> nodeList;
#endif

	bool checkReachable(CycleNode *from, CycleNode *to);

	/** @brief A flag: true if this graph contains cycles */
	bool hasCycles;
	bool oldCycles;

	bool hasRMWViolation;
	bool oldRMWViolation;

	std::vector<CycleNode *> rollbackvector;
	std::vector<CycleNode *> rmwrollbackvector;
};

/** @brief A node within a CycleGraph; corresponds to one ModelAction */
class CycleNode {
 public:
	CycleNode(const ModelAction *action);
	bool addEdge(CycleNode * node);
	std::vector<CycleNode *> * getEdges();
	bool setRMW(CycleNode *);
	CycleNode* getRMW();
	const ModelAction * getAction() {return action;};

	void popEdge() {
		edges.pop_back();
	};
	void clearRMW() {
		hasRMW=NULL;
	}

	SNAPSHOTALLOC
 private:
	/** @brief The ModelAction that this node represents */
	const ModelAction *action;

	/** @brief The edges leading out from this node */
	std::vector<CycleNode *> edges;

	/** Pointer to a RMW node that reads from this node, or NULL, if none
	 * exists */
	CycleNode * hasRMW;
};

#endif
