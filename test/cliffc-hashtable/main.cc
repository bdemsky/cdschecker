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
	IntWrapper *k1 = new IntWrapper(3), *k2 = new IntWrapper(5),
		*k3 = new IntWrapper(1024), *k4 = new IntWrapper(1025);
	IntWrapper *v1 = new IntWrapper(1024), *v2 = new IntWrapper(1025),
		*v3 = new IntWrapper(73), *v4 = new IntWrapper(81);
	table->put(k1, v1);
	table->put(k2, v2);
	val1 = table->get(k3);
	table->put(k3, v3);
}

void threadB(void *arg) {
	IntWrapper *k1 = new IntWrapper(3), *k2 = new IntWrapper(5),
		*k3 = new IntWrapper(1024), *k4 = new IntWrapper(1025);
	IntWrapper *v1 = new IntWrapper(1024), *v2 = new IntWrapper(1025),
		*v3 = new IntWrapper(73), *v4 = new IntWrapper(81);
	table->put(k1, v3);
	table->put(k2, v4);
	val1 = table->get(k2);
}

void threadC(void *arg) {
	IntWrapper *k1 = new IntWrapper(3), *k2 = new IntWrapper(5),
		*k3 = new IntWrapper(1024), *k4 = new IntWrapper(1025);
	IntWrapper *v1 = new IntWrapper(1024), *v2 = new IntWrapper(1025),
		*v3 = new IntWrapper(73), *v4 = new IntWrapper(81);
	table->put(k1, v1);
	table->put(k2, v2);
	val2 = table->get(k1);
}

void threadMain(void *arg) {
	for (int i = 200; i < 300; i++) {
		IntWrapper *k = new IntWrapper(i), *v = new IntWrapper(i * 2);
		table->put(k, v);
	}
}

int user_main(int argc, char *argv[]) {
	thrd_t t1, t2, t3, t4;
	table = new cliffc_hashtable<IntWrapper, IntWrapper>();
	val1 = NULL;
	val2 = NULL;
	threadMain(NULL);

	thrd_create(&t1, threadA, NULL);
	thrd_create(&t2, threadB, NULL);
		
	thrd_join(t1);
	thrd_join(t2);
			
	if (val1 == NULL) {
		cout << "val1: NULL" << endl;
	} else {
		cout << val1->get() << endl;
	}
		if (val2 == NULL) {
		cout << "val2: NULL" << endl;
	} else {
		cout << val2->get() << endl;
	}
	return 0;
}


