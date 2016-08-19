#include "scgraph.h"
#include "common.h"
#include "action.h"
#include "threads-model.h"
#include "clockvector.h"
#include "execution.h"
#include "strongsc_common.h"
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

/**********    SCNode    **********/
SCNode::SCNode(ModelAction *op) :
    op(op),
    incoming(new EdgeList),
    outgoing(new EdgeList),
    sbCV (new ClockVector(NULL, op)),
    writeReadCV (new ClockVector(NULL, op)),
    readWriteCV (new ClockVector(NULL, op)) {
}


// For SB reachability
bool SCNode::mergeSB(SCNode *dest) {
    return dest->sbCV->merge(sbCV);
}

bool SCNode::sbSynchronized(SCNode *dest) {
    // The uninitialized store is SB before any other stores 
    if (op->is_uninitialized())
        return true;
    return dest->sbCV->synchronized_since(op);
}

// For write-read ordering reachability
bool SCNode::mergeWriteRead(SCNode *dest) {
    return dest->writeReadCV->merge(writeReadCV);
}

bool SCNode::writeReadSynchronized(SCNode *dest) {
    return dest->writeReadCV->synchronized_since(op);
}

// For read-write ordering reachability
bool SCNode::mergeReadWrite(SCNode *dest) {
    return dest->readWriteCV->merge(readWriteCV);
}

bool SCNode::readWriteSynchronized(SCNode *dest) {
    return dest->readWriteCV->synchronized_since(op);
}

// For sbUrf reachability
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

