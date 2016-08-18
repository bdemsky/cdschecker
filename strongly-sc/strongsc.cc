#include "strongsc.h"
#include "action.h"
#include "threads-model.h"
#include "clockvector.h"
#include "execution.h"
#include <sys/time.h>
#include "scgraph.h"


StrongSC::StrongSC() {
    assignments = new AssignList;
    assignments->addAssignment(new MOAssignment);
}

StrongSC::~StrongSC() {

}

void StrongSC::setExecution(ModelExecution * execution) {
    this->execution = execution;
}

const char * StrongSC::name() {
	const char * name = "STRONGSC";
	return name;
}

void StrongSC::finish() {
}

bool StrongSC::option(char * opt) {
	if (strcmp(opt, "verbose")==0) {
		return false;
	} else if (strcmp(opt, "buggy")==0) {
		return false;
	} else if (strcmp(opt, "quiet")==0) {
		return false;
	} else if (strcmp(opt, "nonsc")==0) {
		return false;
	} else if (strcmp(opt, "time")==0) {
		return false;
	} else if (strcmp(opt, "help") != 0) {
		model_print("Unrecognized option: %s\n", opt);
	}
	return true;
}

void StrongSC::analyze(action_list_t *actions) {
    SCGraph *graph = new SCGraph(execution, actions, assignments);
}
