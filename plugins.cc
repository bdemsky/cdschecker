#include "plugins.h"
#include "scanalysis.h"
#include "specanalysis.h"
#include "scfence.h"

ModelVector<TraceAnalysis *> * registered_analysis;
ModelVector<TraceAnalysis *> * installed_analysis;

SCFence *wildcard_plugin;

void register_plugins() {
	registered_analysis=new ModelVector<TraceAnalysis *>();
	installed_analysis=new ModelVector<TraceAnalysis *>();
	registered_analysis->push_back(new SCAnalysis());
	registered_analysis->push_back(new SPECAnalysis());

	/** Initialize the wildcard_plugin */
	wildcard_plugin = new SCFence();
	registered_analysis->push_back(wildcard_plugin);
}

ModelVector<TraceAnalysis *> * getRegisteredTraceAnalysis() {
	return registered_analysis;
}

ModelVector<TraceAnalysis *> * getInstalledTraceAnalysis() {
	return installed_analysis;
}
