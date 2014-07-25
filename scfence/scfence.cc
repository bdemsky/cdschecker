#include "scfence.h"
#include "action.h"
#include "threads-model.h"
#include "clockvector.h"
#include "execution.h"
#include <sys/time.h>

#include "model.h"
#include "wildcard.h"
#include <stdio.h>
#include <algorithm>


void print_nothing(const char *str, ...) {

}

Inference *SCFence::curInference;
char *SCFence::candidateFile;
ModelList<Inference *> *SCFence::results;
ModelList<Inference *> *SCFence::potentialResults;

bool SCFence::inferImplicitMO;
int SCFence::implicitMOReadBound;

SCFence::SCFence() :
	cvmap(),
	cyclic(false),
	badrfset(),
	lastwrmap(),
	threadlists(1),
	dup_threadlists(1),
	execution(NULL),
	print_always(false),
	print_buggy(true),
	print_nonsc(false),
	time(false),
	stats((struct sc_statistics *)model_calloc(1, sizeof(struct sc_statistics)))
{
	curInference = new Inference();
	potentialResults = new ModelList<Inference*>();
	results = new ModelList<Inference*>();
	candidateFile = NULL;
	inferImplicitMO = false;
	implicitMOReadBound = DEFAULT_REPETITIVE_READ_BOUND;
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
	model_print("SCFence finishes!\n");
}


void SCFence::inspectModelAction(ModelAction *act) {
	if (act == NULL) // Get pass cases like data race detector
		return;
	if (act->get_mo() >= memory_order_relaxed && act->get_mo() <=
		memory_order_seq_cst) {
		return;
	} else { // For wildcards
		memory_order wildcard = act->get_mo();
		int wildcardID = get_wildcard_id(act->get_mo());
		memory_order order = (*curInference)[wildcardID];
		if (order == WILDCARD_NONEXIST) {
			(*curInference)[wildcardID] = memory_order_relaxed;
			act->set_mo(memory_order_relaxed);
		} else {
			act->set_mo((*curInference)[wildcardID]);
		}
	}
}


void SCFence::actionAtInstallation() {
	// When this pluggin gets installed, it become the inspect_plugin
	model->set_inspect_plugin(this);
}

static const char * get_mo_str(memory_order order) {
	switch (order) {
		case std::memory_order_relaxed: return "relaxed";
		case std::memory_order_acquire: return "acquire";
		case std::memory_order_release: return "release";
		case std::memory_order_acq_rel: return "acq_rel";
		case std::memory_order_seq_cst: return "seq_cst";
		default: 
			model_print("Weird memory order, a bug or something!\n");
			model_print("memory_order: %d\n", order);
			return "unknown";
	}
}


void SCFence::pruneResults() {
	ModelList<Inference*> *newResults = new ModelList<Inference*>();
	ModelList<Inference*>::iterator it, itNew;

	addMoreCandidates(newResults, results);
	results->clear();
	model_free(results);
	results = newResults;
	return;
}

void SCFence::actionAtModelCheckingFinish() {
	// First add the current inference to the result list
	results->push_back(curInference);
	// Remove those potential candidates stronger or equal to curInference
	for (ModelList<Inference*>::iterator it = potentialResults->begin(); it !=
		potentialResults->end(); it++) {
		Inference *infer = *it;
		int compVal = infer->compareTo(curInference);
		if (compVal == 0 || compVal == 1) {
			// Remove it
			it = potentialResults->erase(it);
			it--;
		}
	}

	if (time)
		model_print("Elapsed time in usec %llu\n", stats->elapsedtime);
	model_print("SC count: %u\n", stats->sccount);
	model_print("Non-SC count: %u\n", stats->nonsccount);
	model_print("Total actions: %llu\n", stats->actions);
	unsigned long long actionperexec=(stats->actions)/(stats->sccount+stats->nonsccount);

	
	if (potentialResults->size() > 0) { // Still have candidates to explore
		curInference = potentialResults->front();
		potentialResults->pop_front();
		model->restart();
	} else {
		model_print("Result!\n");
		model_print("Original size: %d!\n", results->size());
		pruneResults();
		model_print("Pruned size: %d!\n", results->size());
		printWildcardResults(results);
	}
}

