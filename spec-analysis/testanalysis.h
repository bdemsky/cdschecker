#ifndef TESTANALYSIS_H
#define TESTANALYSIS_H

#include "traceanalysis.h"
#include "mymemory.h"

class TESTAnalysis : public TraceAnalysis {
 public:
	TESTAnalysis();
	~TESTAnalysis();
	virtual void setExecution(ModelExecution * execution);
	virtual void analyze(action_list_t *actions);
	virtual const char * name();
	virtual bool option(char *);
	virtual void finish();

	SNAPSHOTALLOC
 private:
	ModelExecution *execution;
};


#endif