void SCPath::printWithOrder(SCNode *tailNode, bool lineBreak) {
    edge_list_t::reverse_iterator it = edges->rbegin();
    SCEdge *e = *it;
    SCNode *n = e->node;
    // The wildcard ID 
    int wildcard = 0;
    // The current wildcard memory order
    for (; it != edges->rend(); it++) {
        e = *it;
        n = e->node;
        if (is_wildcard(n->op->get_original_mo())) {
            wildcard = get_wildcard_id(n->op->get_original_mo());
            model_print("%d(w%d) -%s-> ", n->op->get_seq_number(), wildcard,
                get_edge_str(e->type));
        } else {
            model_print("%d(\"%s\") -%s-> ", n->op->get_seq_number(),
                n->op->get_mo_str(), get_edge_str(e->type));
        }
    }
    
    if (is_wildcard(tailNode->op->get_original_mo())) {
        wildcard = get_wildcard_id(tailNode->op->get_original_mo());
        model_print("%d(w%d)", tailNode->op->get_seq_number(), wildcard);
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
SCGraph::SCGraph(ModelExecution *e, action_list_t *actions, AssignList
    *assignments) :
    execution(e),
    actions(actions),
    objLists(),
    objSet(),
    nodes(),
    nodeMap(),
    assignments(assignments),
	cvmap(),
    threadlists(1) {
    buildGraph();
}

SCGraph::~SCGraph() {

}

// Print the graph to a dot file
void SCGraph::printToFile() {
    int execNum = execution->get_execution_number();

    model_print("****  Dot graph for exec %d  ****\n", execNum);
    model_print("digraph G%d {\n", execNum);
    model_print("\tmargin=0\n\n");

    for (node_list_t::iterator it = nodes.begin(); it != nodes.end(); it++) {
        SCNode *n = *it;
        int seq = n->op->get_seq_number();
        for (unsigned i = 0; i < n->incoming->size(); i++) {
            SCEdge *e = (*n->incoming)[i];
            SCNode *n1 = e->node;
            if (n1->op->is_uninitialized())
                continue;
            int seq1 = n1->op->get_seq_number();
            switch (e->type) {
                case SB:
                model_print("\t%d -> %d [label=\"sb\", color=black];\n", seq1,
                    seq);
                break;
                case RF:
                model_print("\t%d -> %d [label=\"rf\", color=red, penwidth=1.5];\n",
                    seq1, seq);
                break;
                case RW:
                model_print("\t%d -> %d [label=\"rw\", color=darkgreen, style=dashed];\n",
                    seq1, seq);
                break;
                case WW:
                model_print("\t%d -> %d [label=\"ww\", color=darkgreen, style=dashed];\n",
                    seq1, seq);
                break;
                default:
                ASSERT (false);
            }
        }
    }

    model_print("}\n");
}

// Print the graph 
void SCGraph::print() {
    model_print("**********    SC Graph    **********\n");
    for (node_list_t::iterator it = nodes.begin(); it != nodes.end(); it++) {
        SCNode *n = *it;
        n->print();
    }
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
            if (act->is_read() || act->is_write()) {
                model_print("  ");
                act->print();
            }
        }
    }
    model_print("**********    Location List (end)    **********\n");
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


// Returns true when w1&w2 are ordered (a path without edge "w1 -WW-> w2")
bool SCGraph::imposeStrongPathWriteWrite(SCNode *w1, SCNode *w2) {
    ASSERT(w1->op->is_write() && w2->op->is_write());
    ASSERT(w1->op->get_location() == w1->op->get_location());
    DPRINT("Impose strong path %d -WW-> %d: ", w1->op->get_seq_number(),
        w2->op->get_seq_number());

    DB (
        if (cvmap.get(w2->op)->synchronized_since(w1->op)) {
            DPRINT("Synced\n");
        }
    )

    patch_list_t *patches = NULL;
    if (w2->op->is_rmw() && w2->op->get_reads_from() == w1->op) {
        return true;
    } else {
        if (w1->sbSynchronized(w2)) {
            // w1 -SB-> w2
            return true;
        } else {
            path_list_t *paths = NULL;
            if (w1->sbRFSynchronized(w2)) {
                // w1 -sbUrf-> w2
                paths = findSynchronizablePaths(w1, w2);
                ASSERT (!paths->empty());
                patches = imposeSynchronizablePath(w1, w2, paths);
                if (patches && !patches->empty()) {
                    assignments->applyPatches(patches);
                }
                return true;
            } else {
                if (cvmap.get(w2->op)->synchronized_since(w1->op)) {
                    // Take out the WW edge from w1->w2
                    int edgeIndex = removeIncomingEdge(w1, w2, WW);
                    paths = findPaths(w1, w2);
                    // Add back the WW edge from w1->w2
                    if (edgeIndex != -1)
                        addBackIncomingEdge(w2, edgeIndex, WW);
                    if (paths->empty()) {
                        // This means w1 -sync-> r && w2 -rf->r
                        EdgeList *outgoingEdges = w2->outgoing;
                        for (unsigned i = 0; i < outgoingEdges->size(); i++) {
                            SCEdge *e = (*outgoingEdges)[i];
                            if (e->type != RF)
                                continue;
                            SCNode *read = e->node;
                            if (imposeStrongPathWriteRead(w1, read)) {
                                return true;
                            }
                        }
                        // There must be at least one such "r"
                        ASSERT (false);
                    } else {
                        patches = imposeStrongPath(w1, w2, paths);
                        if (patches && !patches->empty()) {
                            assignments->applyPatches(patches);
                        }
                        return true;
                    }
                } else {
                    // Stores are not ordered
                    // We currently impose the seq_cst memory order
                    DPRINT("unordered writes\n");
                    SCPatch *p = new SCPatch(w1->op, memory_order_seq_cst,
                        w2->op, memory_order_seq_cst);
                    patches = new patch_list_t;
                    SCPatch::addPatchToList(patches, p);
                    assignments->applyPatches(patches);
                    return false;
                }
            }
        }
    }
}


// Returns true when w&r are ordered without the RF edge to "r"
bool SCGraph::imposeStrongPathWriteRead(SCNode *w, SCNode *r) {
    ASSERT(w->op->is_write() && r->op->is_read());
    ASSERT(w->op->get_location() == w->op->get_location());
    DPRINT("Impose strong path %d -WR-> %d:\n", w->op->get_seq_number(),
        r->op->get_seq_number());
    if (w->sbSynchronized(r)) {
        // w -SB-> r
        DPRINT("%d -WR-> %d: Synced\n", w->op->get_seq_number(),
            r->op->get_seq_number());
        return true;
    } else {
        path_list_t *paths = NULL;
        // Take out the RF edge of "r" 
        unsigned rfIndex = removeRFEdge(r);
        // First try to find synchronizable paths
        paths = findSynchronizablePaths(w, r);
        patch_list_t *patches = NULL;
        if (!paths->empty()) {
            patches = imposeSynchronizablePath(w, r, paths);
            // Add back the RF edge from w->r
            addBackRFEdge(r, rfIndex);
            if (patches && !patches->empty()) {
                assignments->applyPatches(patches);
            }
            DPRINT("%d -WR-> %d: Synced\n", w->op->get_seq_number(),
                r->op->get_seq_number());
            return true;
        } else {
            delete paths;
            paths = findPaths(w, r);
            // Add back the RF edge from w->r
            addBackRFEdge(r, rfIndex);
            if (!paths->empty()) {
                patches = imposeStrongPath(w, r, paths);
                if (patches && !patches->empty()) {
                    assignments->applyPatches(patches);
                }
                DPRINT("%d -WR-> %d: Synced\n", w->op->get_seq_number(),
                    r->op->get_seq_number());
                return true;
            }
        }
    }
    
    DPRINT("%d -WR-> %d: NOT synced\n", w->op->get_seq_number(),
        r->op->get_seq_number());
    return false;
}

// Returns true when r&w are ordered by a rfUsb edge
bool SCGraph::imposeStrongPathReadWrite(SCNode *r, SCNode *w) {
    ASSERT(r->op->is_read() && w->op->is_write());
    ASSERT(w->op->get_location() == w->op->get_location());
    
    DPRINT("Impose strong path %d -RW-> %d:\n", r->op->get_seq_number(),
        w->op->get_seq_number());
    if (r->sbSynchronized(w)) {
        DPRINT("%d -RW-> %d: Synced\n", w->op->get_seq_number(),
            r->op->get_seq_number());
        return true;
    } else if (r->sbRFSynchronized(w)) {
        // r -sbUrf-> w
        path_list_t *paths = findSynchronizablePaths(r, w);
        ASSERT (!paths->empty());
        patch_list_t *patches = imposeSynchronizablePath(r, w, paths);
        DPRINT("%d -RW-> %d: Synced\n", w->op->get_seq_number(),
            r->op->get_seq_number());
        if (patches && !patches->empty()) {
            assignments->applyPatches(patches);       
        }
        return true;
    }
    return false;
}

// The core of checking strong SCness per memory location
void SCGraph::computeLocCV(action_list_t *objList) {
    action_list_t::iterator it = objList->begin();
    ModelAction *act = *it;
    void *loc = act->get_location();
    if (act->is_read() || act->is_write())
        DPRINT("********    Computing Location CV %12p    ********\n", loc);
    else
        return;

    DB (
        DPRINT("********    List on %p    ********\n", loc);
        for (action_list_t::iterator objIt = objList->begin(); objIt != objList->end(); objIt++) {
            ModelAction *cur = *objIt;
                model_print("  ");
                cur->print();
        }
    )

    DPRINT("********    First round (WW & WR)    ********\n");
    // First sync on all the write->write & write->read to the same location
    for (it = objList->begin(); it != objList->end(); it++) {
        act = *it;
        if (act->is_uninitialized())
            continue;

        SCNode *actNode = nodeMap.get(act);
        ASSERT (act->is_write() || act->is_read());

        // Then start to go backward from act 
        action_list_t::iterator it1 = it;
        it1--;
        if (act->is_write()) {
            // act is a write 
            for (; it1 != objList->begin(); it1--) {
                ModelAction *write = *it1;
                ASSERT (write);
                SCNode *writeNode = nodeMap.get(write);
                if (!write->is_write())
                    continue;
                // Sync write -WW-> act
                DPRINT("W1=%d, ", write->get_seq_number());
                writeNode->writeReadCV->print();
                DPRINT("W2=%d, ", act->get_seq_number());
                actNode->writeReadCV->print();
                if (cvmap.get(act)->synchronized_since(write)) {
                    if (!writeNode->writeReadSynchronized(actNode)) {
                        bool res = imposeStrongPathWriteWrite(writeNode, actNode);
                        // Strong path between writes means modification order
                        // write -isc-> act => write -mo-> act
                        ASSERT (res);
                    }
                    // write -sync-> act (in write-read cv)
                    writeNode->mergeWriteRead(actNode);
                    DPRINT("W2=%d, ", act->get_seq_number());
                    actNode->writeReadCV->print();
                } else {
                    // Stores are not ordered
                    // We currently impose the seq_cst memory order
                    DPRINT("Unordered writes\n");
                    SCPatch *p = new SCPatch(act, memory_order_seq_cst,
                        write, memory_order_seq_cst);
                    patch_list_t *patches = new patch_list_t;
                    SCPatch::addPatchToList(patches, p);
                    assignments->applyPatches(patches);
                    return;
                }
            }   
        } else {
            // act is a read
            for (; it1 != objList->begin(); it1--) {
                ModelAction *write = *it1;
                ASSERT (write);
                SCNode *writeNode = nodeMap.get(write);
                if (!write->is_write())
                    continue;
                DPRINT("W1=%d, ", write->get_seq_number());
                writeNode->writeReadCV->print();
                DPRINT("R1=%d, ", act->get_seq_number());
                actNode->writeReadCV->print();
                if (writeNode->writeReadSynchronized(actNode)) {
                    // Sync write -WR-> act 
                    writeNode->mergeWriteRead(actNode);
                    DPRINT("R1=%d, ", act->get_seq_number());
                    actNode->writeReadCV->print();
                } else if (imposeStrongPathWriteRead(writeNode, actNode)) {
                    // Sync write -WR-> act 
                    writeNode->mergeWriteRead(actNode);
                    DPRINT("R1=%d, ", act->get_seq_number());
                    actNode->writeReadCV->print();
                }
            }
        }
    }

    DPRINT("********    Second round (RW)    ********\n");
    // Then sync on all the read->write to the same location
    for (it = objList->begin(); it != objList->end(); it++) {
        ModelAction *act = *it;
        if (act->is_uninitialized())
            continue;
        ASSERT (act);
        SCNode *actNode = nodeMap.get(act);
        if (!act->is_write())
            continue;
        // Then start to go backward from act 
        action_list_t::iterator it1 = it;
        it1--;
        // act is a write 
        for (; it1 != objList->begin(); it1--) {
            ModelAction *act0 = *it1;
            ASSERT (act0);
            SCNode *actNode0 = nodeMap.get(act0);
            if (act0->is_write()) {
                DPRINT("W1=%d, ", act0->get_seq_number());
                actNode0->readWriteCV->print();
                DPRINT("W2=%d, ", act->get_seq_number());
                actNode->readWriteCV->print();
                // act0 is a write
                if (cvmap.get(act)->synchronized_since(act0)) {
                    actNode0->mergeReadWrite(actNode);
                    DPRINT("W2=%d, ", act->get_seq_number());
                    actNode->readWriteCV->print();
                }
            } else {
                // act0 is a read 
                DPRINT("R1=%d, ", act0->get_seq_number());
                actNode0->readWriteCV->print();
                DPRINT("W2=%d, ", act->get_seq_number());
                actNode->readWriteCV->print();
                if (cvmap.get(act)->synchronized_since(act0)) {
                    if (actNode0->readWriteSynchronized(actNode)) {
                        actNode0->mergeReadWrite(actNode);
                        DPRINT("W2=%d, ", act->get_seq_number());
                        actNode->readWriteCV->print();
                    } else if (imposeStrongPathReadWrite(actNode0, actNode)) {
                        // Sync act0 -RW-> act 
                        actNode0->mergeReadWrite(actNode);
                        DPRINT("W2=%d, ", act->get_seq_number());
                        actNode->readWriteCV->print();
                    }
                }
            }
        }
    }

    DPRINT("********    Computing Location CV %12p (end)    ********\n", loc);
}


// Ensure that every memory location is strongly SC in this execution
bool SCGraph::checkStrongSCPerLoc(action_list_t *objList) {
    if (objList->empty())
        return true;

    ModelAction *act = (*objList->begin());
    if (act->is_read() || act->is_write())
        model_print("Location %14p:\n", act->get_location());
    else
        return true;
    computeLocCV(objList);
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


// Check whether a path is single location
bool SCGraph::isSingleLocPath(SCNode *from, SCNode *to, path_list_t *paths) {
    void *loc = to->op->get_location();
    for (path_list_t::iterator it = paths->begin(); it != paths->end(); it++) {
        SCPath *p = *it;
        edge_list_t *edges = p->edges;
        bool flag = true;
        SCNode *cur = to;
        for (edge_list_t::iterator edgeIt = edges->begin(); edgeIt !=
            edges->end(); edgeIt++) {
            SCEdge *e = *edgeIt;
            SCNode *n = e->node;
            if (n->op->get_location() == loc) {
                cur = n;
            } else {
                // ! (n -SB-> cur || n -RF-> cur)
                if (!(n->sbSynchronized(cur) || (cur->op->is_read() &&
                    cur->op->get_reads_from() == n->op))) {
                    flag = false;
                    break;
                }
            }
        }
        if (flag)
            return true;
    }

    return false;
}


// Given a list of rfUsb paths, returns a list of patches that can guarantee
// the paths are synchronized paths
patch_list_t* SCGraph::imposeSynchronizablePath(SCNode *from, SCNode *to, path_list_t *paths) {
    if (paths->empty())
        return NULL;

    if (isSingleLocPath(from, to, paths)) {
        DPRINT("Single location path\n");
        return NULL;
    }

    // Sort the path list; if we have a synchronizable path, it must be the
    // first one in the sorted list
    paths->sort(comparePath);
    path_list_t::iterator it = paths->begin();
    patch_list_t *patches = new patch_list_t;
    for (; it != paths->end(); it++) {
        SCPath *p = *it;
        ASSERT (p->impliedCnt == 0);

        DB (
            model_print("Checking synchronizable path: ");
            p->printWithOrder(to);
        )
        edge_list_t *edges = p->edges;
        SCPatch *patch = new SCPatch;
        imposeSynchronizableSubpath(edges->begin(), edges->end(), from, to,
            edges, patch );
        if (patch->getSize() > 0) {
            SCPatch::addPatchToList(patches, patch);
        } else {
            delete patch;
        }
    }

    return patches;
}


/**
    
    Given a list of paths, returns a list of patches that can guarantee at least
    there's one strong path.
    We only call this method when there's no suitable synchronizable path
    "from -> to"

*/
patch_list_t* SCGraph::imposeStrongPath(SCNode *from, SCNode *to, path_list_t *paths) {
    ASSERT (!paths->empty());

    // Find some paths to impose strongly SC
    paths->sort(comparePath);

    path_list_t::iterator it = paths->begin();
    int cnt = 0;
    // FIXME: Right now we only consider one SC path
    int NUM_PATH_CONSIDERING = 1;
    patch_list_t *patches = new patch_list_t;
    for (; cnt != NUM_PATH_CONSIDERING; cnt++, it++) {
        SCPath *path = *it;
        DB (
            model_print("Checking SC path: ");
            path->printWithOrder(to);
        )
        
        SCPatch *patch = new SCPatch;
        patch->addPatchUnit(from->op, memory_order_seq_cst);
        patch->addPatchUnit(to->op, memory_order_seq_cst);

        // Find the synchronizable subpaths 
        edge_list_t *edges = path->edges;
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
                    imposeSynchronizableSubpath(beginIter, endIter, from,
                        toNode, edges, patch);
                    break;
                }
            } else { // This is either RW or WW
                // Found the from node, i.e. the curNode
                fromNode = curNode;
                // First fromNode has to be seq_cst 
                patch->addPatchUnit(fromNode->op, memory_order_seq_cst);
                if (fromNode != toNode) {
                    // More than one node in this "subpath"
                    endIter = curIter;
                    imposeSynchronizableSubpath(beginIter, endIter, fromNode,
                        toNode, edges, patch);
                }
                // Update the iterators and nodes for the next subpath
                curIter++;
                if (curIter == edges->end()) {
                    // The end of the search
                    break;
                }
                // Otherwise update the invariables
                beginIter = curIter;
                curNode = dest;
                toNode = dest;
                // Also make sure that the toNode is always checked
                patch->addPatchUnit(toNode->op, memory_order_seq_cst);
            }
        }

        // Add the patch to the list
        SCPatch::addPatchToList(patches, patch);
    }
    
    return patches;
}

