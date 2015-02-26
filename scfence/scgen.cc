#include "scgen.h"
#include "action.h"
#include "threads-model.h"
#include "clockvector.h"
#include "execution.h"
#include "sc_annotation.h"
#include <sys/time.h>

#include "model.h"
#include <stdio.h>
#include <algorithm>

SCGenerator::SCGenerator() :
	cvmap(),
	actions(actions),
	cyclic(false),
	badrfset(),
	lastwrmap(),
	threadlists(1),
	dup_threadlists(1),
	execution(NULL),
	print_always(false),
	print_buggy(false),
	print_nonsc(false)
{
	stats = new struct sc_statistics;
}

SCGenerator::~SCGenerator() {
}

void SCGenerator::print_list(action_list_t *list) {
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
			cvmap.get(act)->print();
		}
		hash = hash ^ (hash << 3) ^ ((*it)->hash());
	}
	model_print("HASH %u\n", hash);
	model_print("---------------------------------------------------------------------\n");
}

/******************** SCFence-related Functions (End) ********************/
void SCGenerator::check_rf(action_list_t *list) {
	bool hasBadRF = false;
	for (action_list_t::iterator it = list->begin(); it != list->end(); it++) {
		const ModelAction *act = *it;
		if (act->is_read()) {
			if (act->get_reads_from() != lastwrmap.get(act->get_location())) {
				badrfset.put(act, lastwrmap.get(act->get_location()));
				hasBadRF = true;
			}
		}
		if (act->is_write())
			lastwrmap.put(act->get_location(), act);
	}
	//ASSERT (cyclic == hasBadRF);
}


bool SCGenerator::merge(ClockVector *cv, const ModelAction *act, const ModelAction *act2) {
	ClockVector *cv2 = cvmap.get(act2);
	if (cv2 == NULL)
		return true;

	if (cv2->getClock(act->get_tid()) >= act->get_seq_number() && act->get_seq_number() != 0) {
		cyclic = true;
		//refuse to introduce cycles into clock vectors
		return false;
	}
	if (fastVersion) {
		bool status = cv->merge(cv2);
		return status;
	} else {
		bool merged;
		if (allowNonSC) {
			merged = cv->merge(cv2);
			if (merged)
				allowNonSC = false;
			return merged;
		} else {
			if (act2->happens_before(act) ||
				(act->is_seqcst() && act2->is_seqcst() && *act2 < *act)) {
				return cv->merge(cv2);
			} else {
				return false;
			}
		}
	}

}

