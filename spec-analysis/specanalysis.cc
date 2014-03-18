#include "specanalysis.h"
#include "action.h"
#include "cyclegraph.h"
#include "threads-model.h"
#include "clockvector.h"
#include "execution.h"
#include <sys/time.h>
#include <assert.h>
#include "modeltypes.h"


SPECAnalysis::SPECAnalysis()
{
	execution = NULL;
	func_table = NULL;
	hb_rules = new hbrule_list_t();
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
	return true;
}

void SPECAnalysis::analyze(action_list_t *actions) {
	if (trace_num_cnt % 1000 == 0)
		model_print("SPECAnalysis analyzing: %d!\n", trace_num_cnt);
	trace_num_cnt++;
	//traverseActions(actions);
	
	buildCPGraph(actions);
	if (func_table == NULL) {
		model_print("Missing the entry point annotation!\n");
		return;
	}

	if (cpActions->size() == 0) return;
	buildEdges();
	
	node_list_t *sorted_commit_points = sortCPGraph();
	if (sorted_commit_points == NULL) {
		model_print("Wired data structure, fail to check!\n");
		return;
	}
	//dumpGraph();
	bool passed = check(sorted_commit_points);
	if (!passed) {
		model_print("Error exists!!\n");
	} else {
		//model_print("Passed all the safety checks!\n");
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
		id_func_t id_func = (id_func_t) func_table[2 * interface_num];
		check_action_func_t check_action = (check_action_func_t) func_table[2 * interface_num + 1];
		void *info = node->info;
		///model_print("Here1, inter_name: %d\n", interface_num);
		thread_id_t __TID__ = node->operation->get_tid();
		call_id_t __ID__ = id_func(info, __TID__);
		node->__ID__ = __ID__;
		passed = check_action(info, __ID__, __TID__);
		if (!passed) {
			model_print("Interface %d failed\n", interface_num);
			model_print("ID: %d\n", __ID__);
			model_print("Error exists in simple check!!\n");
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
								//dumpNode(n1);
								//dumpNode(n2);
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


/**
	Topologically sort the commit points graph; If they are not restricted to
	any of the HB, MO & RF rules, they should keep their trace order (to);
*/
node_list_t* SPECAnalysis::sortCPGraph() {
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
				stack->push_back(node);
				if (node->edges != NULL) { // Push back undiscovered nodes
					// Cautious!! Push HB & MO edges first then RF
					for (edge_list_t::reverse_iterator rit =
						node->edges->rbegin(); rit != node->edges->rend(); rit++) {
						commit_point_node *next_node = (*rit)->next_node;
						if (next_node->color == 1) { // back edge -> cycle
							model_print("There exists cycles in this graph!\n");
							return NULL;
						}
						if (next_node->color == 0) {
							stack->push_back(next_node);
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

void SPECAnalysis::buildEdges() {
	//model_print("Building edges...\n");
	action_list_t::iterator iter1, iter2;
	CycleGraph *mo_graph = execution->get_mo_graph();
	for (iter1 = cpActions->begin(); iter1 != cpActions->end(); iter1++) {
		for (iter2 = iter1, iter2++; iter2 != cpActions->end(); iter2++) {
			// Build happens-before edges
			const ModelAction *act1 = *iter1,
				*act2 = *iter2;
			commit_point_node *node1 = cpGraph->get(act1),	
				*node2 = cpGraph->get(act2);
			// When sorting, we favor RF, so we add RF edge first
			if (act2->get_reads_from() == act1) {
				node1->addEdge(node2, RF);
				//dumpNode(node1);
			} else if (act1->get_reads_from() == act2) {
				node2->addEdge(node1, RF);
			} else { // Deal with HB or MO
				if (act1->happens_before(act2)) {
					node1->addEdge(node2, HB);
				} else if (act2->happens_before(act1)) {
					node2->addEdge(node1, HB);
				} else { // Deal with MO
					if (mo_graph->checkReachable(act1, act2)) {
						node1->addEdge(node2, MO);
					} else if (mo_graph->checkReachable(act2, act1)) {
						node2->addEdge(node1, MO);
					}
				}
			}
		}
	}
	//model_print("Finish building edges!\n");
}

ModelAction* SPECAnalysis::getPrevAction(action_list_t *actions,
	action_list_t::iterator *iter, const ModelAction *anno) {
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

commit_point_node* SPECAnalysis::getCPNode(action_list_t *actions, action_list_t::iterator
	*iter) {
	action_list_t::iterator it = *iter;
	const ModelAction *act = *it;
	thread_id_t tid = act->get_tid();
	commit_point_node *node = new commit_point_node();
	spec_annotation *anno = (spec_annotation*) act->get_location();
	assert(anno->type == INTERFACE_BEGIN);
	anno_interface_begin *begin_anno = (anno_interface_begin*) anno->annotation;
	int interface_num = begin_anno->interface_num;
	node->interface_num = interface_num;
	node->begin = act;
	anno_cp_define *cp_define;
	anno_potential_cp_define *pcp_define;
	// A list of potential commit points
	pcp_list_t *pcp_list = new pcp_list_t();
	potential_cp_info *pcp_info;
	bool hasCommitPoint = false, pcp_defined = false;
	for (it++; it != actions->end(); it++) {
		act = *it;
		if (act->get_type() != ATOMIC_ANNOTATION || act->get_tid() != tid
			|| act->get_value() != SPEC_ANALYSIS)
			continue;
		spec_annotation *anno = (spec_annotation*) act->get_location();
		anno_hb_condition *hb_cond;
		switch (anno->type) {
			case POTENTIAL_CP_DEFINE:
				//model_print("POTENTIAL_CP_DEFINE\n");
				// Just wanna make a faster check
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
				if (!pcp_defined) {
					// Fast check 
					break;
				} else {
					// Check if potential commit has been checked
					for (pcp_list_t::iterator pcp_it = pcp_list->begin(); pcp_it
						!= pcp_list->end(); pcp_it++) {
						pcp_info = *pcp_it;
						if (cp_define->potential_cp_label_num == pcp_info->label_num) {
							if (!hasCommitPoint) {
								hasCommitPoint = true;
								node->operation = pcp_info->operation;
							} else {
								model_print("Multiple commit points.\n");
								return NULL;
							}
						}
					}
				}
				break;
			case CP_DEFINE_CHECK:
				//model_print("CP_DEFINE_CHECK\n");
				node->operation = getPrevAction(actions, &it, act);
				if (hasCommitPoint) {
					model_print("Multiple commit points.\n");
					return NULL;
				}
				hasCommitPoint = true;
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
				if (!hasCommitPoint) {
					model_print("Exception: tid_%d\tinterface %d without commit points!\n",
						node->begin->get_tid(), node->interface_num);
					return NULL;
				}
				node->end = act;
				node->info = ((anno_interface_end*) anno->annotation)->info;
				return node;
			default:
				break;
		}
	}
	// Free the potential commit points list
	for (pcp_list_t::iterator free_it = pcp_list->begin(); free_it !=
		pcp_list->end(); free_it++) {
		delete *free_it;
	}
	delete pcp_list;
	return node;
}

void SPECAnalysis::buildCPGraph(action_list_t *actions) {
	cpGraph = new node_table_t();
	cpActions = new action_list_t();
	action_list_t::iterator begin_anno_iter = actions->begin(), iter;
	for (; begin_anno_iter != actions->end(); begin_anno_iter++) {
		const ModelAction *act = *begin_anno_iter;
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
					cpGraph->put(node->operation, node);
					cpActions->push_back(node->operation);
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
	ModelAction *act = node->operation;
	model_print("Node: %d, %d, %d\n", act->get_seq_number(), act->get_tid(),
		node->interface_num);
	if (node->edges == NULL) return;
	for (edge_list_t::reverse_iterator rit =
		node->edges->rbegin(); rit != node->edges->rend(); rit++) {
		commit_point_node *next_node = (*rit)->next_node;
		const char *relationMsg = (*rit)->type == HB ? "HB" : (*rit)->type == RF ?
			"RF" : "MO";
		model_print("Edge: %d, %d, %s\n", next_node->operation->get_seq_number(),
			next_node->interface_num, relationMsg);
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

void SPECAnalysis::dumpGraph() {
	model_print("---------- Dump Graph Begin ----------\n");
	action_list_t::iterator iter = cpActions->begin();
	for (; iter != cpActions->end(); iter++) {
		ModelAction *act = *iter;
		commit_point_node *node = cpGraph->get(act);
		dumpNode(node);
	}
	model_print("---------- Dump Graph End ----------\n");
}

void SPECAnalysis::traverseActions(action_list_t *actions) {
	action_list_t::iterator iter = actions->begin();
	for (; iter != actions->end(); iter++) {
		const ModelAction *act = *iter;
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
				model_print("tid_%d:\tINTERFACE_BEGIN \tinter_num: %d\n",
					act->get_tid(), begin_anno->interface_num);
				interface_num = begin_anno->interface_num;
				break;
			case POTENTIAL_CP_DEFINE:
				pcp_define = (anno_potential_cp_define*) anno->annotation;
				model_print("tid_%d:\tPOTENTIAL_CP_DEFINE \tlabel: %d\n",
					act->get_tid(), pcp_define->label_num);
				break;
			case CP_DEFINE:
				cp_define = (anno_cp_define*) anno->annotation;
				model_print("tid_%d:\tCP_DEFINE \tpotential_label: %d, label: %d\n",
					act->get_tid(), cp_define->potential_cp_label_num,
						cp_define->label_num);
				break;
			case CP_DEFINE_CHECK:
				cp_define_check = (anno_cp_define_check*) anno->annotation;
				model_print("tid_%d\tCP_DEFINE_CHECK \tlabel_num: %d\n",
					act->get_tid(), cp_define_check->label_num);
				break;
			case INTERFACE_END:
				end_anno = (anno_interface_end*) anno->annotation;
				model_print("tid_%d\tINTERFACE_END \tinter_num: %d\n",
					act->get_tid(), end_anno->interface_num);
				break;
			case HB_CONDITION:
				hb_condition = (anno_hb_condition*) anno->annotation;
				model_print("tid_%d\tHB_CONDITION \tinter_num: %d; hb_cond_num: %d\n",
				act->get_tid(), hb_condition->interface_num,
					hb_condition->hb_condition_num);
				break;
			default:
				break;
		}
	}
}