void SCFence::initializeByFile() {
	FILE *fp = fopen(candidateFile, "r");
	if (fp == NULL) {
		fprintf(stderr, "Cannot open the file!\n");
		return;
	}
	Inference *infer = NULL;
	int size = 0,
		curNum = 0;
	memory_order mo;
	char *str = (char *) malloc(sizeof(char) * (30 + 1));
	bool isProcessing = false;
	while (!feof(fp)) {
		// Result #:
		if (!isProcessing) {
			fscanf(fp, "%s", str);

		}
		if (strcmp(str, "Result") == 0 || isProcessing) { // In an inference
			fscanf(fp, "%s", str);
			infer = new Inference();
			isProcessing = false;
			while (!feof(fp)) { // Processing a specific inference
				// wildcard # -> memory_order
				fscanf(fp, "%s", str); // wildcard
				if (strcmp(str, "Result") == 0) {
					//FENCE_PRINT("break: %s\n", str);
					isProcessing = true;
					break;
				}
				//FENCE_PRINT("%s ", str);
				fscanf(fp, "%d", &curNum); // #
				//FENCE_PRINT("%d -> ", curNum);
				fscanf(fp, "%s", str); // ->
				fscanf(fp, "%s", str);
				//FENCE_PRINT("%s\n", str);
				if (strcmp(str, "memory_order_relaxed") == 0)
					mo = memory_order_relaxed;
				else if (strcmp(str, "memory_order_acquire") == 0)
					mo = memory_order_acquire;
				else if (strcmp(str, "memory_order_release") == 0)
					mo = memory_order_release;
				else if (strcmp(str, "memory_order_acq_rel") == 0)
					mo = memory_order_acq_rel;
				else if (strcmp(str, "memory_order_seq_cst") == 0)
					mo = memory_order_seq_cst;
				(*infer)[curNum] = mo;
			}
			if (!addMoreCandidate(potentialResults, infer))
				delete infer;
		}
	}
	FENCE_PRINT("candidate size from file: %d\n", potentialResults->size());
	curInference =  potentialResults->front();
	potentialResults->pop_front();
	fclose(fp);
}

char* SCFence::parseOptionHelper(char *opt, int *optIdx) {
	char *res = (char*) model_malloc(1024 * sizeof(char));
	int resIdx = 0;
	while (opt[*optIdx] != '\0' && opt[*optIdx] != '_') {
		res[resIdx++] = opt[(*optIdx)++];
	}
	res[resIdx] = '\0';
	return res;
}

bool SCFence::parseOption(char *opt) {
	int optIdx = 0;
	char *val = NULL;
	while (true) {
		char option = opt[optIdx++];
		val = parseOptionHelper(opt, &optIdx);
		switch (option) {
			case 'f': // Read initial inference from file
				//model_print("Parsing f option!\n");
				candidateFile = val;
				//model_print("f value: %s\n", val);
				if (strcmp(val, "") == 0) {
					model_print("Need to specify a file that contains initial inference!\n");
					return true;
				}
				break;
			case 'b': // The bound above 
				//model_print("Parsing b option!\n");
				//model_print("b value: %s\n", val);
				implicitMOReadBound = atoi(val);
				if (implicitMOReadBound <= 0) {
					model_print("Enter valid bound value!\n");
					return true;
				}
				break;
			case 'm': // Infer the modification order from repetitive reads from
					  // the same write
				//model_print("Parsing m option!\n");
				//model_print("m value: %s\n", val);
				inferImplicitMO = true;
				if (strcmp(val, "") != 0) {
					model_print("option doesn't take any arguments!\n");
					return true;
				}
				break;
			default:
				model_print("Unknown SCFence option: %c!\n", option);
				return true;
				break;
		}
		if (opt[optIdx] == '_') {
			optIdx++;
		} else {
			break;
		}
	}
	return false;

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
	} else if (!parseOption(opt)) {
		if (candidateFile != NULL)
			initializeByFile();
		return false;
	} else {
		model_print("SC Analysis options\n");
		model_print("verbose -- print all feasible executions\n");
		model_print("buggy -- print only buggy executions (default)\n");
		model_print("nonsc -- print non-sc execution\n");
		model_print("quiet -- print nothing\n");
		model_print("time -- time execution of scanalysis\n");
		
		model_print("OR short options with _ (understore) as separator\n");
		model_print("f -- takes candidate file as argument\n");
		model_print("m -- imply implicit modification order, takes no arguments\n");
		model_print("b -- specify the bound for the mo implication, takes a number as argument\n");
		model_print("\n");
		return true;
	}
	
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


