#include "scfence.h"
#include "action.h"
#include "threads-model.h"
#include "clockvector.h"
#include "execution.h"
#include <sys/time.h>

#include "model.h"
#include "wildcard.h"


SCFence::SCFence() :
	cvmap(),
	cyclic(false),
	badrfset(),
	lastwrmap(),
	threadlists(1),
	execution(NULL),
	print_always(false),
	print_buggy(true),
	print_nonsc(false),
	time(false),
	stats((struct sc_statistics *)model_calloc(1, sizeof(struct sc_statistics))),
	curWildcardMap(),
	curInference(),
	potentialResults()
{
}

SCFence::~SCFence() {
	delete(stats);
}

void SCFence::setExecution(ModelExecution * execution) {
	this->execution=execution;
}

const char * SCFence::name() {
	const char * name = "SCFENCE";
	return name;
}

void SCFence::finish() {
	if (time)
		model_print("Elapsed time in usec %llu\n", stats->elapsedtime);
	model_print("SC count: %u\n", stats->sccount);
	model_print("Non-SC count: %u\n", stats->nonsccount);
	model_print("Total actions: %llu\n", stats->actions);
	unsigned long long actionperexec=(stats->actions)/(stats->sccount+stats->nonsccount);
	model_print("Actions per execution: %llu\n", actionperexec);
}


void SCFence::inspectModelAction(ModelAction *act) {
	if (act->get_mo() >= memory_order_relaxed && act->get_mo() <=
		memory_order_seq_cst) {
		return;
	} else { // For wildcards
		if (curWildcardMap->get(act->get_mo()) == NULL) {
			act->set_mo(memory_order_relaxed);
		} else {
			act->set_mo(curWildcardMap->get(act->get_mo()));
		}
	}
}


void SCFence::actionAtInstallation() {
	// When this pluggin gets installed, it become the inspect_plugin
	model->set_inspect_plugin(this);
}

const char * SCFence::get_mo_str(memory_order order) {
	switch (order) {
		case std::memory_order_relaxed: return "relaxed";
		case std::memory_order_acquire: return "acquire";
		case std::memory_order_release: return "release";
		case std::memory_order_acq_rel: return "acq_rel";
		case std::memory_order_seq_cst: return "seq_cst";
		default: return "unknown";
	}
}

void SCFence::printWildcardResult(inference_list_t *result) {
	for (inference_list_t::iterator it = result->begin(); it
		!= result->end(); it++) {
		InferencePair pair = *it;
		memory_order wildcard = pair.wildcard,
			order = pair.order;
		// Print the wildcard inference result
		model_print("wildcard%d -> memory_order_%s\n",
			get_wildcard_id(wildcard), get_mo_str(order));
	}
}

void SCFence::actionAtModelCheckingFinish() {
	// Print the wildcard result
	printWildcardResult(curInference);

	// FIXME: 
	if (potentialResults.size() != 0) {
		inference_list_t *result = potentialResults.front();
		potentialResults.pop_front();
		for (inference_list_t::iterator it = result->begin(); it
			!= result->end(); it++) {

		}



		model->restart();
	}
	
}

bool SCFence::option(char * opt) {
	if (strcmp(opt, "verbose")==0) {
		print_always=true;
		return false;
	} else if (strcmp(opt, "buggy")==0) {
		return false;
	} else if (strcmp(opt, "quiet")==0) {
		print_buggy=false;
		return false;
	} else if (strcmp(opt, "nonsc")==0) {
		print_nonsc=true;
		return false;
	} else if (strcmp(opt, "time")==0) {
		time=true;
		return false;
	} else if (strcmp(opt, "help") != 0) {
		model_print("Unrecognized option: %s\n", opt);
	}

	model_print("SC Analysis options\n");
	model_print("verbose -- print all feasible executions\n");
	model_print("buggy -- print only buggy executions (default)\n");
	model_print("nonsc -- print non-sc execution\n");
	model_print("quiet -- print nothing\n");
	model_print("time -- time execution of scanalysis\n");
	model_print("\n");
	
	return true;
}

