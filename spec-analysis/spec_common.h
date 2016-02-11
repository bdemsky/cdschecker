#ifndef _SPEC_COMMON_H
#define _SPEC_COMMON_H

#include <set>
#include <stdlib.h>
#include "mymemory.h"

#define SPEC_ANALYSIS 1

/** Null function pointer */
#define NULL_FUNC NULL

#define NEW_SIZE(type, size) (type*) malloc(size)
#define NEW(type) NEW_SIZE(type, sizeof(type))

#define EQ(str1, str2) (strcmp(str1, str2) == 0)

extern const unsigned int METHOD_ID_MAX;
extern const unsigned int METHOD_ID_MIN;

typedef const char *CSTR;

extern CSTR GRAPH_START;
extern CSTR GRAPH_FINISH;

/** Define a snapshotting set for CDSChecker backend analysis */
template<typename _Tp>
class SnapSet : public std::set<_Tp, std::less<_Tp>, SnapshotAlloc<_Tp> >
{   
	public:
	typedef std::set<_Tp, std::less<_Tp>, SnapshotAlloc<_Tp> > set;
	 
	SnapSet() : set() { }

	SNAPSHOTALLOC
};

extern SnapshotAlloc<char> snapshotAlloc;

#endif