ModelList<Inference*>* SCFence::imposeSync(ModelList<Inference*> *partialCandidates, sync_paths_t *paths) {
	bool wasNullList = false;
	if (partialCandidates == NULL) {
		partialCandidates = new ModelList<Inference*>();
		wasNullList = true;
	}

	for (sync_paths_t::iterator i_paths = paths->begin(); i_paths != paths->end(); i_paths++) {
		action_list_t *path = *i_paths;
		bool release_seq = true;
		action_list_t::iterator it = path->begin(),
			it_next;
		for (; it != path->end(); it = it_next) {
			const ModelAction *read = *it,
				*write = read->get_reads_from(),
				*next_read;
			it_next = it++;
			if (it_next != path->end()) {
				next_read = *it_next;
				if (write != next_read) {
					release_seq = false;
					break;
				}
			}
		}
		// Already know whether this can be fixed by release sequence
		bool updateSucc = true;
		if (wasNullList) {
			Inference *infer = new Inference(curInference);
			if (release_seq) {
				const ModelAction *relHead = path->front()->get_reads_from(),
					*lastRead = path->back();
				updateSucc = infer->strengthen(relHead, memory_order_release);
				updateSucc = infer->strengthen(lastRead, memory_order_acquire);
			} else {
				for (action_list_t::iterator it = path->begin(); it != path->end(); it++) {
					const ModelAction *read = *it,
						*write = read->get_reads_from();
					//FENCE_PRINT("path size:%d\n", path->size());
					//write->print();
					//read->print();
					updateSucc = infer->strengthen(write, memory_order_release);
					updateSucc = infer->strengthen(read, memory_order_acquire);
				}
			}
			if (updateSucc) {
				partialCandidates->push_back(infer);
			} else {
				// This inference won't work
				delete infer;
			}
		} else { // We have a partial list of candidates
			for (ModelList<Inference*>::iterator i_cand =
				partialCandidates->begin(); i_cand != partialCandidates->end();
				i_cand++) {
				updateSucc = true;
				Inference *infer = *i_cand;
				if (release_seq) {
					const ModelAction *relHead = path->front()->get_reads_from(),
						*lastRead = path->back();
					updateSucc = infer->strengthen(relHead, memory_order_release);
					updateSucc = infer->strengthen(lastRead, memory_order_acquire);
				} else {
					for (action_list_t::iterator it = path->begin(); it !=
						path->end(); it++) {
						const ModelAction *read = *it,
							*write = read->get_reads_from();
						updateSucc = infer->strengthen(write, memory_order_release);
						updateSucc = infer->strengthen(read, memory_order_acquire);
					}	
				}
				if (!updateSucc) {
					// This inference won't work
					partialCandidates->erase(i_cand);
					i_cand--;
					delete infer;
				}
			}
		}
	}

	return partialCandidates;
}

ModelList<Inference*>* SCFence::imposeSC(ModelList<Inference*> *partialCandidates, const ModelAction *act1, const ModelAction *act2) {
	bool wasNullList = false;
	if (partialCandidates == NULL) {
		partialCandidates = new ModelList<Inference*>();
		wasNullList = true;
	}

	bool updateSucc = true;
	if (wasNullList) {
		Inference *infer = new Inference(curInference);
		updateSucc = infer->strengthen(act1, memory_order_seq_cst);
		updateSucc = infer->strengthen(act2, memory_order_seq_cst);

		if (updateSucc) {
			partialCandidates->push_back(infer);
		} else {
			// This inference won't work
			delete infer;
		}

	} else { // We have a partial list of candidates
		for (ModelList<Inference*>::iterator i_cand =
			partialCandidates->begin(); i_cand != partialCandidates->end();
			i_cand++) {
			updateSucc = true;
			Inference *infer = *i_cand;
			updateSucc = infer->strengthen(act1, memory_order_seq_cst);
			updateSucc = infer->strengthen(act2, memory_order_seq_cst);

			if (!updateSucc) {
				// This inference won't work
				partialCandidates->erase(i_cand);
				i_cand--;
				delete infer;
			}
		}
	}
	return partialCandidates;
}