void SCFence::print_list(action_list_t *list) {
	model_print("---------------------------------------------------------------------\n");
	if (cyclic)
		model_print("Not SC\n");
	unsigned int hash = 0;

	for (action_list_t::iterator it = list->begin(); it != list->end(); it++) {
		const ModelAction *act = *it;
		if (act->get_seq_number() > 0) {
			if (badrfset.contains(act))
				model_print("BRF ");
			act->print();
			if (badrfset.contains(act)) {
				model_print("Desired Rf: %u \n", badrfset.get(act)->get_seq_number());
			}
		}
		hash = hash ^ (hash << 3) ^ ((*it)->hash());
	}
	model_print("HASH %u\n", hash);
	model_print("---------------------------------------------------------------------\n");
}

void SCFence::analyze(action_list_t *actions) {

	struct timeval start;
	struct timeval finish;
	if (time)
		gettimeofday(&start, NULL);
	
	graph = new sc_graph(actions);
	action_list_t *list = generateSC(actions);
	// Now we find a non-SC execution
	if (cyclic) {
		//graph->printGraph();
		graph->printCyclicChain(cycle_act1, cycle_act2);
		print_list(actions);
		breakCycle(cycle_act1, cycle_act2);
		model->restart();
	}
	// Don't forget to clear the graph every time when we are done
	graph->clear();
	
	check_rf(list);
	if (print_always || (print_buggy && execution->have_bug_reports())|| (print_nonsc && cyclic))
		print_list(list);
	if (time) {
		gettimeofday(&finish, NULL);
		stats->elapsedtime+=((finish.tv_sec*1000000+finish.tv_usec)-(start.tv_sec*1000000+start.tv_usec));
	}
	update_stats();
}

void SCFence::update_stats() {
	if (cyclic) {
		stats->nonsccount++;
	} else {
		stats->sccount++;
	}
}

void SCFence::check_rf(action_list_t *list) {
	for (action_list_t::iterator it = list->begin(); it != list->end(); it++) {
		const ModelAction *act = *it;
		if (act->is_read()) {
			if (act->get_reads_from() != lastwrmap.get(act->get_location()))
				badrfset.put(act, lastwrmap.get(act->get_location()));
		}
		if (act->is_write())
			lastwrmap.put(act->get_location(), act);
	}
}


void SCFence::breakCycle(const ModelAction *act1, const ModelAction *act2) {

}


bool SCFence::merge(ClockVector *cv, const ModelAction *act, const ModelAction *act2) {
	ClockVector *cv2 = cvmap.get(act2);
	if (cv2 == NULL)
		return true;
	
	// Add an edge: act2 -> act
	if (act2->get_seq_number() != 0 && act->get_seq_number() != 0) {
		if (!cyclic) { // Only add an edge when there's no cycle
			graph->addEdge(act2, act);
		}
		/*
		if (act2->get_seq_number() == 10 && act->get_seq_number() == 3) {
			model_print("%d ---> %d!\n", act2->get_seq_number(),
				act->get_seq_number());
		} else {
			model_print("%d -> %d\n", act2->get_seq_number(),
				act->get_seq_number());
		}
		*/
	}


	if (cv2->getClock(act->get_tid()) >= act->get_seq_number() && act->get_seq_number() != 0) {
		if (!cyclic) {
			cycle_act1 = act2;
			cycle_act2 = act;
			/*
			model_print("Node1: %d\n", cycle_act1->get_seq_number());
			model_print("Node2: %d\n", cycle_act2->get_seq_number());
			*/
		}
		cyclic = true;

		//refuse to introduce cycles into clock vectors
		return false;
	}
	return cv->merge(cv2);

}

