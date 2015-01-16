#include "specanalysis.h"
#include "action.h"
#include "cyclegraph.h"
#include "threads-model.h"
#include "clockvector.h"
#include "execution.h"
#include <sys/time.h>
#include <assert.h>
#include "modeltypes.h"
#include "model.h"

extern ModelChecker *model;

/** Add ordering points **/

SPECAnalysis::SPECAnalysis()
{
	execution = NULL;
	func_table = NULL;
	hb_rules = new hbrule_list_t();
	isBrokenExecution = false;
	hbEdgeMode = false;
	nonStopMode = false;
	verbose = false;

	cyclicGraph = false;
}

SPECAnalysis::~SPECAnalysis() {
}

void SPECAnalysis::setExecution(ModelExecution * execution) {
	this->execution = execution;
}

const char * SPECAnalysis::name() {
	const char * name = "SPEC";
	return name;
}

void SPECAnalysis::finish() {
	freeCPNodes();
	model_print("SPECAnalysis finished!\n");
}

bool SPECAnalysis::option(char * opt) {
	if (strcmp(opt, "hb-mode") == 0) {
		hbEdgeMode = true;
		return false;
	} else if (strcmp(opt, "verbose") == 0) {
		verbose = true;
		return false;
	} else if (strcmp(opt, "non-stop") == 0) {
		nonStopMode = true;
		return false;
	}
	model_print("SPEC Analysis options\n");
	model_print("non-hb-- Don't take happens-before edges to order API methods\n");
	model_print("non-stop-- Don't stop the checking process even if it\
encounters an inconsistency\n");
	model_print("verbose -- Output the graph information\n");
	return true;
}

void SPECAnalysis::analyze(action_list_t *actions) {
	//if (trace_num_cnt % 1000 == 0)
		//model_print("SPECAnalysis analyzing: %d!\n", trace_num_cnt);
	trace_num_cnt++;
	// Ever before running any analysis, check if the execution is buggy
	if (execution->have_bug_reports()) {
		model_print("No need to run SPEC checker if there's CDSChecker bug.\n");
		execution->print_summary();
		return;
	}

	//traverseActions(actions);
	
	buildCPGraph(actions);
	if (isBrokenExecution)
		return;
	if (func_table == NULL) {
		model_print("Missing the entry point annotation!\n");
		return;
	}

	if (cpActions->size() == 0) return;
	buildEdges();
	
	node_list_t *sorted_commit_points = sortCPGraph(actions);

	
	if (sorted_commit_points == NULL) {
		model_print("Wired data structure, fail to check!\n");
		dumpGraph(sorted_commit_points);
		model->print_execution(true);
		return;
	}
	bool passed = check(sorted_commit_points);

	if (!passed || verbose) {
		if (!passed)
			model_print("Error exists!!\n");
		dumpGraph(sorted_commit_points);
		model->print_execution(true);
	}
}

