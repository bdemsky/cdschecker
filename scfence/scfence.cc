#include "scfence.h"
#include "action.h"
#include "threads-model.h"
#include "clockvector.h"
#include "execution.h"
#include "cyclegraph.h"
#include <sys/time.h>

#include "model.h"
#include "wildcard.h"
#include "inference.h"
#include "inferset.h"
#include "sc_annotation.h"
#include <stdio.h>
#include <algorithm>

scfence_priv *SCFence::priv;

SCFence::SCFence() :
	cvmap(),
	cyclic(false),
	badrfset(),
	lastwrmap(),
	threadlists(1),
	dup_threadlists(1),
	execution(NULL),
	print_always(false),
	print_buggy(false),
	print_nonsc(false),
	time(false),
	stats((struct sc_statistics *)model_calloc(1, sizeof(struct sc_statistics)))
{
	priv = new scfence_priv();
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


/******************** SCFence-related Functions (Beginning) ********************/

void SCFence::inspectModelAction(ModelAction *act) {
	if (act == NULL) // Get pass cases like data race detector
		return;
	if (act->get_mo() >= memory_order_relaxed && act->get_mo() <=
		memory_order_seq_cst) {
		return;
	} else { // For wildcards
		Inference *curInfer = getCurInference();
		int wildcardID = get_wildcard_id(act->get_mo());
		memory_order order = (*curInfer)[wildcardID];
		if (order == WILDCARD_NONEXIST) {
			(*curInfer)[wildcardID] = memory_order_relaxed;
			act->set_mo(memory_order_relaxed);
		} else {
			act->set_mo((*curInfer)[wildcardID]);
		}
	}
}


void SCFence::actionAtInstallation() {
	// When this pluggin gets installed, it become the inspect_plugin
	model->set_inspect_plugin(this);
}

static bool isTheInference(Inference *infer) {
	for (int i = 0; i < infer->getSize(); i++) {
		memory_order mo1 = (*infer)[i], mo2;
		if (mo1 == WILDCARD_NONEXIST)
			mo1 = relaxed;
		switch (i) {
			case 3:
				mo2 = acquire;
			break;
			case 11:
				mo2 = release;
			break;
			default:
				mo2 = relaxed;
			break;
		}
		if (mo1 != mo2)
			return false;
	}
	return true;
}

void SCFence::exitModelChecker() {
	model->exit_model_checker();
}

void SCFence::restartModelChecker() {
	model->restart();
	if (!getHasRestarted())
		setHasRestarted(true);
}

bool SCFence::modelCheckerAtExitState() {
	return model->get_exit_flag();
}

void SCFence::actionAtModelCheckingFinish() {
	// Just bail if the model checker is at the exit state
	if (modelCheckerAtExitState())
		return;

	/** Backtrack with a successful inference */
	routineBacktrack(true);

	if (time)
		model_print("Elapsed time in usec %llu\n", stats->elapsedtime);
	model_print("SC count: %u\n", stats->sccount);
	model_print("Non-SC count: %u\n", stats->nonsccount);
	model_print("Total actions: %llu\n", stats->actions);
	//unsigned long long actionperexec=(stats->actions)/(stats->sccount+stats->nonsccount);
}

void SCFence::initializeByFile() {
	FILE *fp = fopen(getCandidateFile(), "r");
	if (fp == NULL) {
		fprintf(stderr, "Cannot open the file!\n");
		return;
	}
	Inference *infer = NULL;
	int curNum = 0;
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

			/******** addInference ********/
			if (!addInference(infer))
				delete infer;
		}
	}
	fclose(fp);

	FENCE_PRINT("candidate size from file: %d\n", setSize());
	Inference *next = getNextInference();
	if (!next) {
		model_print("Wrong with the candidate file\n");
	} else {
		setCurInference(next);
	}
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
				setCandidateFile(val);
				//model_print("f value: %s\n", val);
				if (strcmp(val, "") == 0) {
					model_print("Need to specify a file that contains initial inference!\n");
					return true;
				}
				break;
			case 'b': // The bound above 
				//model_print("Parsing b option!\n");
				//model_print("b value: %s\n", val);
				setImplicitMOReadBound(atoi(val));
				if (getImplicitMOReadBound() <= 0) {
					model_print("Enter valid bound value!\n");
					return true;
				}
				break;
			case 'm': // Infer the modification order from repetitive reads from
					  // the same write
				//model_print("Parsing m option!\n");
				//model_print("m value: %s\n", val);
				setInferImplicitMO(true);
				if (strcmp(val, "") != 0) {
					model_print("option m doesn't take any arguments!\n");
					return true;
				}
				break;
			case 'a': // Turn on the annotation mode 
				//model_print("Parsing a option!\n");
				annotationMode = true;
				if (strcmp(val, "") != 0) {
					model_print("option a doesn't take any arguments!\n");
					return true;
				}
				break;
			case 't': // The timeout set to force the analysis to backtrack
				model_print("Parsing t option!\n");
				model_print("t value: %s\n", val);
				setTimeout(atoi(val));
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
		if (getCandidateFile() != NULL)
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