int SCFence::getNextActions(ModelAction ** array) {
	int count=0;

	for (int t = 0; t <= maxthreads; t++) {
		action_list_t *tlt = &threadlists[t];
		if (tlt->empty())
			continue;
		ModelAction *act = tlt->front();
		ClockVector *cv = cvmap.get(act);
		
		/* Find the earliest in SC ordering */
		for (int i = 0; i <= maxthreads; i++) {
			if ( i == t )
				continue;
			action_list_t *threadlist = &threadlists[i];
			if (threadlist->empty())
				continue;
			ModelAction *first = threadlist->front();
			if (cv->synchronized_since(first)) {
				act = NULL;
				break;
			}
		}
		if (act != NULL) {
			array[count++]=act;
		}
	}
	if (count != 0)
		return count;
	for (int t = 0; t <= maxthreads; t++) {
		action_list_t *tlt = &threadlists[t];
		if (tlt->empty())
			continue;
		ModelAction *act = tlt->front();
		ClockVector *cv = act->get_cv();
		
		/* Find the earliest in SC ordering */
		for (int i = 0; i <= maxthreads; i++) {
			if ( i == t )
				continue;
			action_list_t *threadlist = &threadlists[i];
			if (threadlist->empty())
				continue;
			ModelAction *first = threadlist->front();
			if (cv->synchronized_since(first)) {
				act = NULL;
				break;
			}
		}
		if (act != NULL) {
			array[count++]=act;
		}
	}

	ASSERT(count==0 || cyclic);

	return count;
}

ModelAction * SCFence::pruneArray(ModelAction **array,int count) {
	/* No choice */
	if (count == 1)
		return array[0];

	/* Choose first non-write action */
	ModelAction *nonwrite=NULL;
	for(int i=0;i<count;i++) {
		if (!array[i]->is_write())
			if (nonwrite==NULL || nonwrite->get_seq_number() > array[i]->get_seq_number())
				nonwrite = array[i];
	}
	if (nonwrite != NULL)
		return nonwrite;
	
	/* Look for non-conflicting action */
	ModelAction *nonconflict=NULL;
	for(int a=0;a<count;a++) {
		ModelAction *act=array[a];
		for (int i = 0; i <= maxthreads && act != NULL; i++) {
			thread_id_t tid = int_to_id(i);
			if (tid == act->get_tid())
				continue;
			
			action_list_t *list = &threadlists[id_to_int(tid)];
			for (action_list_t::iterator rit = list->begin(); rit != list->end(); rit++) {
				ModelAction *write = *rit;
				if (!write->is_write())
					continue;
				ClockVector *writecv = cvmap.get(write);
				if (writecv->synchronized_since(act))
					break;
				if (write->get_location() == act->get_location()) {
					//write is sc after act
					act = NULL;
					break;
				}
			}
		}
		if (act != NULL) {
			if (nonconflict == NULL || nonconflict->get_seq_number() > act->get_seq_number())
				nonconflict=act;
		}
	}
	return nonconflict;
}

