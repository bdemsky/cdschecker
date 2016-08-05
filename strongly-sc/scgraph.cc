#include "scgraph.h"
#include "action.h"
#include "threads-model.h"
#include "clockvector.h"
#include "execution.h"
#include <sys/time.h>


/**********    Misc    **********/
static void printSCEdgeType(SCEdgeType type) {
    switch (type) {
        case SB:
            model_print("SB");
            break;
        case RF:
            model_print("RF");
            break;
        case RW:
            model_print("RW");
            break;
        case WW:
            model_print("WW");
            break;
        default:
            ASSERT(false);
            break;
    }
}


/**********    SCNode    **********/
SCNode::SCNode(ModelAction *op) :
    op(op),
    incoming(new EdgeList),
    outgoing(new EdgeList) {

}

void SCNode::addOutgoingEdge(SCEdge *e) {
    outgoing->push_back(e);
}

void SCNode::addIncomingEdge(SCEdge *e) {
    incoming->push_back(e);
}

void SCNode::print() {
    op->print();
    for (unsigned i = 0; i < outgoing->size(); i++) {
        SCEdge *e = (*outgoing)[i];
        model_print("\t-");
        printSCEdgeType(e->type);
        model_print("->  ");
        e->node->op->print();
    }
}


/**********    SCEdge    **********/
SCEdge::SCEdge(SCEdgeType type, SCNode *node) :
    type(type),
    node(node) {

}


/**********    SCPath    **********/
SCPath::SCPath() :
    impliedCnt(0),
    rfCnt(0),
    edges(new edge_list_t) {
    
}

SCPath::SCPath(SCPath &p) {
    edges = new edge_list_t(*p.edges);
    impliedCnt = p.impliedCnt;
    rfCnt = p.rfCnt;
}

void SCPath::addEdgeFromFront(SCEdge *e) {
    if (e->type == RF)
        rfCnt++;
    else if (e->type == WW || e->type == RW)
        impliedCnt++;
    edges->push_front(e);
}

/**********    SCGraph    **********/
SCGraph::SCGraph(ModelExecution *e, action_list_t *actions) :
    execution(e),
    actions(actions),
    objLists(),
    objSet(),
    nodes(),
    nodeMap(),
	cvmap(),
    threadlists(1) {
    buildGraph();
}

SCGraph::~SCGraph() {

}


// Check whether the property holds
bool SCGraph::checkStrongSC() {
	for (obj_list_list_t::iterator it = objLists.begin(); it != objLists.end(); it++) {
        action_list_t *objs = *it;
        if (!checkStrongSCPerLoc(objs))
            return false;
    }

    return true;
}


bool SCGraph::checkStrongSCPerLoc(action_list_t *objList) {
    for (action_list_t::iterator it1 = objList->begin(); it1 != objList->end();
    it1++) {
        action_list_t::iterator it2 = it1;
        it2++;
        ModelAction *act1 = *it1;
        ClockVector *cv1 = cvmap.get(act1);
        SCNode *n1 = nodeMap.get(act1);
        ASSERT (cv1);
        for (; it2 != objList->end(); it2++) {
            ModelAction *act2 = *it2;
            ClockVector *cv2 = cvmap.get(act2);
            SCNode *n2 = nodeMap.get(act2);
            ASSERT (cv2);
            
            // The objList is always ordered by the seq_number of actions
            ASSERT (act1->get_seq_number() < act2->get_seq_number());
            
            if (cv2->synchronized_since(act1)) {
                if (act1->is_write() && act2->is_write()) {
                    // act2 is a RMW && act1 -rf-> act2
                    if (act2->is_rmw() && act2->get_reads_from() == act1) {
                        // Don't need to do anything
                    } else {
                        // Take out the WW edge from n1->n2
                        SCEdge *incomingEdge = removeIncomingEdge(n1, n2, WW);
                        path_list_t * paths = findPaths(n1, n2);
                        if (!imposeStrongPath(n1, n2, paths))
                            return false;
                        // Add back the WW edge from n1->n2
                        n2->incoming->push_back(incomingEdge);
                    }
                } else if (act1->is_write() && act2->is_read()) {

                }
            } else {
                // act1 and act2 are not ordered
                // We currently impose the seq_cst memory order between stores
                if (act1->is_write() && act2->is_write()) {
                    if (act1->get_mo() != memory_order_seq_cst ||
                        act2->get_mo() != memory_order_seq_cst)
                        return false;
                }

            }
        }
    }

    return true;
}