/******************** SCFence-related Functions (End) ********************/



/******************** SCAnalysis Functions (Beginning) ********************/

/** Check whether a chosen reads-from path is a release sequence */
bool SCFence::isReleaseSequence(path_t *path) {
	ASSERT (path);
	path_t::reverse_iterator rit = path->rbegin(),
		rit_next;
	const ModelAction *read,
		*write,
		*next_read;
	for (; rit != path->rend(); rit++) {
		read = *rit;
		rit_next = rit;
		rit_next++;
		write = read->get_reads_from();
		if (rit_next != path->rend()) {
			next_read = *rit_next;
			if (write != next_read) {
				return false;
			}
		}
	}
	return true;
}

/** Only return those applicable patches in a vector */
SnapVector<Patch*>* SCFence::getAcqRel(const ModelAction *read, const
	ModelAction *readBound,const ModelAction *write, const ModelAction *writeBound) {
	
	SnapVector<Patch*> *patches = new SnapVector<Patch*>;
	Patch *p = new Patch(read, memory_order_acquire, write,
		memory_order_release);
	if (p->isApplicable())
		patches->push_back(p);

	/* Also support rel/acq fences synchronization here */
	// Look for the acq fence after the read action
	int readThrdID = read->get_tid(),
		writeThrdID = write->get_tid();
	action_list_t *readThrd = &dup_threadlists[readThrdID],
		*writeThrd = &dup_threadlists[writeThrdID];
	action_list_t::iterator readIter = std::find(readThrd->begin(),
		readThrd->end(), read);
	action_list_t::reverse_iterator writeIter = std::find(writeThrd->rbegin(),
		writeThrd->rend(), write);

	action_list_t *acqFences = new action_list_t,
		*relFences = new action_list_t;
	ModelAction *act = *readIter;
	while (readIter++ != readThrd->end() && act != readBound) {
		if (act->is_fence()) {
			acqFences->push_back(act);
		}
		act = *readIter;
	}
	act = *writeIter;
	while (writeIter++ != writeThrd->rend() && act != writeBound) {
		if (act->is_fence()) {
			relFences->push_back(act);
		}
		act = *writeIter;
	}
	// Now we have a list of rel/acq fences at hand
	int acqFenceSize = acqFences->size(),
		relFenceSize = relFences->size();
	if (acqFenceSize == 0 && relFenceSize == 0)
		return patches;
	else if (acqFenceSize > 0 && relFenceSize == 0) {
		for (action_list_t::iterator it = acqFences->begin(); it !=
			acqFences->end(); it++) {
			ModelAction *fence = *it;
			p = new Patch(fence, memory_order_acquire, write, memory_order_release);
			if (p->isApplicable())
				patches->push_back(p);
		}
	} else if (acqFenceSize == 0 && relFenceSize > 0) {
		for (action_list_t::iterator it = relFences->begin(); it !=
			relFences->end(); it++) {
			ModelAction *fence = *it;
			p = new Patch(fence, memory_order_release, read, memory_order_acquire);
			if (p->isApplicable())
				patches->push_back(p);
		}
	} else {
		/* Only impose on the acqFences */
		for (action_list_t::iterator it = acqFences->begin(); it !=
			acqFences->end(); it++) {
			ModelAction *fence = *it;
			p = new Patch(fence, memory_order_acquire, write, memory_order_release);
			if (p->isApplicable())
				patches->push_back(p);
		}
		/* Only impose on the relFences */
		for (action_list_t::iterator it = relFences->begin(); it !=
			relFences->end(); it++) {
			ModelAction *fence = *it;
			p = new Patch(fence, memory_order_release, read, memory_order_acquire);
			if (p->isApplicable())
				patches->push_back(p);
		}
		/* Impose on both relFences and acqFences */
		for (action_list_t::iterator it1 = relFences->begin(); it1 !=
			relFences->end(); it1++) {
			ModelAction *relFence = *it1;
			for (action_list_t::iterator it2 = acqFences->begin(); it2 !=
				acqFences->end(); it2++) {
				ModelAction *acqFence = *it2;
				p = new Patch(relFence, memory_order_release, acqFence, memory_order_acquire);
				if (p->isApplicable())
					patches->push_back(p);
			}
		}
	}
	return patches;
}

