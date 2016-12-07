#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>

#include "librace.h"
#include "register.h"

/**	@DeclareState: int val;
	@Initial: val = 0;
	@Commutativity: LOAD <-> LOAD (true)
	@Commutativity: STORE <-> LOAD (true)
	@Commutativity: STORE <-> STORE (true)
*/

/** @Interface: LOAD
	@PreCondition:
	return HasItem(PREV, STATE(val) == RET);
	@SideEffect: STATE(val) = RET;

*/
int Load_WRAPPER__(atomic_int *loc) {
	return loc->load(memory_order_acquire);
}

int Load(atomic_int *loc) {
	// Instrument with the INTERFACE_BEGIN annotation
	Method cur = _createInterfaceBeginAnnotation("LOAD");
	// Call the actual function
	int RET = Load_WRAPPER__(loc);
	
	// Initialize the value struct
	LOAD *value = new LOAD;
	value->RET = RET;
	value->loc = loc;

	// Store the value info into the current MethodCall
	cur->value = value;

	// Return if there exists a return value
	return RET;
}

/** @Interface: STORE 
	@SideEffect: STATE(val) = val;

*/
void Store_WRAPPER__(atomic_int *loc, int val) {
	loc->store(val, memory_order_release);
}

void Store(atomic_int *loc, int val) {
	// Instrument with the INTERFACE_BEGIN annotation
	Method cur = _createInterfaceBeginAnnotation("STORE");
	// Call the actual function
	Store_WRAPPER__(loc, val);
	
	// Initialize the value struct
	STORE *value = new STORE;
	value->loc = loc;
	value->val = val;

	// Store the value info into the current MethodCall
	cur->value = value;
}

static void a(void *obj)
{
	Store(&x, 1);
	Store(&x, 2);
}

static void b(void *obj)
{
	Store(&x, 3);
}

static void c(void *obj)
{
	int r1 = Load(&x);
	int r2 = Load(&x);
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2, t3;

	/** @Entry */
	_createInitAnnotation();

	thrd_create(&t1, (thrd_start_t)&a, NULL);
	thrd_create(&t2, (thrd_start_t)&b, NULL);
	thrd_create(&t3, (thrd_start_t)&c, NULL);

	thrd_join(t1);
	thrd_join(t2);
	thrd_join(t3);

	return 0;
}
