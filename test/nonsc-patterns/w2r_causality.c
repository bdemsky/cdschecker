#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>
#include <model-assert.h>

#include "librace.h"

#include "wildcard.h"

atomic_int x;
atomic_int y;
atomic_int z;

int r1, r2, r3, r4;

/** Write-to-Read Causality 
 *  The optimal solution for this type of cycle is to all the cross-thread
 *  reads-from edges in the cycle
 */


static void a(void *obj)
{
	atomic_store_explicit(&z, 1, memory_order_relaxed);
	atomic_store_explicit(&x, 1, memory_order_release);
}

static void b(void *obj)
{
	r1=atomic_load_explicit(&x, memory_order_acquire);
	atomic_store_explicit(&y, 1, memory_order_release);
}

static void c(void *obj)
{
	r2=atomic_load_explicit(&y, memory_order_acquire);
	//r3=atomic_load_explicit(&x, memory_order_relaxed);
	r4=atomic_load_explicit(&z, memory_order_relaxed);
}


int user_main(int argc, char **argv)
{
	thrd_t t1, t2, t3;

	r1 = 0;
	r2 = 0;
	r3 = 0;
	r4 = 4;

	atomic_init(&x, 0);
	atomic_init(&y, 0);
	atomic_init(&z, 0);

	thrd_create(&t1, (thrd_start_t)&a, NULL);
	thrd_create(&t2, (thrd_start_t)&b, NULL);
	thrd_create(&t3, (thrd_start_t)&c, NULL);

	thrd_join(t1);
	thrd_join(t2);
	thrd_join(t3);

	//MODEL_ASSERT (!(r1 == 1 && r2 == 1 && r4 == 0));

	return 0;
}