/** Impose the synchronization between the begin and end action, and the paths
 *  are a list of paths that each represent the union of rfUsb. We then
 *  strengthen the current inference by necessity.
 */
bool SCFence::imposeSync(InferenceList *inferList,
	paths_t *paths, const ModelAction *begin, const ModelAction *end) {
	if (!inferList)
		return false;
	bool res = false;
	for (paths_t::iterator i_paths = paths->begin(); i_paths != paths->end(); i_paths++) {
		// Iterator over all the possible paths
		path_t *path = *i_paths;
		InferenceList *cands = new InferenceList;
		// Impose synchronization by path
		if (imposeSync(cands, path, begin, end))
			res = true;
		inferList->append(cands);
	}
	return res;
}

/** Impose the synchronization between the begin and end action, and the path
 *  is the union of rfUsb. We then strengthen the current inference by
 *  necessity.
 */
bool SCFence::imposeSync(InferenceList *inferList,
	path_t *path, const ModelAction *begin, const ModelAction *end) {
	
	bool release_seq = isReleaseSequence(path);
	SnapVector<Patch*> *patches;
	if (release_seq) {
		const ModelAction *relHead = path->front()->get_reads_from(),
			*lastRead = path->back();
		patches = getAcqRel(lastRead, end, relHead, begin);
		if (patches->size() == 0)
			return false;
		inferList->applyPatch(getCurInference(), patches);
		delete patches;
		// Bail now for the release sequence because 
		return true;
	}

	for (path_t::iterator it = path->begin(); it != path->end(); it++) {
		const ModelAction *read = *it,
			*write = read->get_reads_from(),
			*prevRead, *nextRead;
		
		const ModelAction *readBound = NULL,
			*writeBound = NULL;
		if (it == path->begin()) {
			prevRead = NULL;
		}
		nextRead = *++it;
		if (it == path->end()) {
			nextRead = NULL;
		}
		it--;
		if (prevRead) {
			readBound = prevRead->get_reads_from();
		} else {
			readBound = end;
		}
		if (nextRead) {
			writeBound = nextRead;
		} else {
			writeBound = begin;
		}

		/* Extend to support rel/acq fences synchronization here */
		patches = getAcqRel(read, readBound, write, writeBound);

		//FENCE_PRINT("path size:%d\n", path->size());
		//ACT_PRINT(write);
		//ACT_PRINT(read);
		if (patches->size() == 0) {
			// We cannot strengthen the inference
			return false;
		}

		inferList->applyPatch(getCurInference(), patches);
		delete patches;
	}
	return true;
}

/** Impose SC orderings to related actions (including fences) such that we
 *  either want to establish mo between act1 & act2 (act1 -mo-> act2) when they
 *  can't establish hb; or that act2 can't read from any actions that are
 *  modification ordered before act1. All these requirements are consistent with
 *  the following strengthening strategy:
 *  1. make both act1 and act2 SC
 *  2. if there are fences in between act1 & act2, and the fences are either in
 *  the threads of either act1 or act2, we can impose SC on the fences or
 *  corresponding actions. 
 */
