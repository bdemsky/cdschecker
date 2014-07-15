#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>

#include "librace.h"

#include "wildcard.h"

atomic_int x;
atomic_int y;
atomic_int z;

static void a(void *obj)
{
	atomic_store_explicit(&z, 0, memory_order_relaxed);
}

static void b(void *obj)
{
	atomic_store_explicit(&x, 1, memory_order_wildcard(1));
	atomic_store_explicit(&y, 1, memory_order_wildcard(2));
	int r1=atomic_load_explicit(&z, memory_order_wildcard(3));
}

static void c(void *obj)
{
	atomic_store_explicit(&z, 1, memory_order_wildcard(4));
	atomic_store_explicit(&x, 1000, memory_order_wildcard(5));
	int r2=atomic_load_explicit(&y, memory_order_wildcard(6));
}

static void d(void *obj)
{
	atomic_store_explicit(&z, 2, memory_order_wildcard(7));
	atomic_store_explicit(&y, 1000, memory_order_wildcard(8));
	int r3=atomic_load_explicit(&x, memory_order_wildcard(9));
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2, t3, t4;

	atomic_init(&x, 0);
	atomic_init(&y, 0);
	atomic_init(&z, 0);

	thrd_create(&t1, (thrd_start_t)&a, NULL);
	thrd_create(&t2, (thrd_start_t)&b, NULL);
	thrd_create(&t3, (thrd_start_t)&c, NULL);
	thrd_create(&t4, (thrd_start_t)&d, NULL);

	thrd_join(t1);
	thrd_join(t2);
	thrd_join(t3);
	thrd_join(t4);

	return 0;
}
