#include "scgraph.h"
#include "action.h"
#include "threads-model.h"
#include "clockvector.h"
#include "execution.h"
#include "mydebug.h"
#include <sys/time.h>


/**********    Misc    **********/
static const char * get_edge_str(SCEdgeType type) {
    switch (type) {
        case SB: return "SB";
        case RF: return "RF";
        case RW: return "RW";
        case WW: return "WW";
        default: return "UnkonwEdge";
    }
}

static const char * get_mo_str(memory_order order) {
    switch (order) {
        case std::memory_order_relaxed: return "relaxed";
        case std::memory_order_acquire: return "acquire";
        case std::memory_order_release: return "release";
        case std::memory_order_acq_rel: return "acq_rel";
        case std::memory_order_seq_cst: return "seq_cst";
        default: return "unknown";
    }
}


/**********    SCNode    **********/
SCNode::SCNode(ModelAction *op) :
    op(op),
    incoming(new EdgeList),
    outgoing(new EdgeList),
    sbCV (new ClockVector(NULL, op)) {
}


bool SCNode::mergeSB(SCNode *dest) {
    return dest->sbCV->merge(sbCV);
}

bool SCNode::sbSynchronized(SCNode *dest) {
    return dest->sbCV->synchronized_since(op);
}

bool SCNode::sbRFSynchronized(SCNode *dest) {
    return op->happens_before(dest->op);
}

bool SCNode::earlier(SCNode *another) {
    return earlier(this, another);
}

bool SCNode::earlier(SCNode *n1, SCNode *n2) {
    return earlier(n1->op, n2->op);
}

bool SCNode::earlier(const ModelAction *act1, const ModelAction *act2) {
    return act1->get_seq_number() < act2->get_seq_number();
}

bool SCNode::is_seqcst() {
    return op->is_seqcst();
}

bool SCNode::is_release() {
    return op->is_release();
}

bool SCNode::is_acquire() {
    return op->is_acquire();
}

void SCNode::addOutgoingEdge(SCEdge *e) {
    // If there's an edge (A, B), then A's seqnum must be less than B's seqnum
    ASSERT (this->earlier(e->node));
    outgoing->push_back(e);
}

void SCNode::addIncomingEdge(SCEdge *e) {
    // If there's an edge (A, B), then A's seqnum must be less than B's seqnum
    ASSERT (e->node->earlier(this));
    incoming->push_back(e);
}

void SCNode::printAction() {
    model_print("%-4d T%-2d   %-13s  %7s  %14p\n",
        op->get_seq_number(), id_to_int(op->get_tid()), op->get_type_str(),
        op->get_mo_str(), op->get_location());
}