bool SCFence::imposeSC(action_list_t * actions, InferenceList *inferList, const ModelAction *act1, const ModelAction *act2) {
	if (!inferList) {
		return false;
	}
	int act1ThrdID = act1->get_tid(),
		act2ThrdID = act2->get_tid();
	action_list_t::iterator act1Iter = std::find(actions->begin(),
		actions->end(), act1);
	action_list_t::iterator act2Iter = std::find(act1Iter,
		actions->end(), act2);

	action_list_t::iterator it = act1Iter;
	it++;
	action_list_t *fences = new action_list_t;
	int size = 0;
	while (it != act2Iter) {
		ModelAction *fence = *it;
		it++;
		if (!fence->is_fence())
			continue;
		if (!is_wildcard(fence->get_original_mo()))
			continue;
		fences->push_back(fence);
		size++;
	}

	Patch *p;
	SnapVector<Patch*> *patches = new SnapVector<Patch*>;
	model_print("fences size %d\n", size);
	// Just impose SC on one fence
	for (action_list_t::iterator fit = fences->begin(); fit != fences->end();
		fit++) {
		ModelAction *fence = *fit;
		p = new Patch(act1, memory_order_seq_cst, fence, memory_order_seq_cst);
		if (p->isApplicable()) {
			p->print();
			patches->push_back(p);
		}
		p = new Patch(act2, memory_order_seq_cst, fence, memory_order_seq_cst);
		if (p->isApplicable()) {
			p->print();
			patches->push_back(p);
		}
	}
	
	// Mmpose SC on two fences
	for (action_list_t::iterator fit1 = fences->begin(); fit1 != fences->end();
		fit1++) {
		ModelAction *fence1 = *fit1;
		action_list_t::iterator fit2 = fit1;
		fit2++;
		for (; fit2 != fences->end(); fit2++) {
			ModelAction *fence2 = *fit2;
			p = new Patch(fence1, memory_order_seq_cst, fence2, memory_order_seq_cst);
			if (p->isApplicable()) {
				p->print();
				patches->push_back(p);
			}
		}
	}

	p = new Patch(act1, memory_order_seq_cst, act2, memory_order_seq_cst);
	if (p->isApplicable()) {
		p->print();
		patches->push_back(p);
	}
	
	if (patches->size() > 0) {
		inferList->applyPatch(getCurInference(), patches);
		return true;
	}
	return false;
}

/** A subroutine that calculates the potential fixes for the non-sc pattern (a),
 *  a.k.a old value reading. The whole idea is as follows.
 *	write -isc-> write2 && write2 -isc->read && write -rf-> read!!!
 *	The fix can be two-step:
 *	a. make write -mo-> write2, and we can accomplish this by imposing hb
 *	between write and write2, and if not possible, make write -sc-> write2
 *	b. make write2 -hb-> read, and if not possible, make write2 -sc-> read.
 */
InferenceList* SCFence::getFixesFromPatternA(action_list_t *list, action_list_t::iterator readIter, action_list_t::iterator writeIter) {
	ModelAction *read = *readIter,
		*write = *writeIter;

	InferenceList *candidates = new InferenceList;
	paths_t *paths1, *paths2;

	// Find all writes between the write1 and the read
	action_list_t *write2s = new action_list_t();
	ModelAction *write2;
	action_list_t::iterator findIt = writeIter;
	findIt++;
	do {
		write2 = *findIt;
		if (write2->is_write() && write2->get_location() ==
			write->get_location()) {
			write2s->push_back(write2);
		}
		findIt++;
	} while (findIt != readIter);
					
	// Found all the possible write2s
	FENCE_PRINT("write2s set size: %d\n", write2s->size());
	for (action_list_t::iterator itWrite2 = write2s->begin();
		itWrite2 != write2s->end(); itWrite2++) {
		InferenceList *tmpCandidates = new InferenceList;
		write2 = *itWrite2;
		FENCE_PRINT("write2:\n");
		ACT_PRINT(write2);
		// write1->write2 (write->write2)
		// Whether write -mo-> write2
		if (!mo_graph->checkReachable(write, write2)) {
			paths1 = get_rf_sb_paths(write, write2);
			if (paths1->size() > 0) {
				FENCE_PRINT("From write1 to write2: \n");
				print_rf_sb_paths(paths1, write, write2);
				// FIXME: Need to make sure at least one path is feasible; what
				// if we got empty candidates here, maybe should then impose SC,
				// same in the write2->read
				imposeSync(tmpCandidates, paths1, write, write2);
			} else {
				FENCE_PRINT("Have to impose sc on write1 & write2: \n");
				ACT_PRINT(write);
				ACT_PRINT(write2);
				imposeSC(list, tmpCandidates, write, write2);
			}
		} else {
			FENCE_PRINT("write1 mo before write2. \n");
		}

		// write2->read (write2->read)
		// now we only need to make write2 -hb-> read 
		if (!write2->happens_before(read)) {
			paths2 = get_rf_sb_paths(write2, read);
			if (paths2->size() > 0) {
				FENCE_PRINT("From write2 to read: \n");
				print_rf_sb_paths(paths2, write2, read);
				//FENCE_PRINT("paths2 size: %d\n", paths2->size());
				imposeSync(tmpCandidates, paths2, write2, read);
			} else {
				FENCE_PRINT("Have to impose sc on write2 & read: \n");
				ACT_PRINT(write2);
				ACT_PRINT(read);
				imposeSC(list, tmpCandidates, write2, read);
			}
		} else {
			FENCE_PRINT("write2 hb before read. \n");
		}
		candidates->append(tmpCandidates);
	}
	// Return the complete list of candidates
	return candidates;
}