bool SPECAnalysis::check(node_list_t *sorted_commit_points) {
	bool passed = true;
	// Actions and simple checks first
	node_list_t::iterator iter;
	//model_print("Sorted nodes:\n");
	for (iter = sorted_commit_points->begin(); iter !=
		sorted_commit_points->end(); iter++) {
		commit_point_node *node = *iter;
		//dumpNode(node);
		int interface_num = node->interface_num;
		const char *interface_name = node->interface_name;
		id_func_t id_func = (id_func_t) func_table[2 * interface_num];
		check_action_func_t check_action = (check_action_func_t) func_table[2 * interface_num + 1];
		void *info = node->info;
		///model_print("Here1, inter_name: %d\n", interface_num);
		thread_id_t __TID__ = node->begin->get_tid();
		
		//model_print("hey_id1\n");
		call_id_t __ID__ = id_func(info, __TID__);
		//model_print("hey_id2\n");
		node->__ID__ = __ID__;
		//model_print("hey_check1\n");
		passed = check_action(info, __ID__, __TID__);
		//model_print("hey_check2\n");
		//model_print("hey!\n");
		if (!passed) {
			model_print("Interface %s failed\n", interface_name);
			model_print("ID: %d\n", __ID__);
			model_print("Error exists in correctness check!!\n");
			// If it's not at the non-stop mode, it will stop checking 
			if (!nonStopMode)
				return false;
		}
		//model_print("%d interface call passed\n", interface_num);
	}

	// Happens-before checks
	if (hb_rules->size() == 0) { // No need to check
		model_print("no hb_rules!\n");
		return passed;
	} else {
		//model_print("hb_rules empty!\n");
	}
	node_list_t::iterator next_iter;
	// Iterate commit point nodes
	for (iter = sorted_commit_points->begin(); iter !=
		sorted_commit_points->end(); iter++) {
		commit_point_node *n1 = *iter;
		if (n1->hb_conds == NULL) continue; // No happens-before check
		// Iterate HB conditions of n1
		for (hbcond_list_t::iterator cond_iter1 = n1->hb_conds->begin();
			cond_iter1 != n1->hb_conds->end(); cond_iter1++) {
			anno_hb_condition *cond1 = *cond_iter1;
			// Find a rule that matches n1
			for (hbrule_list_t::iterator rule_iter = hb_rules->begin();
				rule_iter != hb_rules->end(); rule_iter++) {
				anno_hb_init *rule = *rule_iter;
				if (rule->interface_num_before != cond1->interface_num
					|| rule->hb_condition_num_before != cond1->hb_condition_num) {
					continue;
				}
				// Iterate the later nodes n2
				for (next_iter = iter, next_iter++; next_iter !=
					sorted_commit_points->end(); next_iter++) {
					commit_point_node *n2 = *next_iter;
					if (n1->__ID__ == n2->__ID__) {
						if (n2->hb_conds == NULL) continue; // No hb conditions
						// Find a later node that matches the rule
						for (hbcond_list_t::iterator cond_iter2 = n2->hb_conds->begin();
							cond_iter2 != n2->hb_conds->end(); cond_iter2++) {
							anno_hb_condition *cond2 = *cond_iter2;
							if (rule->interface_num_after !=
								cond2->interface_num ||
								rule->hb_condition_num_after !=
								cond2->hb_condition_num) {
								continue;
							}
							// Check hb here
							if (!n1->begin->happens_before(n2->end)) {
								model_print("Error exists in hb check!!\n");
								dumpNode(n1);
								dumpNode(n2);
								model_print("HB violation ends!!\n");
								return false;
							} else {
								//model_print("hey passed one HB check!\n");
							}
						}
					}
				}
			}
		}
	}
	return passed;
}


/**
	This function is called with sortCPGraph to check if this is a cyclic
	execution graph. We use DFS to traverse the graph and then check if there's
	any back edges to judge if it's cyclic.
*/
bool SPECAnalysis::isCyclic() {
	return false;
}


static void dumpNodeUtil(commit_point_node *node, const char *msg) {
	ordering_act_list_t *operations = node->operations;
	model_print("Node: seq_%d, tid_%d, %s", node->begin->get_seq_number(),
		node->begin->get_tid(), node->interface_name);
	for (ordering_act_list_t::iterator opIter = operations->begin(); opIter !=
		operations->end(); opIter++) {
		ordering_operation *ordering_act = *opIter;
		ModelAction *act = ordering_act->operation;
		model_print("__%d", act->get_seq_number());
	}
	if (msg == NULL) {
		model_print("\n");
	} else {
		model_print(", %s\n", msg);
	}
}

/**
	Topologically sort the commit points graph; If they are not restricted to
	any of the HB, MO & RF rules, they should keep their trace order;
	Here are some rules:
	1. If there's multiple outgoing reads-from edges, read nodes are
	prioritized;
	2. 
*/
node_list_t* SPECAnalysis::sortCPGraph(action_list_t *actions) {
	node_list_t *sorted_list = new node_list_t();
	node_list_t *stack = new node_list_t();
	int time_stamp = 1;
	commit_point_node *node;
	// Start from the end of the trace to keep their trace order
	action_list_t::reverse_iterator iter = cpActions->rbegin();
	node = cpGraph->get(*iter);
	stack->push_back(node);
	// color: 0 -> undiscovered; 1 -> discovered; 2 -> visited
	for (; iter != cpActions->rend(); iter++) {
		node = cpGraph->get(*iter);
		if (node->color == 0) { // Not discovered yet
			stack->push_back(node);
		}
		while (stack->size() > 0) {
			node = stack->back();
			stack->pop_back();
			if (node->color == 0) { // Just discover this node before
				node->color = 1;
				//dumpNodeUtil(node, "Out of stack discovered");
				stack->push_back(node);
				if (node->edges != NULL) { // Push back undiscovered nodes
					// Cautious!! Push HB & MO edges first then RF
					for (edge_list_t::reverse_iterator rit =
						node->edges->rbegin(); rit != node->edges->rend(); rit++) {
						commit_point_node *next_node = (*rit)->next_node;
						if (next_node->color == 1) { // back edge -> cycle
							dumpNodeUtil(next_node, "cycle");
							traverseActions(actions);
							model_print("There exists cycles in this graph!\n");
							cyclicGraph = true;
							return NULL;
						}
						if (next_node->color == 0) {
							stack->push_back(next_node);
							//dumpNodeUtil(next_node, "Discovered");
						} else {
							//dumpNodeUtil(next_node, "Visited");
						}
					}
				}
			} else if (node->color == 1) { // Discovered and ready to visit
				node->color = 2;
				node->finish_time_stamp = time_stamp++;
				//dumpNode(node);
				sorted_list->push_front(node);
			}
		}
	}
	return sorted_list;
}