void SCNode::print() {
    if (op->is_uninitialized()) {
        // Currently ignore the fake actions, i.e. uninitialized stores
        return;
    }
    printAction();
    for (unsigned i = 0; i < outgoing->size(); i++) {
        SCEdge *e = (*outgoing)[i];
        model_print("  -%s->  ", get_edge_str(e->type));
        e->node->printAction();
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
    DB(
        if (!edges->empty()) {
            SCEdge *front = edges->front();
            SCNode *frontNode = front->node;
            ASSERT (frontNode->earlier(e->node));
        }
    )

    if (e->type == RF)
        rfCnt++;
    else if (e->type == WW || e->type == RW)
        impliedCnt++;
    edges->push_front(e);
}

void SCPath::print(bool lineBreak) {
    edge_list_t::reverse_iterator it = edges->rbegin();
    SCEdge *e = *it;
    SCNode *n = e->node;
    for (; it != edges->rend(); it++) {
        e = *it;
        n = e->node;
        model_print("%d -%s-> ", n->op->get_seq_number(),
            get_edge_str(e->type));
    }
    if (lineBreak)
        model_print("\n");
}

void SCPath::printWithOrder(SCNode *tailNode, SCInference *infer, bool lineBreak) {
    edge_list_t::reverse_iterator it = edges->rbegin();
    SCEdge *e = *it;
    SCNode *n = e->node;
    // The wildcard ID 
    int wildcard = 0;
    // The current wildcard memory order
    memory_order curMO;
    for (; it != edges->rend(); it++) {
        e = *it;
        n = e->node;
        if (is_wildcard(n->op->get_original_mo())) {
            wildcard = get_wildcard_id(n->op->get_original_mo());
            curMO = infer->getWildcardMO(wildcard);
            model_print("%d(w%d)(%s) -%s-> ", n->op->get_seq_number(), wildcard,
                get_mo_str(curMO), get_edge_str(e->type));
        } else {
            model_print("%d(\"%s\") -%s-> ", n->op->get_seq_number(),
                n->op->get_mo_str(), get_edge_str(e->type));
        }
    }
    
    if (is_wildcard(tailNode->op->get_original_mo())) {
        wildcard = get_wildcard_id(tailNode->op->get_original_mo());
        curMO = infer->getWildcardMO(wildcard);
        model_print("%d(w%d)(%s)", tailNode->op->get_seq_number(), wildcard,
            get_mo_str(curMO));
    } else {
        model_print("%d(\"%s\")", tailNode->op->get_seq_number(),
            tailNode->op->get_mo_str());
    }

    if (lineBreak)
        model_print("\n");
}

void SCPath::print(SCNode *tailNode, bool lineBreak) {
    edge_list_t::reverse_iterator it = edges->rbegin();
    SCEdge *e = *it;
    SCNode *n = e->node;
    for (; it != edges->rend(); it++) {
        e = *it;
        n = e->node;
        model_print("%d -%s-> ", n->op->get_seq_number(),
            get_edge_str(e->type));
    }
    model_print("%d", tailNode->op->get_seq_number());
    if (lineBreak)
        model_print("\n");
}


/**********    EdgeChoice    **********/
EdgeChoice::EdgeChoice(SCNode *n, int i, int finished) :
    n(n),
    i(i),
    finished(finished) {
}

/**********    SCGraph    **********/
SCGraph::SCGraph(ModelExecution *e, action_list_t *actions, SCInference *infer) :
    execution(e),
    actions(actions),
    objLists(),
    objSet(),
    nodes(),
    nodeMap(),
    inference(infer),
	cvmap(),
    threadlists(1) {
    buildGraph();
}

SCGraph::~SCGraph() {

}

void SCGraph::printInfoPerLoc() {
    model_print("**********    Location List    **********\n");
    for (obj_list_list_t::iterator it = objLists.begin(); it != objLists.end(); it++) {
        action_list_t *objs = *it;
        action_list_t::iterator objIt = objs->begin();
        ModelAction *act = *objIt;
        if (act->is_read() || act->is_write())
            model_print("Location %14p:\n", act->get_location());
        for (; objIt != objs->end(); objIt++) {
            act = *objIt;
            if ((act->is_read() || act->is_write()) &&
                !act->is_uninitialized()) {
                model_print("  ");
                act->print();
            }
        }
    }
}

// Check whether the property holds
bool SCGraph::checkStrongSC() {
    for (obj_list_list_t::iterator it = objLists.begin(); it != objLists.end(); it++) {
        action_list_t *objs = *it;
        if (!checkStrongSCPerLoc(objs)) {
            DPRINT("The execution is NOT strongly SC'able\n");
            return false;
        }
    }
    
    return true;
}


bool SCGraph::checkStrongSCPerLoc(action_list_t *objList) {
    for (action_list_t::iterator it1 = objList->begin(); it1 != objList->end();
    it1++) {
        action_list_t::iterator it2 = it1;
        it2++;
        ModelAction *act1 = *it1;
        if (act1->is_uninitialized()) {
            // Ignore the uninitialized stores
            continue;
        }

        ClockVector *cv1 = cvmap.get(act1);
        SCNode *n1 = nodeMap.get(act1);
        ASSERT (cv1);
        DPRINT("********    Checking Location %12p    ********\n", act1->get_location());
        for (; it2 != objList->end(); it2++) {
            ModelAction *act2 = *it2;
            ClockVector *cv2 = cvmap.get(act2);
            SCNode *n2 = nodeMap.get(act2);
            ASSERT (cv2);
            
            // The objList is always ordered by the seq_number of actions
            ASSERT (n1->earlier(n2));

            path_list_t * paths = NULL;
            if (cv2->synchronized_since(act1)) {
                DPRINT("%d -> %d:\n", act1->get_seq_number(), act2->get_seq_number());
                if (act1->is_write() && act2->is_write()) {
                    // Only need to check when the condition is not true:
                    // act2 is a RMW && act1 -rf-> act2
                    if (!(act2->is_rmw() && act2->get_reads_from() == act1)) {
                        if (n1->sbSynchronized(n2)) {
                            // n1 -SB-> n2
                            continue;
                        }

                        // Take out the WW edge from n1->n2
                        SCEdge *incomingEdge = removeIncomingEdge(n1, n2, WW);
                        if (n1->sbRFSynchronized(n2)) {
                            // n1 -sbUrf-> n2
                            paths = findSynchronizablePathsIteratively(n1, n2);
                            if (!imposeSynchronizablePath(n1, n2, paths))
                                return false;
                            ASSERT (!paths->empty());
                        } else {
                            paths = findPathsIteratively(n1, n2);
                            if (!imposeStrongPath(n1, n2, paths))
                                return false;
                        }
                        // Add back the WW edge from n1->n2
                        if (incomingEdge)
                            n2->incoming->push_back(incomingEdge);
                    }
                } else if (act1->is_write() && act2->is_read()) {
                    if (n1->sbSynchronized(n2)) {
                        // n1 -SB-> n2
                        continue;
                    }

                    // Take out the RF edge of n2
                    SCEdge *incomingEdge = removeRFEdge(n2);
                    paths = findPathsIteratively(n1, n2);
                    if (!imposeStrongPath(n1, n2, paths))
                        return false;
                    // Add back the RF edge from n1->n2
                    if (incomingEdge)
                        n2->incoming->push_back(incomingEdge);
                } else if (act1->is_read() && act2->is_write() &&
                    act1->happens_before(act2)) {
                    // Take out the RW edge from n1->n2
                    SCEdge *incomingEdge = removeIncomingEdge(n1, n2, RW);
                    paths = findSynchronizablePathsIteratively(n1, n2);
                    if (!imposeSynchronizablePath(n1, n2, paths))
                        return false;
                    // Add back the WW edge from n1->n2
                    if (incomingEdge)
                        n2->incoming->push_back(incomingEdge);
                }
            } else {
                // act1 and act2 are not ordered
                // We currently impose the seq_cst memory order between stores
                if (act1->is_write() && act2->is_write()) {
                    DPRINT("Unordered writes NOT seq_cst (%d or %d)\n",
                        act1->get_seq_number(),
                        act2->get_seq_number());
                    if (!inference->imposeSC(act1) &&
                        !inference->imposeSC(act2))
                        return false;
                }
            }
        }
    }

    return true;
}

// Compare the cost of paths
static bool comparePath(const SCPath *first, const SCPath *second)
{
    if (first->rfCnt == 0 && first->impliedCnt == 0)
        return true;
    if (second->rfCnt == 0 && second->impliedCnt == 0)
        return false;

    if (first->impliedCnt == 0 && second->impliedCnt > 0)
        return true;
    if (first->impliedCnt > 0 && second->impliedCnt == 0)
        return false;

    if (first->impliedCnt == second->impliedCnt)
        return first->rfCnt < second->rfCnt;

    return first->impliedCnt < second->impliedCnt;
}


bool SCGraph::imposeSynchronizablePath(SCNode *from, SCNode *to, path_list_t *paths) {
    if (paths->empty())
        return true;

    // Sort the path list; if we have a synchronizable path, it must be the
    // first one in the sorted list
    paths->sort(comparePath);
    path_list_t::iterator it = paths->begin();
    SCPath *p = *it;

    if (p->impliedCnt > 0) {
        // We simply don't have a synchronizable path
        return true;
    } else {
        // If so, we simply impose synchronization on path "p"
        DB (
            model_print("**** Checking path (read->write): ");
            p->printWithOrder(to, inference);
        )
        edge_list_t *edges = p->edges;
        if (imposeSyncPath(edges->begin(), edges->end(), from, to, edges)) {
            DPRINT("Synchronized path\n");
            return true;
        } else {
            DPRINT("Unsynchronizable path from read to write!!\n");
            return false;
        }
    }
}


bool SCGraph::imposeStrongPath(SCNode *from, SCNode *to, path_list_t *paths) {
    // Special case: not ordered
    if (paths->empty()) {
        if (to->op->is_read()) {
            return true;
        } else {
            // Right now, make sure from and to are both seq_cst
            if (inference->is_seqcst(from->op) && inference->is_seqcst(to->op)) {
                return true;
            } else {
                DPRINT("Unordered writes NOT seq_cst (%d or %d)\n",
                    from->op->get_seq_number(),
                    to->op->get_seq_number());
                if (!inference->imposeSC(from->op) &&
                    !inference->imposeSC(to->op))
                    return false;
                return true;
            }
        }

    }

    // Find a path to impose strongly SC
    paths->sort(comparePath);
    path_list_t::iterator it = paths->begin();
    SCPath *p = *it;

    DB (
        model_print("**** Checking path: ");
        p->printWithOrder(to, inference);
    )

    if (p->impliedCnt == 0) {
        if (p->rfCnt == 0) {
            // This is a natrual strong path (sb + thread create/start... => hb)
            DPRINT("SB path\n");
            return true;
        } else {
            // A synchronizable path
            edge_list_t *edges = p->edges;
            if (imposeSyncPath(edges->begin(), edges->end(), from, to, edges)) {
                DPRINT("Synchronized path\n");
                return true;
            }
        }
    } else {
        // An SC'able path
        DPRINT("Checking an SCable path\n");
        if (!inference->is_seqcst(to->op) || !inference->is_seqcst(from->op)) {
            // A quick check first; if nodes "from" and "to" are not
            // seq_cst, imediately check another path
            DPRINT("Try to impose SC on %d or %d\n",
                from->op->get_seq_number(), to->op->get_seq_number());
            if (!inference->imposeSC(from->op) || !inference->imposeSC(to->op))
                return false;
        }
        // Find the synchronizable subpaths 
        edge_list_t *edges = p->edges;
        // "fromNode" and "endNode" represents the head and tail of a
        // synchronizable subpath
        edge_list_t::iterator curIter = edges->begin(),
            beginIter = curIter, endIter = curIter;
        SCNode *fromNode = to, *toNode = to, *curNode = to;
        while (true) {
            // toNode points to the end of the subpath (already seq_cst);
            // fromNode points to the head of the subpath;
            // when they are equals, only one node is in the path;
            // curNode is the node that the edge "e" points to
            SCEdge *e = *curIter;
            SCNode *dest = e->node;

            if (e->type == RF || e->type == SB) {
                // Simply update the curNode & increase curIter
                curNode = e->node;
                curIter++;
                if (dest == from) {
                    // This is the end of the last subpath, from becomes the
                    // fromNode
                    endIter = edges->end();
                    if (!imposeSyncPath(beginIter, endIter, from, toNode,
                        edges)) {
                        // This is not a strong path
                        DPRINT("NOT an SC'able path (%d -> %d is not synchronized)\n",
                            from->op->get_seq_number(), toNode->op->get_seq_number());
                        return false;
                    } else {
                        // Reach the very first subpath, done!
                        DPRINT("SC'ed path\n");
                        return true;
                    }
                }
            } else { // This is either RW or WW
                // Found the from node, i.e. the curNode
                fromNode = curNode;
                // First check seq_cst of fromNode
                if (!inference->is_seqcst(fromNode->op)) {
                    DPRINT("Try to impose SC on %d\n",
                        fromNode->op->get_seq_number());
                    if (!inference->imposeSC(fromNode->op))
                        return false;
                }
                if (fromNode != toNode) {
                    // More than one node in this "subpath"
                    endIter = curIter;
                    if (!imposeSyncPath(beginIter, endIter, fromNode,
                        toNode, edges)) {
                        // This is not a strong path
                        DPRINT("NOT an SC'ed path (%d -> %d is not synchronized)\n",
                            fromNode->op->get_seq_number(), toNode->op->get_seq_number());
                        return false;
                    }
                }
                // Update the iterators and nodes for the next subpath
                curIter++;
                if (curIter == edges->end()) {
                    // The end of the search, and this is a strong path
                    DPRINT("SC'ed path\n");
                    return true;
                }
                // Otherwise update the invariables
                beginIter = curIter;
                curNode = dest;
                toNode = dest;
                // Also make sure that the toNode is always checked
                if (!inference->is_seqcst(toNode->op)) {
                    // This is not a strong path
                    DPRINT("Try to impose SC on %d\n",
                        toNode->op->get_seq_number());
                    if (!inference->imposeSC(toNode->op))
                        return false;
                }
            }
        }
    }
    
    return true;
}

/**
    When we call this method, the begin iterator points to the first edge that
    goes to the "to" node.
*/
bool SCGraph::imposeSyncPath(edge_list_t::iterator begin, edge_list_t::iterator
    end, SCNode *from, SCNode *to, edge_list_t *edges) {

    DPRINT("**** Checking synchronization %d -> %d: ",
        from->op->get_seq_number(), to->op->get_seq_number());

    // Try a simple release-sequence-type synchronization
    // Find a continuous rf chain
    bool rfStart = false;
    bool rfEnd = false;
    bool isRelSeq = true;
    const ModelAction *relHead = NULL, *relTail = NULL;
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

    if (relHead && relTail && isRelSeq) {
        if (inference->is_release(relHead) && inference->is_acquire(relTail)) {
            DPRINT("Synchronized path (release sequence)\n");
            return true;
        } else {
            DPRINT("Try to synchronize path (release sequence) (%d or %d is not rel/acq)\n",
                relHead->get_seq_number(), relTail->get_seq_number());
            if (inference->imposeRelease(relHead) &&
                inference->imposeAcquire(relTail))
                return true;
            else
                return false;
        }
    } else { // Simply require every reads-from edge to be release/acquire
        curNode = to;
        for (edge_list_t::iterator it = begin; it != end; it++) {
            SCEdge *e = *it;
            if (e->type == RF) {
                const ModelAction *write = e->node->op;
                if (!inference->is_release(write) ||
                    !inference->is_acquire(curNode->op)) {
                    DPRINT("Try to synchronize path (%d or %d is not rel/acq)\n",
                        write->get_seq_number(),
                        curNode->op->get_seq_number());
                    if (!inference->imposeRelease(write) ||
                        !inference->imposeAcquire(curNode->op))
                        return false;
                }
            }
            curNode = e->node;
        }
    }

    DPRINT("Synchronized path (normal)\n");
    return true;
}

SCEdge * SCGraph::removeRFEdge(SCNode *read) {
    ASSERT (read->op->is_read());
    EdgeList *incomingEdges = read->incoming;
    for (unsigned i = 0; i < incomingEdges->size(); i++) {
        SCEdge *existing  = (*incomingEdges)[i];
        if (existing->type == RF) {
            DPRINT("Take out the %d -RF->%d edge temporarily\n",
                existing->node->op->get_seq_number(),
                read->op->get_seq_number());
            incomingEdges->erase(incomingEdges->begin() + i);
            return existing;
        }
    }

    // A read must have a store to read from
    ASSERT (false);
}

SCEdge * SCGraph::removeIncomingEdge(SCNode *from, SCNode *to, SCEdgeType type) {
    DPRINT("Take out the %d -%s-> %d edge temporarily\n", from->op->get_seq_number(),
        get_edge_str(type), to->op->get_seq_number());
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
    return NULL;
}

static void printSpace(int num) {
    for (int i = 0; i < num; i++)
        model_print(" ");
}

// Find all the synchronizable paths (sb U rf) iteratively; the main algorithm
// is the same is the findPathsIteratively() method except that it only looks
// for SB & RF edges.
path_list_t * SCGraph::findSynchronizablePathsIteratively(SCNode *from, SCNode *to) {

    DPRINT("****  Iteratively finding synchronizable path %d -> %d  ****\n",
        from->op->get_seq_number(), to->op->get_seq_number());
    // The main stack we use to maintain the choices and backtrack
    SnapList<EdgeChoice *> edgeStack; 
    // The list to store our paths
    path_list_t *result = new path_list_t;

    unsigned fromSeqNum = from->op->get_seq_number();
    // Initially push back the 0th edge of to's incoming edges
    edgeStack.push_back(new EdgeChoice(to, 0, 0));
    // The first edge must be either an SB or RF edge
    ASSERT ((*to->incoming)[0]->type <= RF);
    DPRINT("Push back: (%d, %d, %d)\n", to->op->get_seq_number(), 0, 0);

    // Main loop
    while (!edgeStack.empty()) {
        EdgeChoice *choice = edgeStack.back();
        SCNode *n = choice->n;
        EdgeList *incomingEdges = n->incoming;
        DPRINT("Peek back: (%d, %d, %d)\n", n->op->get_seq_number(), choice->i,
            choice->finished);

        if ((*incomingEdges)[choice->i]->type > RF) {
            // Backtrack since we only look for SB & RF edges
            // The current edge choice is done, backtrack
            DPRINT("RW|WW edge backtrack: \n");
            // FIXME: We can actually give up very early here since all other
            // edges with bigger index should be RW|WW edges
            if (incomingEdges->size() != choice->i + 1) {
                // We don't need to pop the choice yet, just update the choice
                choice->finished = 0;
                choice->i++;
                DPRINT("Advance choice: (%d, %d, %d)\n",
                    n->op->get_seq_number(), choice->i, choice->finished);
            }
        }

        if (choice->finished == 0) { // The current choice of edge is done
            choice->finished = 1; // Don't need to pop the edge choice yet
            // Add the first incoming edge of "n"
            SCEdge *e = (*incomingEdges)[choice->i];
            SCNode *n0 = e->node;

            // This is also an ending condition, backtrack
            if (n0->incoming->empty()) {
                DPRINT("Empty choice: (%d, %d, %d)\n", n->op->get_seq_number(),
                    choice->i, choice->finished);
                if (incomingEdges->size() != choice->i + 1) {
                    // We don't need to pop the choice yet, just update the choice
                    choice->finished = 0;
                    choice->i++;
                    DPRINT("Advance choice: (%d, %d, %d)\n",
                        n->op->get_seq_number(), choice->i, choice->finished);
                } else {
                    // Ready to pop out the choice
                    edgeStack.pop_back();
                    DPRINT("Pop back: (%d, %d, %d)\n", n->op->get_seq_number(),
                        choice->i, choice->finished);
                }
                // Go back to the loop
                continue;
            }

            // Otherwise, push back the first edge of n0
            if (n0->op->get_seq_number() == fromSeqNum) {
                // Found a path, build the path
                SCPath *p = new SCPath;
                // Add the first edge
                DPRINT("Add edge: ");
                for (SnapList<EdgeChoice *>::reverse_iterator iter =
                    edgeStack.rbegin(); iter != edgeStack.rend();
                    iter++) {
                    EdgeChoice *c = *iter;
                    SCEdge *e0 = (*c->n->incoming)[c->i];
                    DPRINT("%d -> ", e0->node->op->get_seq_number());
                    p->addEdgeFromFront(e0);
                }
                DPRINT("%d\n", to->op->get_seq_number());
                // Add the path to the result list
                result->push_back(p);
                DPRINT("Found path:");
                DB( p->printWithOrder(to, inference); )
            } else if (from->earlier(n0)) {
                // Haven't found the "from" node yet, just keep pushing to
                // the stack
                edgeStack.push_back(new EdgeChoice(n0, 0, 0));
                DPRINT("Push back: (%d, %d, %d)\n", n0->op->get_seq_number(), 0,
                    0);
            }
        } else {
            // The current edge choice is done, backtrack
            DPRINT("Finished backtrack: \n");
            if (incomingEdges->size() != choice->i + 1) {
                // We don't need to pop the choice yet, just update the choice
                choice->finished = 0;
                choice->i++;
                DPRINT("Advance choice: (%d, %d, %d)\n",
                    n->op->get_seq_number(), choice->i, choice->finished);
            } else {
                // Ready to pop out the choice
                edgeStack.pop_back();
                DPRINT("Pop back: (%d, %d, %d)\n", n->op->get_seq_number(),
                    choice->i, choice->finished);
            }
        }

        if (n->op->get_seq_number() < fromSeqNum ||
            n->op->is_uninitialized()) {
            // Node n's seq_num > from's seq_num, subpaths should be empty
            // Since it's an SC execution
        }
    }
    
    DB (
        int cnt = 1;
        for (path_list_t::iterator it = result->begin(); it != result->end();
            it++, cnt++) {
            SCPath *p = *it;
            model_print("#%d. ", cnt);
            p->printWithOrder(to, inference);
        }
        DPRINT("****  Iteratively finding syncrhonizable path (done) %d -> %d  ****\n",
            from->op->get_seq_number(), to->op->get_seq_number());
    )

    return result;
}


// Find all the paths iteratively; we basically use a depth-first search
// backwards from the node "to". We end a single search whenever we either: 1)
// reach the node "from" --- we find a path; or 2) we reach a node whose
// sequence number is smaller than that of node "from" --- since in an SC trace
// there can't be an edge from backwards.
path_list_t * SCGraph::findPathsIteratively(SCNode *from, SCNode *to) {

    DPRINT("****  Iteratively finding path %d -> %d  ****\n", from->op->get_seq_number(),
        to->op->get_seq_number());
    // The main stack we use to maintain the choices and backtrack
    SnapList<EdgeChoice *> edgeStack; 
    // The list to store our paths
    path_list_t *result = new path_list_t;

    unsigned fromSeqNum = from->op->get_seq_number();
    // Initially push back the 0th edge of to's incoming edges
    edgeStack.push_back(new EdgeChoice(to, 0, 0));
    DPRINT("Push back: (%d, %d, %d)\n", to->op->get_seq_number(), 0, 0);

    // Main loop
    while (!edgeStack.empty()) {
        EdgeChoice *choice = edgeStack.back();
        SCNode *n = choice->n;
        EdgeList *incomingEdges = n->incoming;
        DPRINT("Peek back: (%d, %d, %d)\n", n->op->get_seq_number(), choice->i,
            choice->finished);

        if (choice->finished == 0) { // The current choice of edge is done
            choice->finished = 1; // Don't need to pop the edge choice yet
            // Add the first incoming edge of "n"
            SCEdge *e = (*incomingEdges)[choice->i];
            SCNode *n0 = e->node;

            // This is also an ending condition, backtrack
            if (n0->incoming->empty()) {
                DPRINT("Empty choice: (%d, %d, %d)\n", n->op->get_seq_number(),
                    choice->i, choice->finished);
                if (incomingEdges->size() != choice->i + 1) {
                    // We don't need to pop the choice yet, just update the choice
                    choice->finished = 0;
                    choice->i++;
                    DPRINT("Advance choice: (%d, %d, %d)\n",
                        n->op->get_seq_number(), choice->i, choice->finished);
                } else {
                    // Ready to pop out the choice
                    edgeStack.pop_back();
                    DPRINT("Pop back: (%d, %d, %d)\n", n->op->get_seq_number(),
                        choice->i, choice->finished);
                }
                // Go back to the loop
                continue;
            }

            // Otherwise, push back the first edge of n0
            if (n0->op->get_seq_number() == fromSeqNum) {
                // Found a path, build the path
                SCPath *p = new SCPath;
                // Add the first edge
                DPRINT("Add edge: ");
                for (SnapList<EdgeChoice *>::reverse_iterator iter =
                    edgeStack.rbegin(); iter != edgeStack.rend();
                    iter++) {
                    EdgeChoice *c = *iter;
                    SCEdge *e0 = (*c->n->incoming)[c->i];
                    DPRINT("%d -> ", e0->node->op->get_seq_number());
                    p->addEdgeFromFront(e0);
                }
                DPRINT("%d\n", to->op->get_seq_number());
                // Add the path to the result list
                result->push_back(p);
                DPRINT("Found path:");
                DB( p->printWithOrder(to, inference); )
            } else if (from->earlier(n0)) {
                // Haven't found the "from" node yet, just keep pushing to
                // the stack
                edgeStack.push_back(new EdgeChoice(n0, 0, 0));
                DPRINT("Push back: (%d, %d, %d)\n", n0->op->get_seq_number(), 0,
                    0);
            }
        } else {
            // The current edge choice is done, backtrack
            DPRINT("Finished backtrack: \n");
            if (incomingEdges->size() != choice->i + 1) {
                // We don't need to pop the choice yet, just update the choice
                choice->finished = 0;
                choice->i++;
                DPRINT("Advance choice: (%d, %d, %d)\n",
                    n->op->get_seq_number(), choice->i, choice->finished);
            } else {
                // Ready to pop out the choice
                edgeStack.pop_back();
                DPRINT("Pop back: (%d, %d, %d)\n", n->op->get_seq_number(),
                    choice->i, choice->finished);
            }
        }

        if (n->op->get_seq_number() < fromSeqNum ||
            n->op->is_uninitialized()) {
            // Node n's seq_num > from's seq_num, subpaths should be empty
            // Since it's an SC execution
        }
    }
    
    DB (
        int cnt = 1;
        for (path_list_t::iterator it = result->begin(); it != result->end();
            it++, cnt++) {
            SCPath *p = *it;
            model_print("#%d. ", cnt);
            p->printWithOrder(to, inference);
        }
        DPRINT("****  Iteratively finding path (done) %d -> %d  ****\n",
            from->op->get_seq_number(), to->op->get_seq_number());
    )

    return result;
}

// The depth parameter is by default 0; and we only use that for the purpose of
// generating debugging output
path_list_t * SCGraph::findPaths(SCNode *from, SCNode *to, int depth) {
    DB( 
        int spaceCnt = depth * 2;
        printSpace(spaceCnt);
    )
    DPRINT("Depth %d: Finding path %d -> %d\n", depth,
        from->op->get_seq_number(), to->op->get_seq_number());
    EdgeList *edges = to->incoming;
    path_list_t *result = new path_list_t;
    for (unsigned i = 0; i < edges->size(); i++) {
        SCEdge *e = (*edges)[i];
        SCNode *n = e->node;
        path_list_t *subpaths = NULL;
        
        DB( printSpace(spaceCnt); )
        DPRINT("Edge %d:  %d -> %d\n", i + 1, n->op->get_seq_number(),
            to->op->get_seq_number());
        // Then recursively find all the paths from "from" to "n"
        if (n == from) {
            // Node "n" is equals to node "from", end of search
            // subpaths contain only 1 edge (i.e., e)
            DB( printSpace(spaceCnt); )
            DPRINT("Hitting beginning node\n");
            
            // Immediately add this path to "result"
            SCPath *p = new SCPath;
            p->addEdgeFromFront(e);
            result->push_back(p);
        } else if (n->op->get_seq_number() < from->op->get_seq_number() ||
            n->op->is_uninitialized()) {
            // Node n's seq_num > from's seq_num, subpaths should be empty
            // Since it's an SC execution
            DB( printSpace(spaceCnt); )
            DPRINT("Not the right edge\n");
            subpaths = NULL;
        } else {
            // Find all the paths: "from" -> "n"
            subpaths = findPaths(from, n, depth + 1);
        }

        // Then attach the edge "e" to the paths
        if (subpaths != NULL && !subpaths->empty()) {
            addPathsFromSubpaths(result, subpaths, e);
            // This should be the right time to delete subpaths
            // But make sure to check subpaths is not null
            if (subpaths) {
                delete subpaths;
            }
        }
    }

    DB (
    if (result->empty()) {
        printSpace(spaceCnt);
        model_print("Depth %d: NO path from %d -> %d\n", depth,
            from->op->get_seq_number(), to->op->get_seq_number());
    } else {
        printSpace(spaceCnt);
        model_print("Depth %d: Found paths from %d -> %d\n", depth,
            from->op->get_seq_number(), to->op->get_seq_number());
        int cnt = 1;
        for (path_list_t::iterator it = result->begin(); it != result->end();
            it++, cnt++) {
            SCPath *p = *it;
            printSpace(spaceCnt);
            model_print("#%d. ", cnt);
            p->printWithOrder(to, inference);
        }
    }
    )
    
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

// Compare the cost of an edge
static bool compareEdge(const SCEdge *e1, const SCEdge *e2)
{
    return e1->type <= e2->type;
}


// Sort the edges in order of SB, RF and (RW|WW)
void SCGraph::sortEdges() {
    // FIXME: We probably don't need this
}

void SCGraph::buildGraph() {
    DB(
        model_print("****  Pre-execution inference  ****\n");
        inference->print();
    )

    buildVectors();
    computeCV();
    computeSBCV();
    sortEdges();
    
    DB(
        print();
        printInfoPerLoc();
    )

    checkStrongSC();

    DB(
        model_print("****  Post-execution inference  ****\n");
        inference->print();
    )
}


void SCGraph::print() {
    model_print("**********    SC Graph    **********\n");
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
            // Update the clock vector
            fromNode->mergeSB(n);
        }

        // Add the sb edge
        action_list_t *threadlist = &threadlists[threadid];
        if (threadlist->size() > 0) {
            from = threadlist->back();
            fromNode = nodeMap.get(from);
            addEdge(fromNode, n, SB);
            // Update the clock vector
            fromNode->mergeSB(n);
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


// Compute the SB clock vectors
void SCGraph::computeSBCV() {
    action_list_t::iterator secondLastIter = actions->end();
    secondLastIter--;
    for (action_list_t::iterator it = actions->begin(); it != secondLastIter; ) {
        ModelAction *curAct = *it;
        it++;
        ModelAction *nextAct = *it;
        SCNode *curNode = nodeMap.get(curAct);
        SCNode *nextNode = nodeMap.get(nextAct);
        if (nextNode->sbCV->synchronized_since(curNode->op)) {
            curNode->mergeSB(nextNode);
        }
    }
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