bool SCGraph::imposeStrongPath(SCNode *from, SCNode *to, path_list_t *paths) {
    for (path_list_t::iterator it = paths->begin(); it != paths->end(); it++) {
        SCPath *p = *it;
        bool isStrongPath = true;
        // This is a natrual strong path (sb + thread create/start... => hb)
        if (p->impliedCnt == 0 && p->rfCnt == 0) {
            return true;
        } else if (p->impliedCnt == 0) { // A synchronizable path
            edge_list_t *edges = p->edges;
            if (imposeSyncPath(edges->begin(), edges->end(), from, to, edges))
                return true;
        } else { // An SC-required path
            if (to->op->get_mo() != memory_order_seq_cst) {
                isStrongPath = false;
                continue;
            }
            // Find the synchronizable subpaths 
            edge_list_t *edges = p->edges;
            // "head" and "tail" represents the head and tail of a
            // synchronizable subpath
            edge_list_t::iterator curIter = edges->begin(),
                beginIter = curIter, endIter = curIter;
            SCNode *fromNode = from, *toNode = from;
            for (; curIter != edges->end(); curIter++) {
                // toNode points to the end of the subpath
                // fromNode points to the head of the subpath
                // when they are equals, only one node is in the path
                SCEdge *e = *curIter;
                SCNode *dest = e->node;
                if (e->type == RW || e->type == WW) { // Found the end node
                    endIter = curIter;
                    toNode = 
                } else {
                    fromNode = dest;
                }
                if (dest == to) { // The last edge, end of search
                    if (imposeSyncPath(headIter, tailIter, fromNode, dest,
                        edges))
                        return true;
                    break;
                }

            }
        }
    }

    return false;
}

/**
    When we call this method, the begin iterator points to the first edge that
    goes to the "to" node.
*/
bool SCGraph::imposeSyncPath(edge_list_t::iterator begin, edge_list_t::iterator
    end, SCNode *from, SCNode *to, edge_list_t *edges) {
    // Try a simple release-sequence-type synchronization
    // Find a continuous rf chain
    bool rfStart = false;
    bool rfEnd = false;
    bool isRelSeq = true;
    const ModelAction *relHead, *relTail;
    SCNode *curNode = to;
    for (edge_list_t::iterator it = begin; it != end; it++) {
        SCEdge *e = *it;
        if (e->type == RF && !rfStart) {
            rfStart = true;
            relTail = curNode->op;
        }
        if (rfStart && e->type != RF) {
            rfEnd = true;
            relHead = curNode->op;
        }
        if (e->type == RF && rfEnd) {
            isRelSeq = false;
            break;
        }
        curNode = e->node;
    }

    if (isRelSeq) {
        return relHead->get_mo() == memory_order_release && relTail->get_mo() ==
            memory_order_acquire;
    } else { // Simply require every reads-from edge to be release/acquire
        curNode = to;
        for (edge_list_t::iterator it = begin; it != end; it++) {
            SCEdge *e = *it;
            if (e->type == RF) {
                const ModelAction *write = e->node->op;
                if (write->get_mo() != memory_order_release ||
                    curNode->op->get_mo() != memory_order_acquire)
                    return false;
            }
            curNode = e->node;
        }
    }

    return true;
}

SCEdge * SCGraph::removeIncomingEdge(SCNode *from, SCNode *to, SCEdgeType type) {
    EdgeList *incomingEdges = to->incoming;
    for (unsigned i = 0; i < incomingEdges->size(); i++) {
        SCEdge *existing  = (*incomingEdges)[i];
        if (type == existing->type &&
            from == existing->node) {
            incomingEdges->erase(incomingEdges->begin() + i);
            return existing;
        }
    }
    // The caller must make sure that there's such an edge
    ASSERT (false);
    return NULL;
}

