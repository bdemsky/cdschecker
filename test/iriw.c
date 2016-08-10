#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>

#include "librace.h"
#include "wildcard.h"

atomic_int x;
atomic_int y;

static int r1, r2, r3, r4;

static void a(void *obj)
{
	atomic_store_explicit(&x, 1, wildcard(1));
}

static void b(void *obj)
{
	atomic_store_explicit(&y, 1, wildcard(2));
}

static void c(void *obj)
{
	r1=atomic_load_explicit(&x, wildcard(3));
	r2=atomic_load_explicit(&y, wildcard(4));
}

static void d(void *obj)
{
	r3=atomic_load_explicit(&y, wildcard(5));
	r4=atomic_load_explicit(&x, wildcard(6));
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2, t3, t4;

	atomic_init(&x, 0);
	atomic_init(&y, 0);

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
