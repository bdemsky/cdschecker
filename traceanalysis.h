#ifndef TRACE_ANALYSIS_H
#define TRACE_ANALYSIS_H
#include "model.h"

class TraceAnalysis {
 public:
	/** Constructor for the analysis with the reference of the ModelChecker */

	TraceAnalysis(ModelChecker **model_ptr) { this->model_ptr = model_ptr; }
	
	/** Constructor for the analysis without the reference of the ModelChecker */

	TraceAnalysis() { this->model_ptr = NULL; }


	/** setExecution is called once after installation with a reference to
	 *  the ModelExecution object. */

	virtual void setExecution(ModelExecution * execution) = 0;
	
	/** analyze is called once for each feasible trace with the complete
	 *  action_list object. */

	virtual void analyze(action_list_t *) = 0;

	/** name returns the analysis name string */

	virtual const char * name() = 0;

	/** Each analysis option is passed into the option method.  This
	 *	occurs before installation (i.e., you don't have a
	 *	ModelExecution object yet).  A TraceAnalysis object should
	 *	support the option "help"  */

	virtual bool option(char *) = 0;

	/** The finish method is called once at the end.  This should be
	 *  used to print out results.  */

	virtual void finish() = 0;

	SNAPSHOTALLOC

  //private:
	/** Expose the reference of the model checker to the pluggin in order for
	 * the pluggin to restart. */
	ModelChecker **model_ptr;
};
#endif