path_list_t * SCGraph::findPaths(SCNode *from, SCNode *to) {
    EdgeList *edges = to->incoming;
    path_list_t *result = new path_list_t;
    for (unsigned i = 0; i < edges->size(); i++) {
        SCEdge *e = (*edges)[i];
        SCNode *n = e->node;
        path_list_t *subpaths = NULL;
        
        if (n == from) {
            // Node "n" is equals to node "from", end of search
            // subpaths contain only 1 edge (i.e., e)
            subpaths = new path_list_t;
            SCPath *p = new SCPath;
            p->addEdgeFromFront(e);
            subpaths->push_back(p);
        } else if (n->op->get_seq_number() > from->op->get_seq_number()) {
            // Node n's seq_num > from's seq_num, subpaths should be empty
            // Since it's an SC execution 
            subpaths = NULL;
        } else {
            // Find all the paths: "from" -> "n"
            //path_list_t *subpaths = findPaths(from, n);
        }

        // Then attach the edge "e" to the subResult
        if (subpaths != NULL && !subpaths->empty()) {
            addPathsFromSubpaths(result, subpaths, e);

            // This should be the right time to delete subpaths
            // But make sure to check subpaths is not null
            if (subpaths) {
                delete subpaths;
            }
        }
    }

    return result;
}


void SCGraph::addPathsFromSubpaths(path_list_t *result, path_list_t *subpaths,
    SCEdge *e) {
    for (path_list_t::iterator it = subpaths->begin(); it != subpaths->end();
    it++) {
        SCPath *subpath = *it;
        SCPath *newPath = new SCPath(*subpath);
        newPath->addEdgeFromFront(e);
        result->push_back(newPath);
    }
}

void SCGraph::buildGraph() {
    buildVectors();
    computeCV();
    checkStrongSC();
}


void SCGraph::print() {
    for (node_list_t::iterator it = nodes.begin(); it != nodes.end(); it++) {
        SCNode *n = *it;
        n->print();
    }
}

void SCGraph::addEdge(SCNode *from, SCNode *to, SCEdgeType type) {
    SCEdge *e = new SCEdge(type, to);
    EdgeList *outgoingEdges = from->outgoing;
    for (unsigned i = 0; i < outgoingEdges->size(); i++) {
        SCEdge *existing  = (*outgoingEdges)[i];
        if (e->type == existing->type &&
            e->node == existing->node) {
            delete e;
            return;
        }
    }
    outgoingEdges->push_back(e);
    to->incoming->push_back(new SCEdge(type, from));
}

int SCGraph::buildVectors() {
	maxthreads = 0;
	int numactions = 0;
	for (action_list_t::iterator it = actions->begin(); it != actions->end(); it++) {
		ModelAction *act = *it;
        // Check if it's a known location
        void *loc = act->get_location();
        if (!objSet.contains(loc)) {
            action_list_t *objList = execution->get_obj_list(loc);
            objLists.push_back(objList);
            objSet.put(loc, loc);
        }

        // Initialize the node, add it to the list & map
        SCNode *n = new SCNode(act);
        nodes.push_back(n);
        nodeMap.put(act, n);

		numactions++;
		int threadid = id_to_int(act->get_tid());
		if (threadid > maxthreads) {
			threadlists.resize(threadid + 1);
			maxthreads = threadid;
		}
        // Process the sb & thread edges (start & join) & rf edges
        ModelAction *from = NULL;
        SCNode *fromNode = NULL;
        if (act->is_thread_start()) 
            from = execution->get_thread(act)->get_creation();
        if (act->is_thread_join()) {
            Thread *joinedthr = act->get_thread_operand();
            from = execution->get_last_action(joinedthr->get_id());
        }
        if (from) { // Add the edge "from->act"
            fromNode = nodeMap.get(from);
            ASSERT (fromNode);
            addEdge(fromNode, n, SB);
        }

        // Add the sb edge
        action_list_t *threadlist = &threadlists[threadid];
        if (threadlist->size() > 0) {
            from = threadlist->back();
            fromNode = nodeMap.get(from);
            addEdge(fromNode, n, SB);
        }

        // Add the rf edge
        if (act->is_read()) {
            const ModelAction *write = act->get_reads_from();
            fromNode = nodeMap.get(write);
            addEdge(fromNode, n, RF);
        }

		threadlists[threadid].push_back(act);
	}
	return numactions;
}

