#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>

#include "librace.h"

#include "wildcard.h"

atomic_int x;
atomic_int y;
atomic_int v;

/** Release Sequence 
 *  The optimal solution for this type of cycle is just to make the head store
 *  operation release and the later load operation acquire
 */

static void a(void *obj)
{
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	atomic_fetch_add_explicit(&v, 1, memory_order_release);
}

static void b(void *obj)
{
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	atomic_fetch_add_explicit(&v, 1, memory_order_release);
}

static void c(void *obj)
{
	int r1=atomic_load_explicit(&v, memory_order_acquire);
	int r2=atomic_load_explicit(&x, memory_order_relaxed);
	int r3=atomic_load_explicit(&y, memory_order_relaxed);
}


int user_main(int argc, char **argv)
{
	thrd_t t1, t2, t3;

	atomic_init(&x, 0);
	atomic_init(&y, 0);
	atomic_init(&v, 0);

	thrd_create(&t1, (thrd_start_t)&a, NULL);
	thrd_create(&t2, (thrd_start_t)&b, NULL);
	thrd_create(&t3, (thrd_start_t)&c, NULL);

	thrd_join(t1);
	thrd_join(t2);
	thrd_join(t3);

	return 0;
}