/**
    Impose syncrhonizable subpath
    "begin" iterator points to the first edge that goes to the "to" node
    A subroutine of "imposeSynchronizablePath" and "imposeStrongPath"
*/
void SCGraph::imposeSynchronizableSubpath(edge_list_t::iterator begin, edge_list_t::iterator
    end, SCNode *from, SCNode *to, edge_list_t *edges, SCPatch *p) {
    
    DPRINT("Try to impose synchronization %d -> %d: ",
        from->op->get_seq_number(), to->op->get_seq_number());

    if (from->sbSynchronized(to)) {
        DPRINT("SB path\n");
        return;
    }

    // Try a simple release-sequence-type synchronization
    // Find a continuous rf chain
    bool rfStart = false;
    bool isRelSeq = true;
    const ModelAction *relHead = NULL, *relTail = NULL;
    SCNode *curNode = to;
    for (edge_list_t::iterator it = begin; it != end; it++) {
        SCEdge *e = *it;
        SCNode *n = e->node;
        bool isRF = e->type == RF || (curNode->op->is_read() &&
            curNode->op->get_reads_from() == n->op);
        if (isRF && !rfStart) {
            rfStart = true;
            relTail = curNode->op;
        }
        if (rfStart && !isRF) {
            relHead = curNode->op;
            isRelSeq = from->op->get_tid() == relHead->get_tid();
            break;
        }
        curNode = e->node;
    }

    if (relHead && relTail && isRelSeq) {
        p->addPatchUnit(relHead, memory_order_release);
        p->addPatchUnit(relTail, memory_order_acquire);
    } else {
        // Simply require every reads-from edge to be release/acquire
        curNode = to;
        for (edge_list_t::iterator it = begin; it != end; it++) {
            SCEdge *e = *it;
            if (e->type == RF) {
                const ModelAction *write = e->node->op;
                p->addPatchUnit(write, memory_order_release);
                p->addPatchUnit(curNode->op, memory_order_acquire);
            }
            curNode = e->node;
        }
    }
}