int SCGenerator::getNextActions(ModelAction ** array) {
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

ModelAction * SCGenerator::pruneArray(ModelAction **array,int count) {
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

action_list_t* SCGenerator::getSCList() {
	struct timeval start;
	struct timeval finish;
	gettimeofday(&start, NULL);
	
	/* Build up the thread lists for general purpose */
	int thrdNum;
	buildVectors(&dup_threadlists, &thrdNum, actions);
	
	fastVersion = true;
	action_list_t *list = generateSC(actions);
	if (cyclic) {
		reset(actions);
		delete list;
		fastVersion = false;
		list = generateSC(actions);
	}
	check_rf(list);
	gettimeofday(&finish, NULL);
	stats->elapsedtime+=((finish.tv_sec*1000000+finish.tv_usec)-(start.tv_sec*1000000+start.tv_usec));
	update_stats();
	return list;
}

void SCGenerator::update_stats() {
	if (cyclic) {
		stats->nonsccount++;
	} else {
		stats->sccount++;
	}
}

action_list_t * SCGenerator::generateSC(action_list_t *list) {
 	int numactions=buildVectors(&threadlists, &maxthreads, list);
	stats->actions+=numactions;

	// Analyze which actions we should ignore in the partially SC analysis
	if (annotationMode) {
		collectAnnotatedReads();
		if (annotationError) {
			model_print("Annotation error!\n");
			return NULL;
		}
	}

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

int SCGenerator::buildVectors(SnapVector<action_list_t> *threadlist, int *maxthread, action_list_t *list) {
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

void SCGenerator::reset(action_list_t *list) {
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

bool SCGenerator::updateConstraints(ModelAction *act) {
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

bool SCGenerator::processReadFast(ModelAction *read, ClockVector *cv) {
	bool changed = false;

	/* Merge in the clock vector from the write */
	const ModelAction *write = read->get_reads_from();
	ClockVector *writecv = cvmap.get(write);
	changed |= merge(cv, read, write) && (*read < *write);

	for (int i = 0; i <= maxthreads; i++) {
		thread_id_t tid = int_to_id(i);
		if (tid == read->get_tid())
			continue;
		// Can't ignore writes from the same thread
		//if (tid == write->get_tid())
		//	continue;
		action_list_t *list = execution->get_actions_on_obj(read->get_location(), tid);
		if (list == NULL)
			continue;
		for (action_list_t::reverse_iterator rit = list->rbegin(); rit != list->rend(); rit++) {
			ModelAction *write2 = *rit;
			if (!write2->is_write())
				continue;
			if (write2 == write)
				continue;
			ClockVector *write2cv = cvmap.get(write2);
			if (write2cv == NULL)
				continue;
			/* write -sc-> write2 &&
				 write -rf-> R =>
				 R -sc-> write2 */
			if (write2cv->synchronized_since(write)) {

			model_print("infer1:\n");
			write->print();
			cvmap.get(write)->print();
			write2->print();
			cvmap.get(write2)->print();
			read->print();
			cvmap.get(read)->print();


				changed |= merge(write2cv, write2, read);
			}

			//looking for earliest write2 in iteration to satisfy this
			/* write2 -sc-> R &&
				 write -rf-> R =>
				 write2 -sc-> write */
			if (cv->synchronized_since(write2)) {
				changed |= writecv == NULL || merge(writecv, write, write2);
				break;
			}
		}
	}
	return changed;
}

bool SCGenerator::processReadSlow(ModelAction *read, ClockVector *cv, bool *updateFuture) {
	bool changed = false;
	
	/* Merge in the clock vector from the write */
	const ModelAction *write = read->get_reads_from();
	ClockVector *writecv = cvmap.get(write);
	if ((*write < *read) || ! *updateFuture) {
		bool status = merge(cv, read, write) && (*read < *write);
		changed |= status;
		*updateFuture = status;
	}

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
				if ((*read < *write2) || ! *updateFuture) {
					bool status = merge(write2cv, write2, read);
					changed |= status;
					*updateFuture |= status && (*write2 < *read);
				}
			}

			//looking for earliest write2 in iteration to satisfy this
			/* write2 -sc-> R &&
				 write -rf-> R =>
				 write2 -sc-> write */
			if (cv->synchronized_since(write2)) {
				if ((*write2 < *write) || ! *updateFuture) {
					bool status = writecv == NULL || merge(writecv, write, write2);
					changed |= status;
					*updateFuture |= status && (*write < *write2);
				}
				break;
			}
		}
	}
	return changed;
}

bool SCGenerator::processAnnotatedReadSlow(ModelAction *read, ClockVector *cv, bool *updateFuture) {
	bool changed = false;
	
	/* Merge in the clock vector from the write */
	const ModelAction *write = read->get_reads_from();
	if ((*write < *read) || ! *updateFuture) {
		bool status = merge(cv, read, write) && (*read < *write);
		changed |= status;
		*updateFuture = status;
	}
	return changed;
}

/** When we call this function, we should first have built the threadlists */
void SCGenerator::collectAnnotatedReads() {
	for (unsigned i = 1; i < threadlists.size(); i++) {
		action_list_t *list = &threadlists.at(i);
		for (action_list_t::iterator it = list->begin(); it != list->end(); it++) {
			ModelAction *act = *it;
			if (!IS_SC_ANNO(act))
				continue;
			if (!IS_ANNO_BEGIN(act)) {
				model_print("SC annotation should begin with a BEGIN annotation\n");
				annotationError = true;
				return;
			}
			act = *++it;
			while (!IS_ANNO_END(act) && it != list->end()) {
				// Look for the actions to keep in this loop
				ModelAction *nextAct = *++it;
				//model_print("in the loop\n");
				//act->print();
				//nextAct->print();
				if (!IS_ANNO_KEEP(nextAct)) { // Annotated reads
					act->print();
					annotatedReadSet.put(act, act);
					annotatedReadSetSize++;
					if (IS_ANNO_END(nextAct))
						break;
				}
			}
			if (it == list->end()) {
				model_print("SC annotation should end with a END annotation\n");
				annotationError = true;
				return;
			}
		}
	}
}


void SCGenerator::computeCV(action_list_t *list) {
	bool changed = true;
	bool firsttime = true;
	ModelAction **last_act = (ModelAction **)model_calloc(1, (maxthreads + 1) * sizeof(ModelAction *));

	while (changed) {
		changed = changed&firsttime;
		firsttime = false;
		bool updateFuture = false;

		for (action_list_t::iterator it = list->begin(); it != list->end(); it++) {
			ModelAction *act = *it;
			ModelAction *lastact = last_act[id_to_int(act->get_tid())];
			if (act->is_thread_start())
				lastact = execution->get_thread(act)->get_creation();
			last_act[id_to_int(act->get_tid())] = act;
			ClockVector *cv = cvmap.get(act);
			if (act->get_seq_number() == 13) {
				model_print("CV: %d\n", cv);
			}
			if (cv == NULL) {
				cv = new ClockVector(act->get_cv(), act);
				model_print("New CV\n");
				act->print();
				cvmap.put(act, cv);
			}
			if (act->get_seq_number() == 13) {
				act->print();
				cv->print();
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
				if (fastVersion) {
					changed |= processReadFast(act, cv);
				} else if (annotatedReadSet.contains(act)) {
					changed |= processAnnotatedReadSlow(act, cv, &updateFuture);
				} else {
					changed |= processReadSlow(act, cv, &updateFuture);
				}
			}
		}
		/* Reset the last action array */
		if (changed) {
			bzero(last_act, (maxthreads + 1) * sizeof(ModelAction *));
		} else {
			if (!fastVersion) {
				if (!allowNonSC) {
					allowNonSC = true;
					changed = true;
				} else {
					break;
				}
			}
		}
	}
	model_free(last_act);
}