/**
	Build an edge between the two nodes with respect to the operations. When the
	two operations are on the same location, if an edge has been established, it
	returns 1; else it returns 0. When the two operations are on different
	locations, it returns -1. If in hb-mode and the two operations are ordered
	by hb, it returns 2.
*/
int SPECAnalysis::buildEdge(CycleGraph *mo_graph, commit_point_node *node1, commit_point_node *node2,
	ModelAction *act1, ModelAction *act2) {
	const ModelAction *rfAct1 = act1->get_reads_from(),
		*rfAct2 = act2->get_reads_from();
	int res = -1;
	
	if (hbEdgeMode) {
		if (act1->happens_before(act2)) {
			node1->addEdge(node2, HB);
		} else if (act2->happens_before(act1)) {
			node2->addEdge(node1, HB);
		} else {
			res = -1;
		}

		res = 2;
	}

	if (act1->get_location() == act2->get_location()) { // Same location 
		if (act1->is_read() && rfAct1 == act2) {
			node2->addEdge(node1, RF);
			if (verbose) {
				model_print("RF:\n");
				act2->print();
				act1->print();
			}
			return 1;
		} else if (act2->is_read() && rfAct2 == act1) {
			node1->addEdge(node2, RF);
			if (verbose) {
				model_print("RF:\n");
				act1->print();
				act2->print();
			}
			return 1;
		} else {
			// Compare the two actions by MO order
			if (act1->is_write())
			rfAct1 = act1;
			if (act2->is_write())
				rfAct2 = act2;

			if (rfAct1 == rfAct2) { // Prioritize reads
				// When both are reads, let it alone
				if (rfAct1->is_write() || rfAct2->is_write()) {
					if (!rfAct1->is_write()) {
						node1->addEdge(node2, MO);
						if (verbose) {
							model_print("read1 write2:\n");
							act1->print();
							act2->print();
						}
						return 1;
					} else if (!rfAct2->is_write()) {
						node2->addEdge(node1, MO);
						if (verbose) {
							model_print("write1 read2:\n");
							act2->print();
							act1->print();
						}
						return 1;
					}
					if (verbose)
						model_print("MO both reads no edges.\n");
					return 0;
				}
			} else {
				if (mo_graph->checkReachable(rfAct1, rfAct2)) {
					node1->addEdge(node2, MO);
					if (verbose) {
						model_print("normal MO:\n");
						act1->print();
						act2->print();
					}
					return 1;
				} else if (mo_graph->checkReachable(rfAct2, rfAct1)) {
					node2->addEdge(node1, MO);
					if (verbose) {
						model_print("normal MO:\n");
						act2->print();
						act1->print();
					}
					return 1;
				}
			}
		}
	}
	
	return res;
}