action_list_t * SCFence::generateSC(action_list_t *list) {
 	int numactions=buildVectors(&threadlists, &maxthreads, list);
	stats->actions+=numactions;

	computeCV(list);

	action_list_t *sclist = new action_list_t();
	ModelAction **array = (ModelAction **)model_calloc(1, (maxthreads + 1) * sizeof(ModelAction *));
	int * choices = (int *) model_calloc(1, sizeof(int)*numactions);
	int endchoice = 0;
	int currchoice = 0;
	int lastchoice = -1;
	while (true) {
		int numActions = getNextActions(array);
		if (numActions == 0)
			break;
		ModelAction * act=pruneArray(array, numActions);
		if (act == NULL) {
			if (currchoice < endchoice) {
				act = array[choices[currchoice]];
				//check whether there is still another option
				if ((choices[currchoice]+1)<numActions)
					lastchoice=currchoice;
				currchoice++;
			} else {
				act = array[0];
				choices[currchoice]=0;
				if (numActions>1)
					lastchoice=currchoice;
				currchoice++;
			}
		}
		thread_id_t tid = act->get_tid();
		//remove action
		threadlists[id_to_int(tid)].pop_front();
		//add ordering constraints from this choice
		if (updateConstraints(act)) {
			//propagate changes if we have them
			bool prevc=cyclic;
			computeCV(list);
			if (!prevc && cyclic) {
				model_print("ROLLBACK in SC\n");
				//check whether we have another choice
				if (lastchoice != -1) {
					//have to reset everything
					choices[lastchoice]++;
					endchoice=lastchoice+1;
					currchoice=0;
					lastchoice=-1;
					// Also need to reset the graph
					graph->reset(list);

					reset(list);
					buildVectors(&threadlists, &maxthreads, list);
					computeCV(list);
					sclist->clear();
					continue;
				}
			}
		}
		//add action to end
		sclist->push_back(act);
	}
	model_free(array);
	return sclist;
}

int SCFence::buildVectors(SnapVector<action_list_t> *threadlist, int *maxthread, action_list_t *list) {
	*maxthread = 0;
	int numactions = 0;
	for (action_list_t::iterator it = list->begin(); it != list->end(); it++) {
		ModelAction *act = *it;
		numactions++;
		int threadid = id_to_int(act->get_tid());
		if (threadid > *maxthread) {
			threadlist->resize(threadid + 1);
			*maxthread = threadid;
		}
		(*threadlist)[threadid].push_back(act);
	}
	return numactions;
}

void SCFence::reset(action_list_t *list) {
	for (int t = 0; t <= maxthreads; t++) {
		action_list_t *tlt = &threadlists[t];
		tlt->clear();
	}
	for (action_list_t::iterator it = list->begin(); it != list->end(); it++) {
		ModelAction *act = *it;
		delete cvmap.get(act);
		cvmap.put(act, NULL);
	}

	cyclic=false;	
}

bool SCFence::updateConstraints(ModelAction *act) {
	bool changed = false;
	for (int i = 0; i <= maxthreads; i++) {
		thread_id_t tid = int_to_id(i);
		if (tid == act->get_tid())
			continue;

		action_list_t *list = &threadlists[id_to_int(tid)];
		for (action_list_t::iterator rit = list->begin(); rit != list->end(); rit++) {
			ModelAction *write = *rit;
			if (!write->is_write())
				continue;
			ClockVector *writecv = cvmap.get(write);
			if (writecv->synchronized_since(act))
				break;
			if (write->get_location() == act->get_location()) {
				//write is sc after act
				merge(writecv, write, act);
				changed = true;
				break;
			}
		}
	}
	return changed;
}

bool SCFence::processRead(ModelAction *read, ClockVector *cv) {
	bool changed = false;
	/*
	model_print("processRead:\n");
	cv->print();
	read->print();
	*/

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
			}

			//looking for earliest write2 in iteration to satisfy this
			/* write2 -sc-> R &&
				 write -rf-> R =>
				 write2 -sc-> write */
			if (cv->synchronized_since(write2)) {
				/*
				if (write2->get_seq_number() != 0) {
					model_print("write2: %d\n", write2->get_seq_number());
					write2->get_cv()->print();
					model_print("write: %d\n", write->get_seq_number());
					model_print("read: %d\n", read->get_seq_number());
					cv->print();	
				}
				*/
				changed |= writecv == NULL || merge(writecv, write, write2);
				break;
			}
		}
	}
	return changed;
}

void SCFence::computeCV(action_list_t *list) {
	bool changed = true;
	bool firsttime = true;
	ModelAction **last_act = (ModelAction **)model_calloc(1, (maxthreads + 1) * sizeof(ModelAction *));
	while (changed) {
		changed = changed&firsttime;
		firsttime = false;

		for (action_list_t::iterator it = list->begin(); it != list->end(); it++) {
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
