#ifndef _MO_ASSIGNMENT_H
#define _MO_ASSIGNMENT_H

#include "wildcard.h"
#include "sc_patch.h"

#define ASSIGNEMNT_INCOMPARABLE(x) (!(-1 <= (x) <= 1))

class MOAssignment {
	private:
	memory_order *orders;
	int size;

	void resize(int newsize);

    // Check whether specific semantics (wildcard) is on an action
	bool isRelease(const ModelAction *act);
	bool isAcquire(const ModelAction *act);
	bool isSC(const ModelAction *act);

    // Impose specific semantics on an action
	bool imposeRelease(const ModelAction *act);
	bool imposeAcquire(const ModelAction *act);
	bool imposeSC(const ModelAction *act);
	
	public:
	MOAssignment();
	MOAssignment(MOAssignment *assign);
	~MOAssignment();

    // Check whether the assignment has already satisfied the patch
    bool hasSatisfied(SCPatch *p);
    bool apply(SCPatch *p);
	
	/** return value:
	  * 0 -> mo1 == mo2;
	  * 1 -> mo1 stronger than mo2;
	  * -1 -> mo1 weaker than mo2;
	  * 2 (i.e. ASSIGNMENT_INCOMPARABLE(x)) -> mo1 & mo2 are uncomparable.
	 */
	static int compareMemoryOrder(memory_order mo1, memory_order mo2);

	int getSize() {
		return size;
	}

	memory_order &operator[](int idx);
	
	/** @Return:
		1 -> 'this is stronger than assign';
		-1 -> 'this is weaker than assign'
		0 -> 'this == assignment'
		ASSIGNMENT_INCOMPARABLE(x) -> true means incomparable
	 */
	int compareTo(const MOAssignment *assign) const;

	unsigned long getHash();
	
	void print(bool hasHash = true);

	MEMALLOC
};
#endif