/**
	This method iterates the pair of commit points between two nodes to build
	edges. In no-hb-edge mode, it does NOT add happens-before edges between
	actions (nodes).
*/
void SPECAnalysis::buildEdges() {
	//model_print("Building edges...\n");
	action_list_t::iterator iter1, iter2;
	CycleGraph *mo_graph = execution->get_mo_graph();
	for (iter1 = cpActions->begin(); iter1 != cpActions->end(); iter1++) {
		for (iter2 = iter1, iter2++; iter2 != cpActions->end(); iter2++) {
			ModelAction *begin1 = *iter1,
				*begin2 = *iter2,
				*act1, *act2;
			ordering_operation *ordering_act1, *ordering_act2;
			commit_point_node *node1 = cpGraph->get(begin1),
				*node2 = cpGraph->get(begin2);
			// Iterate the commit point lists of both nodes to build edges
			ordering_act_list_t::iterator cpIter1, cpIter2;
			int res = -1;
			bool sameLoc = false;
			bool hasEdge = false;
			// Omit additional ordering points at first 
			for (cpIter1 = node1->operations->begin(); cpIter1 !=
				node1->operations->end(); cpIter1++) {
				ordering_act1 = *cpIter1;
				if (ordering_act1->is_additional_point)
					continue;
				act1 = ordering_act1->operation;
				for (cpIter2 = node2->operations->begin(); cpIter2 !=
					node2->operations->end(); cpIter2++) {
					ordering_act2 = *cpIter2;
					if (ordering_act2->is_additional_point)
						continue;
					act2 = ordering_act2->operation;
					res = buildEdge(mo_graph, node1, node2, act1, act2);
					if (res > 0)
						hasEdge = true;
					if (res == 0 || res == 1)
						sameLoc = true;
				}
			}

			// This time we count on additional ordering points to add edges 
			if (sameLoc && !hasEdge) {
				for (cpIter1 = node1->operations->begin(); cpIter1 !=
					node1->operations->end(); cpIter1++) {
					ordering_act1 = *cpIter1;
					act1 = ordering_act1->operation;
					for (cpIter2 = node2->operations->begin(); cpIter2 !=
						node2->operations->end(); cpIter2++) {
						ordering_act2 = *cpIter2;
						act2 = ordering_act2->operation;
						res = buildEdge(mo_graph, node1, node2, act1, act2);
						if (res > 0)
							hasEdge = true;
						if (res == 0 || res == 1)
							sameLoc = true;
					}
				}
			}
		}
	}
	//model_print("Finish building edges!\n");
}

bool SPECAnalysis::hasAnEdge(const ModelAction *act1, const ModelAction *act2) {
	if (act2->get_reads_from() == act1)
		return true;
	if (act1->happens_before(act2))
		return true;
	CycleGraph *mo_graph = execution->get_mo_graph();
	if (mo_graph->checkReachable(act1, act2))
		return true;
	return false;
}

// Given the current iterator and the annotation, return the latest operation in
// the trace of that thread
ModelAction* SPECAnalysis::getPrevAction(action_list_t *actions,
	action_list_t::iterator *iter, ModelAction *anno) {
	action_list_t::iterator it = *iter;
	int cnt = 0;
	ModelAction *act;
	while (true) {
		it--;
		cnt++;
		act = *it;
		if (anno->get_tid() == act->get_tid()
			&& act->get_type() != ATOMIC_ANNOTATION) {
			for (int i = 0; i < cnt; i++)
				it++;
			return act;
		}
	}
}