bool SCFence::addMoreCandidate(ModelList<Inference*> *existCandidates, Inference *newCandidate) {
	ModelList<Inference*>::iterator it;
	int res;
	bool isWeaker = false;
	for (it = existCandidates->begin(); it != existCandidates->end(); it++) {
		Inference *exist = *it;
		res = exist->compareTo(newCandidate);
		if (res == 0) { // Got an equal
			//FENCE_PRINT("Got an equal or stronger candidate, NOT adding!\n");
			return false;
		} else if (res == -1) { // Got a stronger candidate
			//FENCE_PRINT("Got an equal or stronger candidate, NOT adding!\n");
			//return false;
		} else if (res == 1) { // Got a weaker candidate
			//isWeaker = true;
			// But should remove the stronger existing candidate before adding
			//delete exist;
			//it = existCandidates->erase(it);
			//it--;
		} else {/*
			FENCE_PRINT("res: %d\n", res);
			FENCE_PRINT("exist\n");
			printWildcardResult(exist, wildcardNum);
			FENCE_PRINT("newCandidate\n");
			printWildcardResult(newCandidate, wildcardNum);
			FENCE_PRINT(" ***** ------------------- *****\n");
			*/
		}
	}
	if (isWeaker) {
		//FENCE_PRINT("Got a weaker candidate, MUST add!\n");
		/*
		FENCE_PRINT("newCandidate\n");
		printWildcardResult(newCandidate, wildcardNum);
		FENCE_PRINT(" -----*****--------*****------ \n");
		*/
	} else {
		//FENCE_PRINT("Got an uncomparable candidate, MUST add!\n");
	}
	existCandidates->push_back(newCandidate);
	return true;
}

bool SCFence::addMoreCandidates(ModelList<Inference*> *existCandidates, ModelList<Inference*> *newCandidates) {
	bool added = false;
	ModelList<Inference*>::iterator it;
	for (it = newCandidates->begin(); it != newCandidates->end(); it++) {
		Inference *newCandidate = *it;
		added = addMoreCandidate(existCandidates, newCandidate);
		if (added) {
			added = true;
		} else {
			delete newCandidate;
			it = newCandidates->erase(it);
			it--;
		}
	}

	return added;
}


