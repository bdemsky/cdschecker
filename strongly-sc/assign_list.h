#ifndef _ASSIGN_LIST_H
#define _ASSIGN_LIST_H

#include "mo_assignment.h"

/**
    This class represents a list of memory order assnginments that have so far
    passed a set of rules. Given a list of patches, we can apply those patches
    to strengthen the assignment list.
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
     * then append assign to the end of the list.
     */
	bool addAssignment(MOAssignment *assign);

	static bool addAssignment(ModelList<MOAssignment*> *aList, MOAssignment *assign);

    /**
        Check the list to see if there is any redundant assignments (obviously
        stronger than other existing assignments)
    */
    void compressList();

    /**
        Given a list of patches, we apply each patch to the existing list of
        assignments, and create a new list of strengthened assignments.
    */
    void applyPatches(patch_list_t *patches);

	void print(const char *msg = NULL);

	MEMALLOC

};


#endif
