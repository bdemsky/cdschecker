#include <cdsannotate.h>
#include <modeltypes.h>
#include <atomic>
#include <threads.h>
#include <stdatomic.h>

#include "mymemory.h"
#include "specannotation.h"
#include "cdsspec.h"
#include "methodcall.h"

using namespace std;

typedef struct StateStruct {
	int val;

	SNAPSHOTALLOC
} StateStruct;

typedef struct LOAD {
	int RET;
	atomic_int *loc;
} LOAD;

typedef struct STORE {
	atomic_int *loc;
	int val;
} STORE;

inline void _initial(Method m) {
	StateStruct *state = new StateStruct;
	state->val = 0;
	m->state = state;
}

inline void _copyState(Method dest, Method src) {
	StateStruct *stateSrc = (StateStruct*) src;
	StateStruct *stateDest = (StateStruct*) dest;
	stateDest->val = stateSrc->val;
}

inline bool _checkCommutativity1(Method m1, Method m2) {
	if (m1->interfaceName == "LOAD" && m2->interfaceName == "LOAD") {
		if (true)
			return true;
		else
			return false;
	}
	return false;
}

inline bool _checkCommutativity2(Method m1, Method m2) {
	if (m1->interfaceName == "STORE" && m2->interfaceName == "LOAD") {
		if (true)
			return true;
		else
			return false;
	}
	return false;
}

inline bool _checkCommutativity3(Method m1, Method m2) {
	if (m1->interfaceName == "STORE" && m2->interfaceName == "STORE") {
		if (true)
			return true;
		else
			return false;
	}
	return false;
}

inline void _LOAD_Transition(Method _M, Method _exec) {
	return;
}

inline bool _LOAD_PreCondition(Method _M) {
	int RET = ((LOAD*) _M->value)->RET;
	atomic_int *loc = ((LOAD*) _M->value)->loc;
	return HasItem(PREV, Guard(STATE(val) == RET && loc));
}

inline void _LOAD_SideEffect(Method _M) {
	int RET = ((LOAD*) _M->value)->RET;
	atomic_int *loc = ((LOAD*) _M->value)->loc;
	STATE(val) = RET;
}

inline void _STORE_Transition(Method _M, Method _exec) {
	return;
}

inline void _STORE_SideEffect(Method _M) {
	atomic_int *loc = ((STORE*) _M->value)->loc;
	int val = ((STORE*) _M->value)->val;
	STATE(val) = val;
}

inline void _createInitAnnotation() {
	int CommuteRuleSize = 3;
	void *raw = CommutativityRule::operator new[](sizeof(CommutativityRule) * CommuteRuleSize);
	CommutativityRule *commuteRules = static_cast<CommutativityRule*>(raw);

	// Initialize commutativity rule 1
	new( &commuteRules[0] )CommutativityRule("LOAD", "LOAD", "LOAD <-> LOAD (true)", _checkCommutativity1);
	// Initialize commutativity rule 2
	new( &commuteRules[1] )CommutativityRule("Store", "LOAD", "STORE <-> LOAD (true)", _checkCommutativity2);
	// Initialize commutativity rule 3
	new( &commuteRules[2] )CommutativityRule("STORE", "STORE", "STORE <-> STORE (true)", _checkCommutativity3);

	// Initialize AnnoInit
	AnnoInit *anno = new AnnoInit(_initial, _copyState, commuteRules, CommuteRuleSize);

	// Initialize StateFunctions map
	StateFunctions *stateFuncs;

	// StateFunction for LOAD
	stateFuncs = new StateFunctions("LOAD", _LOAD_Transition, NULL, _LOAD_PreCondition, _LOAD_SideEffect, NULL);
	anno->addInterfaceFunctions("LOAD", stateFuncs);
	// StateFunction for STORE 
	stateFuncs = new StateFunctions("STORE", _STORE_Transition, NULL, NULL, _STORE_SideEffect, NULL);
	anno->addInterfaceFunctions("STORE", stateFuncs);

	// Create and instrument with the INIT annotation
	cdsannotate(SPEC_ANALYSIS, new SpecAnnotation(INIT, anno));
}

inline Method _createInterfaceBeginAnnotation(string name) {
	Method cur = new MethodCall(name);
	// Create and instrument with the INTERFACE_BEGIN annotation
	cdsannotate(SPEC_ANALYSIS, new SpecAnnotation(INTERFACE_BEGIN, cur));
	return cur;
}

inline void _createOPDefineAnnotation() {
	cdsannotate(SPEC_ANALYSIS, new SpecAnnotation(OP_DEFINE, NULL));
}

inline void _createPotentialOPAnnotation(string label) {
	cdsannotate(SPEC_ANALYSIS, new SpecAnnotation(POTENTIAL_OP, new AnnoPotentialOP(label)));
}

inline void _createOPCheckAnnotation(string label) {
	cdsannotate(SPEC_ANALYSIS, new SpecAnnotation(OP_CHECK, new AnnoOPCheck(label)));
}

inline void _createOPClearAnnotation() {
	cdsannotate(SPEC_ANALYSIS, new SpecAnnotation(OP_CLEAR, NULL));
}

inline void _createOPClearDefineAnnotation() {
	cdsannotate(SPEC_ANALYSIS, new SpecAnnotation(OP_CLEAR_DEFINE, NULL));
}