void SCFence::addPotentialFixes(action_list_t *list) {
	for (action_list_t::iterator it = list->begin(); it != list->end(); it++) {
		sync_paths_t *paths1 = NULL, *paths2 = NULL;
		ModelList<Inference*> *candidates = NULL;
		const ModelAction *unInitRead = NULL,
			*act = *it;
		if (act->get_seq_number() > 0) {
			if (badrfset.contains(act)) {
				const ModelAction *write = act->get_reads_from();
					
				if (write->get_seq_number() == 0) { // Uninitialzed read
					unInitRead = act;
					action_list_t::iterator itWrite = it;
					itWrite++;
					bool solveUninit = false;
					for (; itWrite != list->end(); itWrite++) {
						const ModelAction *theWrite = *itWrite;
						if (theWrite->get_location() !=
							unInitRead->get_location()) 
							continue;
						FENCE_PRINT("Running through pattern (b') (unint read)!\n");

						// Try to find path from theWrite to the act 
						paths1 = get_rf_sb_paths(theWrite, act);
						if (paths1->size() > 0) {
							FENCE_PRINT("From some write to uninit read: \n");
							print_rf_sb_paths(paths1, theWrite, act);
							candidates = imposeSync(NULL, paths1);
							model_print("candidates size: %d.\n", candidates->size());
							//potentialResults->insert(potentialResults->end(),
							//	candidates->begin(), candidates->end());
							solveUninit = true;
						} else {
							FENCE_PRINT("Might not be able to impose sync to solve the uninit \n");
							//ACT_PRINT(act);
							//ACT_PRINT(write);
						}
					}
					if (solveUninit)
						break;
				}

				// Check reading old or future value
				bool readOldVal = false;
				for (action_list_t::iterator it1 = list->begin(); it1 !=
					list->end(); it1++) {
					ModelAction *act1 = *it1;
					if (act1 == act)
						break;
					if (act1 == write) {
						readOldVal = true;
						break;
					}
				}

				if (readOldVal) { // Pattern (a) read old value
					FENCE_PRINT("Running through pattern (a)!\n");
					
					// Find all writes between the write1 and the read
					action_list_t *write2s = new action_list_t();
					ModelAction *write2;
					action_list_t::iterator findIt = find(list->begin(), list->end(), write);
					findIt++;
					do {
						write2 = *findIt;
						if (write2->is_write() && write2->get_location() ==
							write->get_location()) {
							write2s->push_back(write2);
						}
						findIt++;
					} while (write2 != act);
					FENCE_PRINT("write2s set size: %d\n", write2s->size());
					for (action_list_t::iterator itWrite2 = write2s->begin();
						itWrite2 != write2s->end(); itWrite2++) {
						candidates = NULL;
						FENCE_PRINT("write2:\n");
						ACT_PRINT(write2);
						write2 = *itWrite2;
						// act->read, write->write1 & write2->write2
						if (!isSCEdge(write, write2) &&
							!write->happens_before(write2)) {
							paths1 = get_rf_sb_paths(write, write2);
							if (paths1->size() > 0) {
								FENCE_PRINT("From write1 to write2: \n");
								print_rf_sb_paths(paths1, write, write2);
								candidates = imposeSync(NULL, paths1);
							} else {
								FENCE_PRINT("Have to impose sc on write1 & write2: \n");
								ACT_PRINT(write);
								ACT_PRINT(write2);
								candidates = imposeSC(NULL, write, write2);
							}
						} else {
							FENCE_PRINT("write1 mo before write2. \n");
						}

						if (!isSCEdge(write2, act) &&
							!write2->happens_before(act)) {
							paths2 = get_rf_sb_paths(write2, act);
							if (paths2->size() > 0) {
								FENCE_PRINT("From write2 to read: \n");
								print_rf_sb_paths(paths2, write2, act);
								//FENCE_PRINT("paths2 size: %d\n", paths2->size());
								if (candidates == NULL) {
									candidates = imposeSync(NULL, paths2);
								} else {
									candidates = imposeSync(candidates, paths2);
								}
							} else {
								FENCE_PRINT("Have to impose sc on write2 & read: \n");
								ACT_PRINT(write2);
								ACT_PRINT(act);
								if (candidates == NULL) {
									candidates = imposeSC(NULL, write2, act);
								} else {
									candidates = imposeSC(candidates, write2, act);
								}
							}
						} else {
							FENCE_PRINT("write2 hb/sc before read. \n");
						}
						
						// Add candidates to potentialResults list for current
						// write2
						model_print("candidates size: %d.\n", candidates->size());
						//printWildcardResults(candidates);
						addMoreCandidates(potentialResults, candidates);
						model_print("potential results size: %d.\n", potentialResults->size());
						//printWildcardResults(potentialResults);
						delete candidates;
					}
				} else { // Pattern (b) read future value
					// act->read, write->futureWrite
					FENCE_PRINT("Running through pattern (b)!\n");
					// Fixing one direction (read -> futureWrite)
					paths1 = get_rf_sb_paths(act, write);
					paths2 = get_rf_sb_paths(write, act);
					if (paths1->size() > 0 || paths2->size() > 0) {
						if (paths1->size() > 0) {
							// Fixing one direction (read -> futureWrite )
							FENCE_PRINT("From read to future write: \n");
							print_rf_sb_paths(paths1, act, write);
							candidates = imposeSync(NULL, paths1);
							model_print("candidates size: %d.\n", candidates->size());
							model_print("potential results size: %d.\n", potentialResults->size());
							printWildcardResults(potentialResults);
							//potentialResults->insert(potentialResults->end(),
							//	candidates->begin(), candidates->end());
							addMoreCandidates(potentialResults, candidates);
							delete candidates;
						}
						if (paths2->size() > 0) {
							candidates = NULL;
							FENCE_PRINT("paths2 size in pattern(b) : %d\n",
								paths2->size());
							if (candidates != NULL)
								delete candidates;
							// Fixing the other direction (futureWrite -> read)
							FENCE_PRINT("From future write to read: \n");
							print_rf_sb_paths(paths2, write, act);
							candidates = imposeSync(NULL, paths2);
							model_print("candidates size: %d.\n", candidates->size());
							addMoreCandidates(potentialResults, candidates);
							delete candidates;
						}
					} else {
						FENCE_PRINT("Have to impose sc on read and future write: \n");
						ACT_PRINT(act);
						ACT_PRINT(write);
						/*
						FENCE_PRINT("write wildcard: %d\n",
							get_wildcard_id(write->get_original_mo()));
						FENCE_PRINT("read wildcard: %d\n",
							get_wildcard_id(act->get_original_mo()));

						*/
						candidates = imposeSC(NULL, act, write);
						addMoreCandidates(potentialResults, candidates);
						delete candidates;
					}
				}
				// Just eliminate the first cycle we see in the execution
				break;
			}
		}
	}
}


