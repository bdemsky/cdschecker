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

    /**
        Check the list to see if there is any redundant assignments (obviously
        stronger than other existing assignments)
    */
    void compressList();

	/** Append another list to this list. If assign is stronger than any
     * assignemnts of the current list, then ignore assign; if assign is weaker
     * than any existing assignment "a" in the list, remove all such "a", and
     * then append assign to the end of the list.
     */
	bool addAssignment(MOAssignment *assign);

    /**
        Given a list of patches, we apply each patch to the existing list of
        assignments, and create a new list of strengthened assignments.
    */
    void applyPatches(patch_list_t *patches);

	void print(const char *msg = NULL);

	MEMALLOC

};


#endif
