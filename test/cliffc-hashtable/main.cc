#include <iostream>
#include <threads.h>
#include "cliffc_hashtable.h"

using namespace std;

template<typename TypeK, typename TypeV>
slot* const cliffc_hashtable<TypeK, TypeV>::MATCH_ANY = new slot(false, NULL);

template<typename TypeK, typename TypeV>
slot* const cliffc_hashtable<TypeK, TypeV>::NO_MATCH_OLD = new slot(false, NULL);

template<typename TypeK, typename TypeV>
slot* const cliffc_hashtable<TypeK, TypeV>::TOMBPRIME = new slot(true, NULL);

template<typename TypeK, typename TypeV>
slot* const cliffc_hashtable<TypeK, TypeV>::TOMBSTONE = new slot(false, NULL);


class IntWrapper {
	private:
		public:
	    int _val;

		IntWrapper(int val) : _val(val) {}

		IntWrapper() : _val(0) {}

		IntWrapper(IntWrapper& copy) : _val(copy._val) {}

		int get() {
			return _val;
		}

		int hashCode() {
			return _val;
		}
		
		bool operator==(const IntWrapper& rhs) {
			return false;
		}

		bool equals(const void *another) {
			if (another == NULL)
				return false;
			IntWrapper *ptr =
				(IntWrapper*) another;
			return ptr->_val == _val;
		}
};

cliffc_hashtable<IntWrapper, IntWrapper> *table;
IntWrapper *val1, *val2;

void threadA(void *arg) {
	
	IntWrapper *k1 = new IntWrapper(3), *k2 = new IntWrapper(2),
		*k3 = new IntWrapper(1024), *k4 = new IntWrapper(1025);
	IntWrapper *v1 = new IntWrapper(1024), *v2 = new IntWrapper(1025),
		*v3 = new IntWrapper(73), *v4 = new IntWrapper(81);
	
	table->put(k1, v1);
	table->put(k2, v2);
			
	val1 = table->get(k3);
	if (val1 != NULL)
		model_print("val1: %d\n", val1->_val);
	else
		model_print("val1: NULL\n");
		
}

void threadB(void *arg) {
	
}

void threadMain(void *arg) {
	
	IntWrapper *k1 = new IntWrapper(3), *k2 = new IntWrapper(5),
		*k3 = new IntWrapper(1024), *k4 = new IntWrapper(1025);
	IntWrapper *v1 = new IntWrapper(1024), *v2 = new IntWrapper(1025),
		*v3 = new IntWrapper(73), *v4 = new IntWrapper(81);
	table->put(k3, v3);
	}

int user_main(int argc, char *argv[]) {
	thrd_t t1, t2;
	table = new cliffc_hashtable<IntWrapper, IntWrapper>(2);
	val1 = NULL;
	val2 = NULL;
	
	thrd_create(&t1, threadA, NULL);
	thrd_create(&t2, threadB, NULL);
	threadMain(NULL);

	thrd_join(t1);
	thrd_join(t2);
	
	return 0;
}