void SCFence::printWildcardResults(ModelList<Inference*> *results) {
	for (ModelList<Inference*>::iterator it = results->begin(); it !=
		results->end(); it++) {
		int idx = distance(results->begin(), it) + 1;
		model_print("Result %d:\n", idx);
		(*it)->print();
	}
}

/** Return false to indicate there's no implied mo for this execution */
bool SCFence::addFixesImplicitMO(action_list_t *list) {
	ModelList<Inference*> *candidates = NULL;
	for (action_list_t::reverse_iterator rit2 = list->rbegin(); rit2 !=
		list->rend(); rit2++) {
		ModelAction *write2 = *rit2;
		if (!write2->is_write())
			continue;
		action_list_t::reverse_iterator rit1 = rit2;
		rit1++;
		for (; rit1 != list->rend(); rit1++) {
			ModelAction *write1 = *rit1;
			if (!write1->is_write() || write1->get_location() !=
				write2->get_location())
				continue;
			int readCnt = 0;
			action_list_t::reverse_iterator ritRead = rit2;
			ritRead++;
			for (; ritRead != rit1; ritRead++) {
				ModelAction *read = *ritRead;
				if (!read->is_read() || read->get_location() !=
					write1->get_location())
					continue;
				readCnt++;
			}
			if (readCnt > implicitMOReadBound) {
				// Found it, make write1 --hb-> write2
				FENCE_PRINT("Running through pattern (c) -- implicit mo!\n");
				FENCE_PRINT("Read count between the two writes: %d\n", readCnt);
				FENCE_PRINT("implicitMOReadBound: %d\n", implicitMOReadBound);
				WILDCARD_ACT_PRINT(write1);
				WILDCARD_ACT_PRINT(write2);
				sync_paths_t *paths1 = get_rf_sb_paths(write1, write2);
				if (paths1->size() > 0) {
					FENCE_PRINT("From write1 to write2: \n");
					print_rf_sb_paths(paths1, write1, write2);
					ModelList<Inference*> *curCandidates = imposeSync(NULL, paths1);
					candidates = imposeSync(candidates, paths1);
					model_print("potential results size: %d.\n", potentialResults->size());
					// Add the curCandidates as potential results
					for (ModelList<Inference*>::iterator it =
						curCandidates->begin(); it != curCandidates->end(); it++) {
						Inference *curCand = *it;
						// The candidate won't work
						int compVal = curCand->compareTo(curInference);
						if (compVal == 0)
							continue;
						if (addMoreCandidate(potentialResults, curCand)) {
							model_print("curCand\n");
							curCand->print();
						} else {
							delete curCand;
						}
					}
					delete curCandidates;
					model_print("potential results size: %d.\n", potentialResults->size());

					// Add the candidates as potential results
					for (ModelList<Inference*>::iterator it =
						candidates->begin(); it != candidates->end(); it++) {
						Inference *cand = *it;
						// The candidate won't work
						int compVal = cand->compareTo(curInference);
						if (compVal == 0)
							continue;
						if (addMoreCandidate(potentialResults, cand)) {
							model_print("cand\n");
							cand->print();
						}
					}
					model_print("potential results size: %d.\n", potentialResults->size());
/*
					model_print("potential results size: %d.\n", potentialResults->size());
					model_print("candidates size: %d.\n", candidates->size());
					printWildcardResults(candidates);
					potentialResults->insert(potentialResults->end(),
						candidates->begin(), candidates->end());
					model_print("potential results size: %d.\n", potentialResults->size());
					delete candidates;
					return true;
*/

				} else {
					FENCE_PRINT("Cannot establish hb between write1 & write2: \n");
					ACT_PRINT(write1);
					ACT_PRINT(write2);
				}
			}
			break;
		}
		//if (candidates != NULL)
			//break;
		// Find other pairs of writes
	}
	if (candidates == NULL)
		return false;
 	if (candidates->size() == 0)
		delete candidates;
	return true;
}