/** To fix this pattern, we have two options:
 *	1. impose futureWrite -hb-> read so that the SC analysis will order
 *	in a way that the the write is ordered before the read;
 *	2. impose read -hb->futureWrite so that the reads-from edge from futureWrite
 *	to the read is not allowed.
*/
InferenceList* SCFence::getFixesFromPatternB(action_list_t *list, action_list_t::iterator readIter, action_list_t::iterator writeIter) {
	InferenceList *res = new InferenceList,
		*candidates = new InferenceList;
	
	ModelAction *read = *readIter,
		*write = *writeIter;
	// Fixing one direction (read -> futureWrite)
	paths_t *paths1 = get_rf_sb_paths(read, write);
	if (paths1->size() > 0) {
		FENCE_PRINT("From read to future write: \n");
		print_rf_sb_paths(paths1, read, write);
		imposeSync(res, paths1, read, write);
	}

	// Fixing the other direction (futureWrite -> read) for one edge case
	paths_t *paths2 = get_rf_sb_paths(write, read);
	FENCE_PRINT("From future write to read (edge case): \n");
	print_rf_sb_paths(paths2, write, read);
	imposeSync(candidates, paths2, write, read);

	// Append the candidates to the res list
	res->append(candidates);
	return res;
}

bool SCFence::addFixesNonSC(action_list_t *list) {
	bool added = false;
	for (action_list_t::iterator it = list->begin(); it != list->end(); it++) {
		InferenceList *candidates = NULL;
		ModelAction	*act = *it;

		// Save the iterator of the read and the write
		action_list_t::iterator readIter = it, writeIter;
		if (act->get_seq_number() > 0) {
			// Annotated reads will be ignored
			if (badrfset.contains(act) && !annotatedReadSet.contains(act)) {
				const ModelAction *write = act->get_reads_from();
				// Check reading old or future value
				writeIter = std::find(list->begin(),
					list->end(), write);
				action_list_t::iterator findIt = std::find(list->begin(),
					readIter, write);
				bool readOldVal = findIt != readIter ? true : false;

				if (readOldVal) { // Pattern (a) read old value
					FENCE_PRINT("Running through pattern (a)!\n");
					candidates = getFixesFromPatternA(list, readIter, writeIter);
					// Add candidates pattern (a)
					
					added = addCandidates(candidates);
				} else { // Pattern (b) read future value
					// act->read, write->futureWrite
					FENCE_PRINT("Running through pattern (b)!\n");
					candidates = getFixesFromPatternB(list, readIter, writeIter);
					// Add candidates pattern (b)
					added = addCandidates(candidates);
				}
				// Just eliminate the first cycle we see in the execution
				break;
			}
		}
	}
	return added;
}


/** Return false to indicate there's no fixes for this execution. This can
 * happen for specific reason such as it's a user-specified assertion failure */
bool SCFence::addFixesBuggyExecution(action_list_t *list) {
	InferenceList *candidates = new InferenceList;
	bool foundFix = false;
	bool added = false;
	for (action_list_t::reverse_iterator rit = list->rbegin(); rit !=
		list->rend(); rit++) {
		ModelAction *uninitRead = *rit;
		if (uninitRead->get_seq_number() > 0) {
			if (!uninitRead->is_read() || 
				!uninitRead->get_reads_from()->is_uninitialized())
				continue;
			for (action_list_t::iterator it = list->begin(); it !=
				list->end(); it++) {
				ModelAction *write = *it;
				if (write->same_var(uninitRead)) {
					// Now we can try to impose sync write hb-> uninitRead
					paths_t *paths1 = get_rf_sb_paths(write, uninitRead);
					if (paths1->size() > 0) {
						FENCE_PRINT("Running through pattern (b') (unint read)!\n");
						print_rf_sb_paths(paths1, write, uninitRead);
						imposeSync(candidates, paths1, write, uninitRead);
						added = addCandidates(candidates);
						if (added) {
							foundFix = true;
							break;
						}
					}
				}
			}
		}
		if (foundFix)
			break;
	}

	return added;
}

