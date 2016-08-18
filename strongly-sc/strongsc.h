#ifndef STRONGSC_H
#define STRONGSC_H
#include "scanalysis.h"
#include "traceanalysis.h"
#include "hashtable.h"
#include "mo_assignment.h"
#include "assign_list.h"

class StrongSC : public TraceAnalysis {
 public:
	StrongSC();
	~StrongSC();
	virtual void setExecution(ModelExecution * execution);
	virtual void analyze(action_list_t *);
	virtual const char * name();
	virtual bool option(char *);
	virtual void finish();

	SNAPSHOTALLOC
 private:
    ModelExecution *execution;
    AssignList *assignments;
};
#endif