/**
	Rule of getting the list of ordering points:
	If there are a sequence of potential_commit_points, and a
	commit_point_define, then the last closest potential_commit_point is an
	ordering point;
	If there is a commit point define check, then the closest atomic operation
	is also an ordering point;
	If we encounter a Clear_Ordering_Point annotation, we drop the previous list
	of commit points (we don't have that annotation yet)
*/
commit_point_node* SPECAnalysis::getCPNode(action_list_t *actions, action_list_t::iterator
	*iter) {
	action_list_t::iterator it = *iter;
	ModelAction *act = *it;
	thread_id_t tid = act->get_tid();
	commit_point_node *node = new commit_point_node();
	//Initialize the commit_point_node
	spec_annotation *anno = (spec_annotation*) act->get_location();
	assert(anno->type == INTERFACE_BEGIN);
	anno_interface_begin *begin_anno = (anno_interface_begin*) anno->annotation;
	anno_interface_end* end_anno;
	int interface_num = begin_anno->interface_num;
	node->interface_num = interface_num;
	node->interface_name = begin_anno->interface_name;
	node->begin = act;
	// Don't forget the color
	node->color = 0;

	anno_cp_define *cp_define;
	anno_potential_cp_define *pcp_define;
	anno_cp_define_check *cp_define_check;
	anno_cp_clear *cp_clear;
	// A list of potential commit points
	pcp_list_t *pcp_list = new pcp_list_t();
	potential_cp_info *pcp_info;
	ModelAction *operation = NULL;
	bool hasCommitPoint = false, pcp_defined = false;
	bool hasCorrespondingPCP = false;
	node->operations = new ordering_act_list_t();
	for (it++; it != actions->end(); it++) {
		act = *it;
		if (act->get_type() != ATOMIC_ANNOTATION || act->get_tid() != tid
			|| act->get_value() != SPEC_ANALYSIS)
			continue;
		spec_annotation *anno = (spec_annotation*) act->get_location();
		anno_hb_condition *hb_cond;
		ordering_operation *order_op;
		switch (anno->type) {
			case POTENTIAL_CP_DEFINE:
				//model_print("POTENTIAL_CP_DEFINE\n");
				// Make a faster check
				pcp_defined = true;
				pcp_define = (anno_potential_cp_define*) anno->annotation;
				pcp_info = new potential_cp_info();
				pcp_info->label_num = pcp_define->label_num;
				pcp_info->operation = getPrevAction(actions, &it, act);
				pcp_list->push_back(pcp_info);
				break;
			case CP_DEFINE:
				//model_print("CP_DEFINE\n");
				cp_define = (anno_cp_define*) anno->annotation;
				if (node->interface_num != cp_define->interface_num)
					continue;
				if (!pcp_defined) {
					// Fast check 
					break;
				} else {
					// Check if potential commit has been checked
					hasCorrespondingPCP = false;
					for (pcp_list_t::iterator pcp_it = pcp_list->begin(); pcp_it
						!= pcp_list->end(); pcp_it++) {
						pcp_info = *pcp_it;
						if (cp_define->potential_cp_label_num == pcp_info->label_num) {
							hasCommitPoint = true;
							hasCorrespondingPCP = true;
							operation = pcp_info->operation;
						}
					}
					if (hasCorrespondingPCP) {
						order_op = new ordering_operation;
						order_op->is_additional_point = cp_define->is_additional_point;
						order_op->operation = operation;
						node->operations->push_back(order_op);
						// Destroy the previous list of potential commit points
						deletePcpList(pcp_list);
						// Get a new list of potential commit points
						pcp_list = new pcp_list_t();
					}
				}
				break;
			case CP_DEFINE_CHECK:
				//model_print("CP_DEFINE_CHECK\n");
				cp_define_check = (anno_cp_define_check*) anno->annotation;
				if (node->interface_num != cp_define_check->interface_num)
					continue;
				operation = getPrevAction(actions, &it, act);
				// Add the commit_point_define_check operation
				order_op = new ordering_operation;
				order_op->is_additional_point = cp_define_check->is_additional_point;
				order_op->operation = operation;
				node->operations->push_back(order_op);
				hasCommitPoint = true;
				break;
			
			case CP_CLEAR:
				//model_print("CP_CLEAR\n");
				cp_clear = (anno_cp_clear*) anno->annotation;
				//if (node->interface_num != cp_clear->interface_num)
					//continue;
				hasCorrespondingPCP = false;
				hasCommitPoint = false;
				// Destroy the previous list of potential commit points
				//deletePcpList(pcp_list);
				// Get a new list of potential commit points
				//pcp_list = new pcp_list_t();
				// Also destroy the previous list of commit points
				delete node->operations;
				node->operations = new ordering_act_list_t();
				break;

			case HB_CONDITION:
				//model_print("HB_CONDITION\n");
				hb_cond = (anno_hb_condition*) anno->annotation;
				if (hb_cond->interface_num != interface_num) {
					model_print("Bad interleaving of the same thread.\n");
					return NULL;
				}
				if (node->hb_conds == NULL) {
					node->hb_conds = new hbcond_list_t();	
				}
				node->hb_conds->push_back(hb_cond);
				break;
			case INTERFACE_END:
				//model_print("INTERFACE_END\n");
				end_anno = (anno_interface_end*) anno->annotation;
				if (!hasCommitPoint) {
					model_print("Exception: %d, tid_%d\tinterface %d without commit points!\n",
						act->get_seq_number(),
						node->begin->get_tid(), node->interface_num);
					traverseActions(actions);
					return NULL;
				}
				node->end = act;
				node->info = end_anno->info;
				return node;
			default:
				break;
		}
	}
	deletePcpList(pcp_list);
	return node;
}


// Free the potential commit points list
void SPECAnalysis::deletePcpList(pcp_list_t *pcp_list) {
	for (pcp_list_t::iterator free_it = pcp_list->begin(); free_it !=
		pcp_list->end(); free_it++) {
		delete *free_it;
	}
	delete pcp_list;
}


