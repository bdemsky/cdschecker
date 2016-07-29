#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>

#include "librace.h"

atomic_int x;
atomic_int y;

static int r1, r2;

static void a(void *obj)
{
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	atomic_store_explicit(&y, 1, memory_order_release);
}

static void b(void *obj)
{
	r1=atomic_load_explicit(&y, memory_order_acquire);
	r1=atomic_load_explicit(&x, memory_order_relaxed);
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2,t3, t4;

	atomic_init(&x, 0);
	atomic_init(&y, 0);

	thrd_create(&t1, (thrd_start_t)&a, NULL);
	thrd_create(&t2, (thrd_start_t)&b, NULL);

	thrd_join(t1);
	thrd_join(t2);

	/* Check and/or print r1, r2, r3? */

	return 0;
}
