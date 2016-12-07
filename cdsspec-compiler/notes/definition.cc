#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <iterator>
#include <algorithm>
#include <set>

#include <functional>

#include <stdarg.h>

using namespace std;

struct MethodCall;

typedef MethodCall *Method;
typedef set<Method> *MethodSet;

struct MethodCall {
	string interfaceName; // The interface label name
	void *value; // The pointer that points to the struct that have the return
				 // value and the arguments
	void *state; // The pointer that points to the struct that represents
					  // the state
	MethodSet prev; // Method calls that are hb right before me
	MethodSet next; // Method calls that are hb right after me
	MethodSet concurrent; // Method calls that are concurrent with me

	MethodCall(string name) : MethodCall() { interfaceName = name; }
	
	MethodCall() : interfaceName(""), prev(new set<Method>),
		next(new set<Method>), concurrent(new set<Method>) { }

	void addPrev(Method m) { prev->insert(m); }

	void addNext(Method m) { next->insert(m); }
	
	void addConcurrent(Method m) { concurrent->insert(m); }

};


typedef vector<int> IntVector;
typedef list<int> IntList;
typedef set<int> IntSet;

typedef vector<double> DoubleVector;
typedef list<double> DoubleList;
typedef set<double> DoubleSet;

/********** More general specification-related types and operations **********/

#define NewMethodSet new set<Method>

#define CAT(a, b) CAT_HELPER(a, b) /* Concatenate two symbols for macros! */
#define CAT_HELPER(a, b) a ## b
#define X(name) CAT(__##name, __LINE__) /* unique variable */

/**
	This is a generic ForEach primitive for all the containers that support
	using iterator to iterate.
*/
#define ForEach(item, container) \
	auto X(_container) = (container); \
	auto X(iter) = X(_container)->begin(); \
	for (auto item = *X(iter); X(iter) != X(_container)->end(); item = ((++X(iter)) != \
		X(_container)->end()) ? *X(iter) : 0)

/**
	This is a common macro that is used as a constant for the name of specific
	variables. We basically have two usage scenario:
	1. In Subset operation, we allow users to specify a condition to extract a
	subset. In that condition expression, we provide NAME, RET(type), ARG(type,
	field) and STATE(field) to access each item's (method call) information.
	2. In general specification (in pre- & post- conditions and side effects),
	we would automatically generate an assignment that assign the current
	MethodCall* pointer to a variable namedd _M. With this, when we access the
	state of the current method, we use STATE(field) (this is a reference
	for read/write).
*/
#define ITEM _M
#define _M ME

#define NAME Name(_M)

#define STATE(field) State(_M, field)

#define VALUE(type, field) Value(_M, type, field)
#define RET(type) VALUE(type, RET)
#define ARG(type, arg) VALUE(type, arg)

/*
#define Subset(s, subset, condition) \
	MethodSet original = s; \
	MethodSet subset = NewMethodSet; \
	ForEach (_M, original) { \
		if ((condition)) \
			subset->insert(_M); \
	} \
*/


/**
	This operation is specifically for Method set. For example, when we want to
	construct an integer set from the state field "x" (which is an
	integer) of the previous set of method calls (PREV), and the new set goes to
	"readSet", we would call "MakeSet(int, PREV, readSet, STATE(x));". Then users
	can use the "readSet" as an integer set (set<int>)
*/
#define MakeSet(type, oldset, newset, field) \
	auto newset = new set<type>; \
	ForEach (_M, oldset) \
		newset->insert(field); \

/**
	We provide a more general subset operation that takes a plain boolean
	expression on each item (access by the name "ITEM") of the set, and put it
	into a new set if the boolean expression (Guard) on that item holds.  This
	is used as the second arguments of the Subset operation. An example to
	extract a subset of positive elements on an IntSet "s" would be "Subset(s,
	GeneralGuard(int, ITEM > 0))"
*/
#define GeneralGuard(type, expression)  std::function<bool(type)> ( \
	[&](type ITEM) -> bool { return (expression);}) \

/**
	This is a more specific guard designed for the Method (MethodCall*). It
	basically wrap around the GeneralGuard with the type Method. An example to
	extract the subset of method calls in the PREV set whose state "x" is equal
	to "val" would be "Subset(PREV, Guard(STATE(x) == val))"
*/
#define Guard(expression) GeneralGuard(Method, expression)

/**
	A general subset operation that takes a condition and returns all the item
	for which the boolean guard holds.
*/
inline MethodSet Subset(MethodSet original, std::function<bool(Method)> condition) {
	MethodSet res = new SnapSet<Method>;
	ForEach (_M, original) {
		if (condition(_M))
			res->insert(_M);
	}
	return res;
}

/**
	A general set operation that takes a condition and returns if there exists
	any item for which the boolean guard holds.
*/
template <class T>
inline bool HasItem(set<T> *original, std::function<bool(T)> condition) {
	ForEach (_M, original) {
		if (condition(_M))
			return true;
	}
	return false;
}