/** Return false to indicate there's no implied mo for this execution. The idea
 * is that it counts the number of reads in the middle of write1 and write2, if
 * that number exceeds a specific number, then the analysis thinks that the
 * program is stuck in an infinite loop because write1 does not establish proper
 * synchronization with write2 such that the reads can read from write1 for
 * ever. */
bool SCFence::addFixesImplicitMO(action_list_t *list) {
	bool added = false;
	InferenceList *candidates = new InferenceList;
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
			if (readCnt > getImplicitMOReadBound()) {
				// Found it, make write1 --hb-> write2
				bool isMO = execution->get_mo_graph()->checkReachable(write1, write2);
				if (isMO) // Only impose mo when it doesn't have mo impsed
					break;
				//FENCE_PRINT("write1 --mo-> write2?: %d\n", isMO1);
				FENCE_PRINT("Running through pattern (c) -- implicit mo!\n");
				FENCE_PRINT("Read count between the two writes: %d\n", readCnt);
				FENCE_PRINT("implicitMOReadBound: %d\n",
					getImplicitMOReadBound());
				ACT_PRINT(write1);
				ACT_PRINT(write2);
				paths_t *paths1 = get_rf_sb_paths(write1, write2);
				if (paths1->size() > 0) {
					FENCE_PRINT("From write1 to write2: \n");
					print_rf_sb_paths(paths1, write1, write2);
					imposeSync(candidates, paths1, write1, write2);
					// Add the candidates as potential results
					added = addCandidates(candidates);
					if (added)
						return true;
				} else {
					FENCE_PRINT("Cannot establish hb between write1 & write2: \n");
					ACT_PRINT(write1);
					ACT_PRINT(write2);
				}
			}
			break;
		}
	}
	return false;
}

bool SCFence::addFixes(action_list_t *list, fix_type_t type) {
	bool added = false;
	switch(type) {
		case BUGGY_EXECUTION:
			added = addFixesBuggyExecution(list);
			break;
		case IMPLICIT_MO:
			added = addFixesImplicitMO(list);
			break;
		case NON_SC:
			added = addFixesNonSC(list);
			break;
		default:
			break;
	}
	if (added && isBuggy()) {
		// If this is a buggy inference and we have got fixes for it, set the
		// falg
		setHasFixes(true);
	}
	model_print("Added = %d\n", added);
	return added;
}


bool SCFence::routineBacktrack(bool feasible) {
	model_print("Backtrack routine:\n");
	
	/******** commitCurInference ********/
	commitInference(getCurInference(), feasible);
	if (feasible) {
		if (!isBuggy()) {
			model_print("Found one result!\n");
		} else if (!hasFixes()) { // Buggy and have no fixes
			model_print("Found one buggy candidate!\n");
		}
		getCurInference()->print();
	} else {
		model_print("Reach an infeasible inference!\n");
	}

	/******** getNextInference ********/
	Inference *next = getNextInference();

	if (next) {
		/******** setCurInference ********/
		setCurInference(next);
		/******** restartModelChecker ********/
		restartModelChecker();
		return true;
	} else {
		// Finish exploring the whole process
		model_print("We are done with the whole process!\n");
		model_print("The results are as the following:\n");
		printResults();
		printCandidates();
				
		/******** exitModelChecker ********/
		exitModelChecker();

		return false;
	}
}

void SCFence::routineAfterAddFixes() {
	model_print("Add fixes routine begin:\n");
	
	/******** getNextInference ********/
	Inference *next = getNextInference();
	ASSERT (next);

	/******** setCurInference ********/
	setCurInference(next);
	/******** restartModelChecker ********/
	restartModelChecker();
	
	model_print("Add fixes routine end:\n");
	model_print("Restart checking with the follwing inference:\n");
	getCurInference()->print();
	model_print("Checking...\n");
}