bool SCFence::getNextCandidate() {
	while (true) {
		Inference *candidate = potentialResults->front();
		if (candidate == NULL) {
			// We are unable to find any inferences
			return false;
		}
		//printWildcardResult(candidate, wildcardNum);
		potentialResults->pop_front();
		model_print("potential results size: %d.\n", potentialResults->size());
		int compVal = candidate->compareTo(results);
		// Only try those candidates weaker than the results
		if (compVal == 0 || compVal == 1) {
			delete candidate;
			continue;
		}
		// Clear the current inference before over-writing
		delete curInference;
		curInference = candidate;
		curInference->print();
		return true;
	}
}


void SCFence::analyze(action_list_t *actions) {
	struct timeval start;
	struct timeval finish;
	if (time)
		gettimeofday(&start, NULL);
	
	/* Build up the thread lists for general purpose */
	int thrdNum;
	buildVectors(&dup_threadlists, &thrdNum, actions);
	action_list_t *list = generateSC(actions);
	
	check_rf(list);
	if (print_always || (print_buggy && execution->have_bug_reports())|| (print_nonsc && cyclic))
		print_list(list);
	if (time) {
		gettimeofday(&finish, NULL);
		stats->elapsedtime+=((finish.tv_sec*1000000+finish.tv_usec)-(start.tv_sec*1000000+start.tv_usec));
	}
	update_stats();

	// Now we find a non-SC execution
	if (cyclic) {
		print_list(list);
		addPotentialFixes(list);
		if (!getNextCandidate()) {
			model_print("Maybe you should have more wildcards parameters for us to infer!\n");
		}
		/*
		Inference *candidate = potentialResults->front();
		if (candidate == NULL) {
			// We are unable to find any inferences
			model_print("Maybe you should have more wildcards parameters for us to infer!\n");
			return;
		}
		//printWildcardResult(candidate, wildcardNum);
		potentialResults->pop_front();
		// Clear the current inference before over-writing
		delete curInference;
		curInference = candidate;
		*/
		model->restart();
	} else {
		// Also check if we should imply the implicit modification order
		/*
		model_print("inferImplictMO: %d\n", inferImplicitMO);
		model_print("too_many_steps: %d\n",
			execution->too_many_steps());
		model_print("maxreads & implicitReadBound: %d & %d\n",
		    model->params.maxreads, implicitMOReadBound);
		*/
		if (inferImplicitMO && execution->too_many_steps() &&
			model->params.maxreads < implicitMOReadBound) {
			print_list(list);
			bool infered = addFixesImplicitMO(list);
			if (infered) {
				FENCE_PRINT("Found an implicit mo pattern to fix!\n");
				if (!getNextCandidate()) {
					model_print("Maybe you should have more wildcards parameters for us to infer!\n");
				}
				/*
				Inference *candidate = potentialResults->front();
				potentialResults->pop_front();
				// Clear the current inference before over-writing
				delete curInference;
				curInference = candidate;*/
				model->restart();
			}
		}
	}
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

sync_paths_t * SCFence::get_rf_sb_paths(const ModelAction *act1, const ModelAction *act2) {
	int idx1 = id_to_int(act1->get_tid()),
		idx2 = id_to_int(act2->get_tid());
	action_list_t *list1 = &dup_threadlists[idx1],
		*list2 = &dup_threadlists[idx2];
	action_list_t::iterator it1 = list1->begin();
	// First action of the thread where act1 belongs
	ModelAction *start = *it1;
	
	// The container for all possible results
	sync_paths_t *paths = new sync_paths_t();
	// A stack that records all current possible paths
	sync_paths_t *stack = new sync_paths_t();
	action_list_t *path;
	// Initial stack with loads sb-ordered before act2
	for (action_list_t::iterator it2 = list2->begin(); it2 != list2->end(); it2++) {
		ModelAction *act = *it2;
		if (act->get_seq_number() > act2->get_seq_number())
			continue;
		if (!act->is_read())
			continue;
		//FENCE_PRINT("init read:\n");
		//ACT_PRINT(act);

		path = new action_list_t();
		path->push_front(act);
		stack->push_back(path);
	}
	while (stack->size() > 0) {
		path = stack->back();
		stack->pop_back();
		ModelAction *read = path->front();
		//FENCE_PRINT("Temporary path size: %d\n", path->size());
		const ModelAction *write = read->get_reads_from();
		/** In case of cyclic sbUrf & for the purpose of redundant path, make
		 * sure the write appears in a new thread
		 */
		bool atNewThrd = true;
		for (action_list_t::iterator p_it = path->begin(); p_it != path->end();
			p_it++) {
			ModelAction *prev_read = *p_it;
			if (id_to_int(write->get_tid()) == id_to_int(prev_read->get_tid())) {
				//FENCE_PRINT("Reaching previous read thread, bail!\n");
				path->clear();
				//delete path;
				atNewThrd = false;
				break;
			}
		}
		if (!atNewThrd) {
			continue;
		}

		//FENCE_PRINT("Temporary read & write:\n");
		//ACT_PRINT(read);
		//ACT_PRINT(write);
		
		int write_seqnum = write->get_seq_number();
		if (id_to_int(write->get_tid()) == idx1) {
			if (write_seqnum >= act1->get_seq_number()) { // Find a path
				//FENCE_PRINT("Find a path.\n");
				paths->push_back(path);
				continue;
			} else { // Not a rfUsb path
				//FENCE_PRINT("Not a path.\n");
				path->clear();
				continue;
			}
		}
		int idx = id_to_int(write->get_tid());
		action_list_t *list = &dup_threadlists[idx];
		for (action_list_t::iterator it = list->begin(); it != list->end(); it++) {
			ModelAction *act = *it;
			if (act->get_seq_number() > write_seqnum)
				continue;
			if (!act->is_read())
				continue;
			action_list_t *new_path = new action_list_t(*path);
			new_path->push_front(act);
			stack->push_back(new_path);
		}
		path->clear();
	}

	return paths;
}

void SCFence::print_rf_sb_paths(sync_paths_t *paths, const ModelAction *start, const ModelAction *end) {
	FENCE_PRINT("Starting from:\n");
	WILDCARD_ACT_PRINT(start);
	for (sync_paths_t::iterator paths_i = paths->begin(); paths_i !=
		paths->end(); paths_i++) {
		FENCE_PRINT("Path %d:\n", distance(paths->begin(), paths_i));
		action_list_t *path = *paths_i;
		action_list_t::iterator it = path->begin(), i_next;
		for (; it != path->end(); it++) {
			i_next = it;
			i_next++;
			const ModelAction *read = *it,
				*write = read->get_reads_from(),
				*next_read = (i_next != path->end()) ? *i_next : NULL;
			WILDCARD_ACT_PRINT(write);
			if (next_read == NULL || next_read->get_reads_from() != read) {
				// Not the same RMW, also print the read operation
				WILDCARD_ACT_PRINT(read);/*
				model_print("Right here!\n");
				model_print("wildcard: %d\n",
					get_wildcard_id(read->get_original_mo()));*/

			}
		}
	}

	FENCE_PRINT("Ending with:\n");
	WILDCARD_ACT_PRINT(end);
}

bool SCFence::merge(ClockVector *cv, const ModelAction *act, const ModelAction *act2) {
	ClockVector *cv2 = cvmap.get(act2);
	if (cv2 == NULL)
		return true;

	if (cv2->getClock(act->get_tid()) >= act->get_seq_number() && act->get_seq_number() != 0) {
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

	/* We should honor the SC & HB in order to propogate the fixes to other
	 * problematic spots by adding the sc&hb edges before the implied SC edges
	 */
	for (action_list_t::iterator it1 = list->begin(); it1 != list->end(); it1++) {
		ModelAction *act1 = *it1, *act2;
		action_list_t::iterator it2 = it1;
		it2++;
		for (; it2 != list->end(); it2++) {
			act2 = *it2;
			if (act1->happens_before(act2)) {
				ClockVector *cv = cvmap.get(act2);
				if (cv == NULL) {
					cv = new ClockVector(NULL, act2);
					cvmap.put(act2, cv);
				}
				// Add the hb edge to the clock vector
				merge(cv, act2, act1);
				continue;
			}
			// FIXME: how to get the SC edges, only adding edges for those
			// non-conflicting operations
			if (!isSCEdge(act1, act2) || !isConflicting(act1, act2))
				continue;
			// This is an SC edge
			ClockVector *cv = cvmap.get(act2);
			if (cv == NULL) {
				cv = new ClockVector(NULL, act2);
				cvmap.put(act2, cv);
			}
			// Add the SC edge to the clock vector
			merge(cv, act2, act1);
		}

	}

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
