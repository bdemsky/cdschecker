#include "testanalysis.h"
#include "action.h"
#include "cyclegraph.h"
#include "threads-model.h"
#include "clockvector.h"
#include "execution.h"
#include <sys/time.h>
#include <assert.h>

static int test_trace_cnt = 0;

TESTAnalysis::TESTAnalysis() {}

TESTAnalysis::~TESTAnalysis() {}

void TESTAnalysis::setExecution(ModelExecution * execution) {
	this->execution = execution;
}

const char * TESTAnalysis::name() {
	const char * name = "TEST";
	return name;
}

void TESTAnalysis::finish() {
	model_print("TESTAnalysis finished!\n");
}

bool TESTAnalysis::option(char * opt) {
	return true;
}


void TESTAnalysis::analyze(action_list_t *actions) {
	if (test_trace_cnt % 1000 == 0)
		model_print("TESTAnalysis analyzing: %d!\n", test_trace_cnt);
	test_trace_cnt++;
	//traverseActions(actions);
}