// Remove the read-from edge that the "read" reads from (simply set the edge
// type of the RF edge in the incoming edges of "read" to REMOVED, and returns
// that edge index
int SCGraph::removeRFEdge(SCNode *read) {
    ASSERT (read->op->is_read());
    EdgeList *incomingEdges = read->incoming;
    for (unsigned i = 0; i < incomingEdges->size(); i++) {
        SCEdge *existing  = (*incomingEdges)[i];
        if (existing->type == RF) {
            DPRINT("Take out the %d -RF->%d edge temporarily\n",
                existing->node->op->get_seq_number(),
                read->op->get_seq_number());
            existing->type = REMOVED;
            return i;
        }
    }

    // A read must have a store to read from
    ASSERT (false);
}


// Given a read node, and its RF edge index, reset that edge's type to RF
void SCGraph::addBackRFEdge(SCNode *read, int index) {
    ASSERT (read->op->is_read());
    SCEdge *e = (*read->incoming)[index];
    ASSERT (e->type == REMOVED);
    e->type = RF;
}


// Remove the incoming edge (from, to, type) in to's incoming edge list (by
// setting the edge type to REMOVED), and returns that edge index
int SCGraph::removeIncomingEdge(SCNode *from, SCNode *to, SCEdgeType type) {
    ASSERT (to->op->is_write() && (type == RW || type == WW));
    DPRINT("Take out the %d -%s-> %d edge temporarily\n", from->op->get_seq_number(),
        get_edge_str(type), to->op->get_seq_number());
    EdgeList *incomingEdges = to->incoming;
    for (unsigned i = 0; i < incomingEdges->size(); i++) {
        SCEdge *existing  = (*incomingEdges)[i];
        if (type == existing->type &&
            from == existing->node) {
            existing->type = REMOVED;
            return i;
        }
    }
    // We do not have such an edge
    return -1;
}