void SPECAnalysis::buildCPGraph(action_list_t *actions) {
	cpGraph = new node_table_t();
	cpActions = new action_list_t();
	action_list_t::iterator begin_anno_iter = actions->begin(), iter;
	for (; begin_anno_iter != actions->end(); begin_anno_iter++) {
		ModelAction *act = *begin_anno_iter;
		if (act->get_type() != ATOMIC_ANNOTATION)
			continue;
		if (act->get_value() != SPEC_ANALYSIS)
			continue;
		spec_annotation *anno = (spec_annotation*) act->get_location();
		if (anno == NULL) {
			model_print("Wrong annotation!\n");
			return;
		}
		anno_hb_init *hb_rule = NULL;
		commit_point_node *node;
		anno_init *init;

		anno_interface_begin *begin_anno;
		int interface_num;
		switch (anno->type) {
			case INIT:
				init = (anno_init*) anno->annotation;
				//model_print("INIT. func_table: %d, size: %d\n",
					//init->func_table, init->func_table_size);
				if (func_table != NULL) {
					//break;
				}
				func_table = init->func_table;
				for (int i = 0; i < init->hb_init_table_size; i++) {
					hb_rule = init->hb_init_table[i];
					hb_rules->push_back(hb_rule);
				}
				//model_print("hb_rules size: %d!\n", hb_rules->size());
				break;
			case INTERFACE_BEGIN:
				//model_print("INTERFACE_BEGIN\n");
				iter = begin_anno_iter;
				begin_anno = (anno_interface_begin*) anno->annotation;
				interface_num = begin_anno->interface_num;
				node = getCPNode(actions, &iter);
				if (node != NULL) {
					// Store the graph in a hashtable
					cpGraph->put(node->begin, node);
					cpActions->push_back(node->begin);
				}
				break;
			default:
				break;
		}
	}
}

void SPECAnalysis::freeCPNodes() {
}

void SPECAnalysis::dumpNode(commit_point_node *node) {
	/*
	action_list_t *operations = node->operations;
	model_print("Node: seq_%d, tid_%d, inter_%d", node->begin->get_seq_number(),
		node->begin->get_tid(), node->interface_num);
	for (action_list_t::iterator opIter = operations->begin(); opIter !=
		operations->end(); opIter++) {
		ModelAction *act = *opIter;
		model_print("__%d", act->get_seq_number());
	}
	model_print("\n");
	*/
	dumpNodeUtil(node, NULL);
	if (node->edges == NULL) return;
	for (edge_list_t::iterator it = node->edges->begin();
		it != node->edges->end(); it++) {
		commit_point_node *next_node = (*it)->next_node;
		cp_edge_type type = (*it)->type;
		const char *relationMsg = type == HB ? "HB" : type == RF ?
			"RF" : type == MO ? "MO" : "RBW";
		model_print("Edge: seq_%d, %s, %s\n", next_node->begin->get_seq_number(),
			next_node->interface_name, relationMsg);
	}
}

/*
void SPECAnalysis::dumpActions(action_list_t actions) {
	action_list_t::iterator iter = actions->begin();
	for (; iter != actions->end(); iter++) {
		ModelAction *act = *iter;
		if (act->get_type() == ATOMIC_ANNOTATION) {

		}
	}
}*/

void SPECAnalysis::dumpDotGraph() {
	/*
	model_print("#---------- Dump Dot Graph Begin ----------\n");
	model_print("digraph {\n");
	action_list_t::iterator iter = cpActions->begin();
	for (; iter != cpActions->end(); iter++) {
		ModelAction *act = *iter;
		commit_point_node *node = cpGraph->get(act);

		model_print("\tn%d[label=\"%d_%d\"];\n", act->get_seq_number(),
			act->get_seq_number(), node->interface_num);
		if (node->edges == NULL) return;
		for (edge_list_t::iterator it = node->edges->begin();
			it != node->edges->end(); it++) {
			commit_point_node *next_node = (*it)->next_node;
			cp_edge_type type = (*it)->type;
			const char *relationMsg = type == HB ? "HB" : type == RF ?
				"RF" : type == MO ? "MO" : "RBW";
			model_print("\tn%d->n%d[label=\"%s\"];\n", act->get_seq_number(),
				next_node->operation->get_seq_number(), relationMsg);
		}
	}
	model_print("}\n");
	model_print("---------- Dump Dot Graph End ----------\n");
	*/
}

