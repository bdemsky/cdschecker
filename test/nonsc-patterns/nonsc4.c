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
	int r1=atomic_load_explicit(&x, memory_order_wildcard(1));
	atomic_store_explicit(&y, 1, memory_order_wildcard(2));
}

static void b(void *obj)
{
	int r2=atomic_load_explicit(&y, memory_order_wildcard(3));
	atomic_store_explicit(&z, 1, memory_order_wildcard(4));
}

static void c(void *obj)
{
	int r3=atomic_load_explicit(&z, memory_order_wildcard(5));
	atomic_store_explicit(&x, 1, memory_order_wildcard(6));
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2, t3;

	atomic_init(&x, 0);
	atomic_init(&y, 0);
	atomic_init(&z, 0);

	thrd_create(&t1, (thrd_start_t)&a, NULL);
	thrd_create(&t2, (thrd_start_t)&b, NULL);
	thrd_create(&t3, (thrd_start_t)&c, NULL);

	thrd_join(t1);
	thrd_join(t2);
	thrd_join(t3);

	return 0;
}