void SCFence::analyze(action_list_t *actions) {
	if (getTimeout() > 0 && isTimeout()) {
		model_print("Backtrack because we reached the timeout bound.\n");
		routineBacktrack(false);
		return;
	}

	struct timeval start;
	struct timeval finish;
	if (time)
		gettimeofday(&start, NULL);
	
	/* Build up the thread lists for general purpose */
	int thrdNum;
	buildVectors(&dup_threadlists, &thrdNum, actions);
	
	// First of all check if there's any uninitialzed read bugs
	if (execution->have_bug_reports()) {
		/*
		bool added = addFixes(actions, BUGGY_EXECUTION);
		if (added) {
			model_print("Found a solution for buggy execution!\n");
			routineAfterAddFixes();
			return;
		} else {
			// We can't fix the problem in this execution, but we may not be an
			// SC execution
			model_print("Buggy...\n");
			setBuggy(true);
		}
		*/
		setBuggy(true);
	}

	fastVersion = true;
	action_list_t *list = generateSC(actions);
	printCV(actions);
	if (cyclic) {
		reset(actions);
		delete list;
		fastVersion = false;
		list = generateSC(actions);
	} else if (execution->have_bug_reports()) {
		model_print("Be careful. This execution has bugs and still SC\n");
	}
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
		/******** The Non-SC case (beginning) ********/
		print_list(list);

		bool added = addFixes(list, NON_SC);
		if (added) {
			routineAfterAddFixes();
			return;
		} else { // Couldn't imply anymore, backtrack
			routineBacktrack(false);
			return;
		}
		/******** The Non-SC case (end) ********/
	} else {
		/******** The implicit MO case (beginning) ********/
		if (getInferImplicitMO() && execution->too_many_steps() &&
			!execution->is_complete_execution()) {
			print_list(list);
			bool added = addFixes(list, IMPLICIT_MO);
			if (added) {
				FENCE_PRINT("Found an implicit mo pattern to fix!\n");
				routineAfterAddFixes();
				return;
			} else {
				// This can be a good execution, so we can't do anything,
				// backtrack
				//model_print("Weird!! We cannot infer the mo for infinite reads!\n");
				//model_print("Maybe you should have more wildcards parameters for us to infer!\n");
				//routineBacktrack(false);
				return;
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


/** Extract the actions that should be ignored in the partially SC analysis; it
 * is based on the already built threadlists */
void SCFence::extractIgnoredActions() {
	for (unsigned i = 1; i < threadlists.size(); i++) {
		action_list_t *threadList = &threadlists[i];
		for (action_list_t::iterator it = threadList->begin(); it !=
			threadList->end(); it++) {
			ModelAction *act = *it;
			if (IS_SC_ANNO(act)) {
				// SC anatation, should deal with the SC_BEGIN
				if (!IS_ANNO_BEGIN(act)) {
					model_print("Erroneous usage of the SC annotation!\n");
					model_print("You should have a leading BEGIN annotation.\n");
					return;
				}
				ModelAction *lastAct = NULL;
				do {
					it++;
					act = *it;
					if (IS_SC_ANNO(act)) {
						if (IS_ANNO_END(act)) // Find the corresponding anno end
							break;
						if (IS_ANNO_KEEP(act) && lastAct) {
							// should keep lastAct
							// So simply do nothing
							lastAct = NULL;
						} else { // This is an anno error
							model_print("Erroneous usage of the SC annotation!\n");
							model_print("You should have an END annotation\
							after\
							the BEGIN or have an action to for the KEEP.\n");
							return;
						}
					} else { // Ignore the last action, put it in the ignoredActions
						if (lastAct) {
							lastAct->print();
							ignoredActions.put(lastAct, lastAct);
							ignoredActionSize++;
						}
						lastAct = act;
					}
				} while (true);
			}
		}
	}
}


/** This function finds all the paths that is a union of reads-from &
 * sequence-before relationship between act1 & act2. */
paths_t * SCFence::get_rf_sb_paths(const ModelAction *act1, const ModelAction *act2) {
	int idx1 = id_to_int(act1->get_tid()),
		idx2 = id_to_int(act2->get_tid());
	// Retrieves the two lists of actions of thread1 & thread2
	action_list_t *list1 = &dup_threadlists[idx1],
		*list2 = &dup_threadlists[idx2];
	if (list1->size() == 0 || list2->size() == 0) {
		return new paths_t();
	}

	// The container for all possible results
	paths_t *paths = new paths_t();
	// A stack that records all current possible paths
	paths_t *stack = new paths_t();
	path_t *path;
	// Initialize the stack with loads sb-ordered before act2
	for (action_list_t::iterator it2 = list2->begin(); it2 != list2->end(); it2++) {
		ModelAction *act = *it2;
		// Add read action not sb after the act2 (including act2)
		if (act->get_seq_number() > act2->get_seq_number())
			break;
		if (!act->is_read())
			continue;
		// Each start with a possible path
		path = new path_t();
		path->push_front(act);
		stack->push_back(path);
	}
	while (stack->size() > 0) {
		path = stack->back();
		stack->pop_back();
		// The latest read in the temporary path
		ModelAction *read = path->front();
		const ModelAction *write = read->get_reads_from();
		// If the read is uninitialized, don't do it
		if (write->get_seq_number() == 0) {
			delete path;
			continue;
		}
		/** In case of cyclic sbUrf, make sure the write appears in a new thread
		 * or in an existing thread that is sequence-before the added read
		 * actions
		 */
		bool loop = false;
		for (path_t::iterator p_it = path->begin(); p_it != path->end();
			p_it++) {
			ModelAction *addedRead = *p_it;
			if (id_to_int(write->get_tid()) == id_to_int(addedRead->get_tid())) {
				// In the same thread
				if (write->get_seq_number() >= addedRead->get_seq_number()) {
					// Have a loop
					delete path;
					loop = true;
					break;
				}
			}
		}
		// Not a useful rfUsb path (loop)
		if (loop) {
			continue;
		}

		unsigned write_seqnum = write->get_seq_number();
		// We then check if we got a valid path 
		if (id_to_int(write->get_tid()) == idx1 &&
			write_seqnum >= act1->get_seq_number()) {
			// Find a path
			paths->push_back(path);
			continue;
		}
		// Extend the path with the latest read
		int idx = id_to_int(write->get_tid());
		action_list_t *list = &dup_threadlists[idx];
		for (action_list_t::iterator it = list->begin(); it != list->end(); it++) {
			ModelAction *act = *it;
			if (act->get_seq_number() > write_seqnum) // Could be RMW
				break;
			if (!act->is_read())
				continue;
			path_t *new_path = new path_t(*path);
			new_path->push_front(act);
			stack->push_back(new_path);
		}
		delete path;
	}
	return paths;
}

void SCFence::print_rf_sb_paths(paths_t *paths, const ModelAction *start, const ModelAction *end) {
	FENCE_PRINT("Starting from:\n");
	ACT_PRINT(start);
	FENCE_PRINT("\n");
	for (paths_t::iterator paths_i = paths->begin(); paths_i !=
		paths->end(); paths_i++) {
		path_t *path = *paths_i;
		FENCE_PRINT("Path %d, size (%d):\n", distance(paths->begin(), paths_i),
			path->size());
		path_t::iterator it = path->begin(), i_next;
		for (; it != path->end(); it++) {
			i_next = it;
			i_next++;
			const ModelAction *read = *it,
				*write = read->get_reads_from(),
				*next_read = (i_next != path->end()) ? *i_next : NULL;
			ACT_PRINT(write);
			if (next_read == NULL || next_read->get_reads_from() != read) {
				// Not the same RMW, also print the read operation
				ACT_PRINT(read);
			}
		}
		// Output a linebreak at the end of the path
		FENCE_PRINT("\n");
	}
	FENCE_PRINT("Ending with:\n");
	ACT_PRINT(end);
}

/******************** SCFence-related Functions (End) ********************/

bool SCFence::merge(ClockVector *cv, const ModelAction *act, const ModelAction *act2) {
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
	mo_graph = execution->get_mo_graph();
 	int numactions=buildVectors(&threadlists, &maxthreads, list);
	stats->actions+=numactions;

	// Analyze which actions we should ignore in the partially SC analysis
	//extractIgnoredActions();
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

bool SCFence::processReadFast(ModelAction *read, ClockVector *cv) {
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

bool SCFence::processReadSlow(ModelAction *read, ClockVector *cv, bool *updateFuture) {
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

bool SCFence::processAnnotatedReadSlow(ModelAction *read, ClockVector *cv, bool *updateFuture) {
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
void SCFence::collectAnnotatedReads() {
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


void SCFence::printCV(action_list_t *list) {
	model_print("Printing CVs:\n");
	for (action_list_t::iterator it = list->begin(); it != list->end(); it++) {
		ModelAction *act = *it;
		act->print();
		cvmap.get(act)->print();
	}
}

void SCFence::computeCV(action_list_t *list) {
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
			if (cv == NULL) {
				cv = new ClockVector(act->get_cv(), act);
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


/******************** SCAnalysis Functions (End) ********************/