void SPECAnalysis::dumpGraph(node_list_t *sorted_commit_points) {
	model_print("---------- Dump Graph Begin ----------\n");
	action_list_t::iterator iter = cpActions->begin();
	for (; iter != cpActions->end(); iter++) {
		ModelAction *act = *iter;
		commit_point_node *node = cpGraph->get(act);
		dumpNode(node);
	}

	dumpDotGraph();

	model_print("---------- Sorted Nodes --------------\n");
	if (sorted_commit_points == NULL) {
		return;
	}
	node_list_t::iterator nodeIter;
	for (nodeIter = sorted_commit_points->begin(); nodeIter !=
		sorted_commit_points->end(); nodeIter++) {
		commit_point_node *n = *nodeIter;
		dumpNode(n);
	}
	model_print("---------- Dump Graph End ----------\n");
}

void SPECAnalysis::traverseActions(action_list_t *actions) {
	action_list_t::iterator iter = actions->begin();
	for (; iter != actions->end(); iter++) {
		ModelAction *act = *iter;
		if (act->get_type() != ATOMIC_ANNOTATION)
			continue;
		//model_print("FUNC_TABLE_INIT%d\n", act->get_value());
		if (act->get_value() != SPEC_ANALYSIS)
			continue;
		spec_annotation *anno = (spec_annotation*) act->get_location();
		if (anno == NULL) {
			model_print("Wrong annotation!\n");
			return;
		}
		anno_hb_init *hb_rule = NULL;
		anno_hb_condition *hb_condition = NULL;
		commit_point_node *node;
		anno_init *init;

		anno_interface_begin *begin_anno;
		anno_interface_end *end_anno;
		anno_cp_define_check *cp_define_check;
		anno_cp_clear *cp_clear;
		anno_potential_cp_define *pcp_define;
		anno_cp_define *cp_define;
		int interface_num;
		switch (anno->type) {
			case INIT:
				model_print("INIT\n");
				//init = (anno_init*) anno->annotation;
				break;
			case INTERFACE_BEGIN:
				begin_anno = (anno_interface_begin*) anno->annotation;
				model_print("seq_%d, tid_%d:\tINTERFACE_BEGIN \tinter_name: %s\n",
					act->get_seq_number(),
					act->get_tid(), begin_anno->interface_name);
				interface_num = begin_anno->interface_num;
				break;
			case POTENTIAL_CP_DEFINE:
				pcp_define = (anno_potential_cp_define*) anno->annotation;
				model_print("seq_%d, tid_%d:\tPOTENTIAL_CP_DEFINE \tlabel: %s\n",
					act->get_seq_number(),
					act->get_tid(), pcp_define->label_name);
				break;
			case CP_DEFINE:
				cp_define = (anno_cp_define*) anno->annotation;
				model_print("seq_%d, tid_%d:\tCP_DEFINE \tpotential_label: %s, label: %s\n",
					act->get_seq_number(),
					act->get_tid(), cp_define->potential_label_name,
						cp_define->label_name);
				break;
			case CP_DEFINE_CHECK:
				cp_define_check = (anno_cp_define_check*) anno->annotation;
				model_print("seq_%d, tid_%d\tCP_DEFINE_CHECK \tlabel_name: %s\n",
					act->get_seq_number(),
					act->get_tid(), cp_define_check->label_name);
				break;
			case CP_CLEAR:
				cp_clear = (anno_cp_clear*) anno->annotation;
				model_print("seq_%d, tid_%d\tCP_CLEAR:\n",
					act->get_seq_number(),
					act->get_tid());
				break;

			case INTERFACE_END:
				end_anno = (anno_interface_end*) anno->annotation;
				model_print("seq_%d, tid_%d\tINTERFACE_END \tinter_num: %d\n",
					act->get_seq_number(),
					act->get_tid(), end_anno->interface_num);
				break;
			case HB_CONDITION:
				hb_condition = (anno_hb_condition*) anno->annotation;
				model_print("seq_%d, tid_%d\tHB_CONDITION \tinter_num: %d; hb_cond_num: %d\n",
					act->get_seq_number(),
					act->get_tid(), hb_condition->interface_num,
					hb_condition->hb_condition_num);
				break;
			default:
				break;
		}
	}
}