/**
	A general sublist operation that takes a condition and returns all the item
	for which the boolean guard holds in the same order as in the old list.
*/
template <class T>
inline list<T>* Sublist(list<T> *original, std::function<bool(T)> condition) {
	list<T> *res = new list<T>;
	ForEach (_M, original) {
		if (condition(_M))
			res->push_back(_M);
	}
	return res;
}

/**
	A general subvector operation that takes a condition and returns all the item
	for which the boolean guard holds in the same order as in the old vector.
*/
template <class T>
inline vector<T>* Subvector(vector<T> *original, std::function<bool(T)> condition) {
	vector<T> *res = new vector<T>;
	ForEach (_M, original) {
		if (condition(_M))
			res->push_back(_M);
	}
	return res;
}

/** General for set, list & vector */
#define Size(container) ((container)->size())

#define _BelongHelper(type) \
	template<class T> \
	inline bool Belong(type<T> *container, T item) { \
		return std::find(container->begin(), \
			container->end(), item) != container->end(); \
	}

_BelongHelper(set)
_BelongHelper(vector)
_BelongHelper(list)

/** General set operations */
template<class T>
inline set<T>* Intersect(set<T> *set1, set<T> *set2) {
	set<T> *res = new set<T>;
	ForEach (item, set1) {
		if (Belong(set2, item))
			res->insert(item);
	}
	return res;
}

template<class T>
inline set<T>* Union(set<T> *s1, set<T> *s2) {
	set<T> *res = new set<T>(*s1);
	ForEach (item, s2)
		res->insert(item);
	return res;
}

template<class T>
inline set<T>* Subtract(set<T> *set1, set<T> *set2) {
	set<T> *res = new set<T>;
	ForEach (item, set1) {
		if (!Belong(set2, item))
			res->insert(item);
	}
	return res;
}

template<class T>
inline void Insert(set<T> *s, T item) { s->insert(item); }

template<class T>
inline void Insert(set<T> *s, set<T> *others) {
	ForEach (item, others)
		s->insert(item);
}

/*
inline MethodSet MakeSet(int count, ...) {
	va_list ap;
	MethodSet res;

	va_start (ap, count);
	res = NewMethodSet;
	for (int i = 0; i < count; i++) {
		Method item = va_arg (ap, Method);
		res->insert(item);
	}
	va_end (ap);
	return res;
}
*/

/********** Method call related operations **********/
#define Name(method) method->interfaceName

#define State(method, field) ((StateStruct*) method->state)->field

#define Value(method, type, field) ((type*) method->value)->field
#define Ret(method, type) Value(method, type, RET)
#define Arg(method, type, arg) Value(method, type, arg)

#define Prev(method) method->prev
#define PREV _M->prev

#define Next(method) method->next
#define NEXT _M->next

#define Concurrent(method) method->concurrent
#define CONCURRENT  _M->concurrent


/***************************************************************************/
/***************************************************************************/

// This auto-generated struct can have different fields according to the read
// state declaration. Here it's just a test example
typedef struct StateStruct {
	int x;
} StateStruct;

// These auto-generated struct can have different fields according to the return
// value and arguments of the corresponding interface. The struct will have the
// same name as the interface name. Here it's just a test example
typedef struct Store {
	int *loc;
	int val;
} Store;

typedef struct Load {
	int RET;
	int *loc;
} Load;

int main() {
	set<int> *is1 = new set<int>;
	set<int> *is2 = new set<int>;

	list<int> *il1 = new list<int>;
	list<int> *il2 = new list<int>;

	il1->push_back(2);
	il1->push_back(3);
	
	is1->insert(1);
	is1->insert(3);
	
	is2->insert(4);
	is2->insert(5);


	MethodSet ms = NewMethodSet;
	Method m = new MethodCall;
	m->interfaceName = "Store";
	StateStruct *ss = new StateStruct;
	ss->x = 1;
	m->state = ss;
	Store *st = new Store;
	st->val = 2;
	m->value = st;
	ms->insert(m);

	m = new MethodCall;
	m->interfaceName= "Store";
	ss = new StateStruct;
	ss->x = 2;
	m->state = ss;
	st = new Store;
	st->val = 0;
	m->value = st;
	ms->insert(m);

	m = new MethodCall;
	m->interfaceName= "Load";
	ss = new StateStruct;
	ss->x = 0;
	m->state = ss;
	Load *ld = new Load;
	ld->RET = 2;
	m->value = ld;
	ms->insert(m);

	//MakeSet(int, ms, newis, STATE(x));
	//cout << "Size=" << Size(newis) << " | val= " << Belong(newis, 2) << endl;
	
	int x = 2;
	int y = 2;
	cout << "HasItem=" << HasItem(ms, Guard(STATE(x) == x && y == 0)) << endl;
	
	//ForEach (i, newis) {
		//cout << "elem: " << i << endl;
	//}


	//Subset(ms, sub, NAME == "Store" && VALUE(Store, val) != 0);
	//cout << "Size=" << Size(Subset(ms, Guard(NAME == "Store" && ARG(Store, val)
		//>= 0 && STATE(x) >= 0 ))) << endl;
	return 0;
}