void SCGraph::computeCV() {
	bool changed = true;
	bool firsttime = true;
	ModelAction **last_act = (ModelAction **)model_calloc(1, (maxthreads + 1) * sizeof(ModelAction *));
	while (changed) {
		changed = changed&firsttime;
		firsttime = false;

		for (action_list_t::iterator it = actions->begin(); it != actions->end(); it++) {
			ModelAction *act = *it;
			ModelAction *lastact = last_act[id_to_int(act->get_tid())];
			if (act->is_thread_start())
				lastact = execution->get_thread(act)->get_creation();
			last_act[id_to_int(act->get_tid())] = act;
			ClockVector *cv = cvmap.get(act);
			if (cv == NULL) {
				cv = new ClockVector(NULL, act);
				cvmap.put(act, cv);
			}
			if (lastact != NULL) {
				merge(cv, act, lastact);
			}
			if (act->is_thread_join()) {
				Thread *joinedthr = act->get_thread_operand();
				ModelAction *finish = execution->get_last_action(joinedthr->get_id());
				changed |= merge(cv, act, finish);
			}
			if (act->is_read()) {
				changed |= processRead(act, cv);
			}
		}
		/* Reset the last action array */
		if (changed) {
			bzero(last_act, (maxthreads + 1) * sizeof(ModelAction *));
		}
	}
	model_free(last_act);
}


bool SCGraph::processRead(ModelAction *read, ClockVector *cv) {
	bool changed = false;

	/* Merge in the clock vector from the write */
	const ModelAction *write = read->get_reads_from();
	ClockVector *writecv = cvmap.get(write);
	changed |= merge(cv, read, write) && (*read < *write);

	for (int i = 0; i <= maxthreads; i++) {
		thread_id_t tid = int_to_id(i);
		if (tid == read->get_tid())
			continue;
		if (tid == write->get_tid())
			continue;
		action_list_t *list = execution->get_actions_on_obj(read->get_location(), tid);
		if (list == NULL)
			continue;
		for (action_list_t::reverse_iterator rit = list->rbegin(); rit != list->rend(); rit++) {
			ModelAction *write2 = *rit;
			if (!write2->is_write())
				continue;

			ClockVector *write2cv = cvmap.get(write2);
			if (write2cv == NULL)
				continue;

			/* write -sc-> write2 &&
				 write -rf-> R =>
				 R -sc-> write2 */
			if (write2cv->synchronized_since(write)) {
				changed |= merge(write2cv, write2, read);
                SCNode *readNode = nodeMap.get(read);
                SCNode *write2Node = nodeMap.get(write2);
                addEdge(readNode, write2Node, RW);
			}

			//looking for earliest write2 in iteration to satisfy this
			/* write2 -sc-> R &&
				 write -rf-> R =>
				 write2 -sc-> write */
			if (cv->synchronized_since(write2)) {
				changed |= writecv == NULL || merge(writecv, write, write2);
                SCNode *write2Node = nodeMap.get(write2);
                SCNode *writeNode = nodeMap.get(write);
                addEdge(write2Node, writeNode, WW);
				break;
			}
		}
	}
	return changed;
}

bool SCGraph::merge(ClockVector *cv, const ModelAction *act, const ModelAction *act2) {
	ClockVector *cv2 = cvmap.get(act2);
	if (cv2 == NULL)
		return true;
	if (cv2->getClock(act->get_tid()) >= act->get_seq_number() && act->get_seq_number() != 0) {
		// There cannot be cycles for SC executions 
		ASSERT(false);
	}

	return cv->merge(cv2);
}


