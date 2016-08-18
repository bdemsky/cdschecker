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
