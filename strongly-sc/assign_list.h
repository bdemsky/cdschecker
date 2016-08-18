#ifndef _ASSIGN_LIST_H
#define _ASSIGN_LIST_H

#include "mo_assignment.h"

/**
    This class represents a list of memory order assnginments that have so far
    passed a set of rules
*/
class AssignList {
	private:
	ModelList<MOAssignment*> *list;

	public:
	AssignList();
	int getSize();

	/** Append another list to this list. If assign is stronger than any
     * assignemnts of the current list, then ignore assign; if assign is weaker
     * than any existing assignment "a" in the list, remove all such "a", and
     * then append assign to the end of the list. The invariable of the list is
     * that any two instances are not comparable. */
	bool addAssignment(MOAssignment *assign);

	void print(const char *msg = NULL);

	MEMALLOC

};


#endif