// Reset the type of the incoming edge (at index "index") type to "type"
void SCGraph::addBackIncomingEdge(SCNode *to, int index, SCEdgeType type) {
    ASSERT (to->op->is_write() && (type == RW || type == WW));
    if (index == -1)
        return;
    SCEdge *e = (*to->incoming)[index];
    ASSERT (e->type == REMOVED);
    e->type = type;
}


// Find all the synchronizable paths (sb U rf) iteratively; the main algorithm
// is the same is the findPaths() method except that it only looks
// for SB & RF edges.
path_list_t * SCGraph::findSynchronizablePaths(SCNode *from, SCNode *to) {

    DPRINT("****  Finding synchronizable path %d -> %d  ****\n",
        from->op->get_seq_number(), to->op->get_seq_number());
    // The main stack we use to maintain the choices and backtrack
    SnapList<EdgeChoice *> edgeStack; 
    // The list to store our paths
    path_list_t *result = new path_list_t;

    unsigned fromSeqNum = from->op->get_seq_number();
    // Initially push back the 0th edge of to's incoming edges
    edgeStack.push_back(new EdgeChoice(to, 0, 0));
    // The first edge must be either an SB or RF edge
    SCEdgeType t = (*to->incoming)[0]->type;
    ASSERT (t == SB || t == RF);
    VDPRINT("Push back: (%d, %d, %d)\n", to->op->get_seq_number(), 0, 0);

    // Main loop
    while (!edgeStack.empty()) {
        EdgeChoice *choice = edgeStack.back();
        SCNode *n = choice->n;
        EdgeList *incomingEdges = n->incoming;
        VDPRINT("Peek back: (%d, %d, %d)\n", n->op->get_seq_number(), choice->i,
            choice->finished);

        t = (*incomingEdges)[choice->i]->type;
        if (t == RW || t == WW || t == REMOVED) {
            // Backtrack since we only look for SB & RF edges
            // The current edge choice is done, backtrack
            VDPRINT("RW|WW edge backtrack: \n");
            // FIXME: We can actually give up very early here since all other
            // edges with bigger index should be RW|WW edges
            // Make sure this is true

            // Ready to pop out the choice
            edgeStack.pop_back();
            VDPRINT("Pop back: (%d, %d, %d)\n", n->op->get_seq_number(),
                choice->i, choice->finished);
            continue;
        }

        if (choice->finished == 0) { // The current choice of edge is done
            choice->finished = 1; // Don't need to pop the edge choice yet
            // Add the first incoming edge of "n"
            SCEdge *e = (*incomingEdges)[choice->i];
            SCNode *n0 = e->node;

            // This is also an ending condition, backtrack
            if (n0->incoming->empty()) {
                VDPRINT("Empty choice: (%d, %d, %d)\n", n->op->get_seq_number(),
                    choice->i, choice->finished);
                if (incomingEdges->size() != choice->i + 1) {
                    // We don't need to pop the choice yet, just update the choice
                    choice->finished = 0;
                    choice->i++;
                    VDPRINT("Advance choice: (%d, %d, %d)\n",
                        n->op->get_seq_number(), choice->i, choice->finished);
                } else {
                    // Ready to pop out the choice
                    edgeStack.pop_back();
                    VDPRINT("Pop back: (%d, %d, %d)\n", n->op->get_seq_number(),
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
                VDPRINT("Add edge: ");
                for (SnapList<EdgeChoice *>::reverse_iterator iter =
                    edgeStack.rbegin(); iter != edgeStack.rend();
                    iter++) {
                    EdgeChoice *c = *iter;
                    SCEdge *e0 = (*c->n->incoming)[c->i];
                    VDPRINT("%d -> ", e0->node->op->get_seq_number());
                    p->addEdgeFromFront(e0);
                }
                VDPRINT("%d\n", to->op->get_seq_number());
                // Add the path to the result list
                result->push_back(p);
                // We check every path right after we found it --- good for
                // early return
                VDB(
                    VDPRINT("Found path:");
                    p->printWithOrder(to);
                )
                // FIXME: Looking for all the sbUrf paths can be very costly
                // Maybe we can use some tricks to exit early

            } else if (from->earlier(n0)) {
                // Haven't found the "from" node yet, just keep pushing to
                // the stack
                edgeStack.push_back(new EdgeChoice(n0, 0, 0));
                VDPRINT("Push back: (%d, %d, %d)\n", n0->op->get_seq_number(), 0,
                    0);
            }
        } else {
            // The current edge choice is done, backtrack
            VDPRINT("Finished backtrack: \n");
            if (incomingEdges->size() != choice->i + 1) {
                // We don't need to pop the choice yet, just update the choice
                choice->finished = 0;
                choice->i++;
                VDPRINT("Advance choice: (%d, %d, %d)\n",
                    n->op->get_seq_number(), choice->i, choice->finished);
            } else {
                // Ready to pop out the choice
                edgeStack.pop_back();
                VDPRINT("Pop back: (%d, %d, %d)\n", n->op->get_seq_number(),
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
            p->printWithOrder(to);
        }
        if (result->empty()) {
            DPRINT("****  NO syncrhonizable path: %d -> %d  ****\n",
                from->op->get_seq_number(), to->op->get_seq_number());
        }
    )

    return result;
}


// Find all the paths iteratively; we basically use a depth-first search
// backwards from the node "to". We end a single search whenever we either: 1)
// reach the node "from" --- we find a path; or 2) we reach a node whose
// sequence number is smaller than that of node "from" --- since in an SC trace
// there can't be an edge from backwards.
path_list_t * SCGraph::findPaths(SCNode *from, SCNode *to) {

    DPRINT("****  Finding path %d -> %d  ****\n", from->op->get_seq_number(),
        to->op->get_seq_number());
    // The main stack we use to maintain the choices and backtrack
    SnapList<EdgeChoice *> edgeStack; 
    // The list to store our paths
    path_list_t *result = new path_list_t;

    unsigned fromSeqNum = from->op->get_seq_number();
    // Initially push back the 0th edge of to's incoming edges
    edgeStack.push_back(new EdgeChoice(to, 0, 0));
    VDPRINT("Push back: (%d, %d, %d)\n", to->op->get_seq_number(), 0, 0);

    // Main loop
    while (!edgeStack.empty()) {
        EdgeChoice *choice = edgeStack.back();
        SCNode *n = choice->n;
        EdgeList *incomingEdges = n->incoming;
        VDPRINT("Peek back: (%d, %d, %d)\n", n->op->get_seq_number(), choice->i,
            choice->finished);

        if (choice->finished == 0) { // The current choice of edge is done
            choice->finished = 1; // Don't need to pop the edge choice yet
            // Add the first incoming edge of "n"
            SCEdge *e = (*incomingEdges)[choice->i];
            SCNode *n0 = e->node;

            // This is also an ending condition, backtrack
            if (n0->incoming->empty() || e->type == REMOVED) {
                if (n0->incoming->empty()) {
                    VDPRINT("Empty choice: (%d, %d, %d)\n",
                        n->op->get_seq_number(), choice->i, choice->finished);
                } else {
                    VDPRINT("Removed edge: (%d, %d, %d)\n",
                        n->op->get_seq_number(), choice->i, choice->finished);
                }
                if (incomingEdges->size() != choice->i + 1) {
                    // We don't need to pop the choice yet, just update the choice
                    choice->finished = 0;
                    choice->i++;
                    VDPRINT("Advance choice: (%d, %d, %d)\n",
                        n->op->get_seq_number(), choice->i, choice->finished);
                } else {
                    // Ready to pop out the choice
                    edgeStack.pop_back();
                    VDPRINT("Pop back: (%d, %d, %d)\n", n->op->get_seq_number(),
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
                VDPRINT("Add edge: ");
                for (SnapList<EdgeChoice *>::reverse_iterator iter =
                    edgeStack.rbegin(); iter != edgeStack.rend();
                    iter++) {
                    EdgeChoice *c = *iter;
                    SCEdge *e0 = (*c->n->incoming)[c->i];
                    VDPRINT("%d -> ", e0->node->op->get_seq_number());
                    p->addEdgeFromFront(e0);
                }
                VDPRINT("%d\n", to->op->get_seq_number());
                // Add the path to the result list
                result->push_back(p);
                VDPRINT("Found path:");
                VDB( p->printWithOrder(to); )
            } else if (from->earlier(n0)) {
                // Haven't found the "from" node yet, just keep pushing to
                // the stack
                edgeStack.push_back(new EdgeChoice(n0, 0, 0));
                VDPRINT("Push back: (%d, %d, %d)\n", n0->op->get_seq_number(), 0,
                    0);
            }
        } else {
            // The current edge choice is done, backtrack
            VDPRINT("Finished backtrack: \n");
            if (incomingEdges->size() != choice->i + 1) {
                // We don't need to pop the choice yet, just update the choice
                choice->finished = 0;
                choice->i++;
                VDPRINT("Advance choice: (%d, %d, %d)\n",
                    n->op->get_seq_number(), choice->i, choice->finished);
            } else {
                // Ready to pop out the choice
                edgeStack.pop_back();
                VDPRINT("Pop back: (%d, %d, %d)\n", n->op->get_seq_number(),
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
            p->printWithOrder(to);
        }
        if (result->empty()) {
            DPRINT("****  NO SC path: %d -> %d  ****\n",
                from->op->get_seq_number(), to->op->get_seq_number());
        }
    )

    return result;
}

void SCGraph::buildGraph() {
    DB(
        model_print("****  Building Graph %d  ****\n",
            execution->get_execution_number());
    )

    buildVectors();
    computeCV();
    computeSBCV();
    
    DB(
        print();
        //printToFile();
        printInfoPerLoc();
    )

    checkStrongSC();
}

// A wrapper that makes sure the same edge is only added once
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



// **** We steal most of the following from the SC analysis ****

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
