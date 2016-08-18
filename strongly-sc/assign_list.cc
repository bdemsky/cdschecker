#include "strongsc_common.h"
#include "assign_list.h"

AssignList::AssignList() {
	list = new ModelList<MOAssignment *>;
}

int AssignList::getSize() {
	return list->size();
}

/** Append another list to this list. If assign is stronger than any
 * assignemnts of the current list, then ignore assign; if assign is weaker
 * than any existing assignment "a" in the list, remove all such "a", and
 * then append assign to the end of the list. The invariable of the list is
 * that any two instances are not comparable. */
bool AssignList::addAssignment(MOAssignment *assign) {
	ModelList<MOAssignment *>::iterator it;
	int compVal;
    bool hasReplaced = false;
	for (it = list->begin(); it != list->end();) {
		MOAssignment *existing = *it;
		compVal = existing->compareTo(assign);
		if (compVal == 0 || compVal == -1) {
            // "existing" is as strong as "assign" or weaker than "assign", bail
			return false;
		} else if (compVal == 1) {
            // "existing" is stronger than "assign"
            if (hasReplaced) {
                // "assign" is already in the list
                // Simply delete the existing
                list->erase(it);
                delete existing;
                continue;
            } else {
                // Replace existing with assign
                *it = assign;
                hasReplaced = true;
                delete existing;
            }
        }
        // Go to the next one
        it++;
	}
    list->push_back(assign);
    return true;
}


void AssignList::clearAll() {
    for (ModelList<MOAssignment*>::iterator assignIter = list->begin();
        assignIter != list->end(); assignIter++) {
        MOAssignment 
    }
}

/**
    Check the list to see if there is any redundant assignments (obviously
    stronger than other existing assignments)
*/
void AssignList::compressList() {
    ModelList<MOAssignment*> *newList = new ModelList<MOAssignment*>;
    for (ModelList<MOAssignment*>::iterator it = list->begin();
        it != list->end(); it++) {
        MOAssignment *assign = *it;
        if (!newList->addAssignment(assign)) {
            // "assign" was not added since it is either the same or stronger
            // than existing assignments

            // So we delte it
            delete assign;
        }
    }
    delete list;
    list = newList;
}

/**
    Given a list of patches, we apply each patch to the existing list of
    assignments, and create a new list of strengthened assignments.
*/
void AssignList::applyPatches(patch_list_t *patches) {
    if (patches == NULL || patches->empty())
        return;
    unsigned assignSize = list->size();
    ModelList<MOAssignment*>::iterator assignIter = list->begin();
    for (unsigned assignCnt = 0; assignCnt != assignSize; assignCnt++) {
        MOAssignment *assign = *assignIter;
        bool hasSatisfied = false;
        for (patch_list_t::iterator it = patches->begin(); it != patches->end();
            it++) {
            SCPatch *p = *it;
            if (assign->hasSatisfied(p)) {
                // For an assignment, if any of the patch is satisfied, no need
                // to strengthen anything for these patches
                hasSatisfied = true;
                break;
            } else {
                MOAssignment *newAssign = new MOAssignment(assign);
                newAssign->apply(p);
                list->push_back(p);
            }
        }
        if (!hasSatisfied) {
            // The current assignment should be deleted since it is too weak
            delete assign;
            list->erase(it);
        } else {
            // The current assignment is still good, simply go to the next
            // assignment
            assignIter++;
        }
    }

    // When the list is too big (probably too many redundant assignments), we
    // compress the list
    // FIXME: WHY do we randomly pick 50
    if (list->size() > 50) {
        compressList();
    }
}

void AssignList::print(const char *msg) {
    int cnt = 1;
	for (ModelList<MOAssignment *>::iterator it = list->begin(); it !=
		list->end(); it++) {
		MOAssignment *assign = *it;
        if (msg) {
		    model_print("%s %d:\n", msg, cnt++);
        } else {
		    model_print("%d:\n", cnt++);
        }
		assign->print();
		model_print("\n");
	}
}
