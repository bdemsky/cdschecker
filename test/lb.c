#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>

#include "librace.h"
#include "wildcard.h"

atomic_int x;
atomic_int y;

static int r1, r2;

static void a(void *obj)
{
	r1=atomic_load_explicit(&y, wildcard(1));
	atomic_store_explicit(&x, 1, wildcard(2));
}

static void b(void *obj)
{
	r2=atomic_load_explicit(&x, wildcard(3));
	atomic_store_explicit(&y, 1, wildcard(4));
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2;

	atomic_init(&x, 0);
	atomic_init(&y, 0);

	thrd_create(&t1, (thrd_start_t)&a, NULL);
	thrd_create(&t2, (thrd_start_t)&b, NULL);

	thrd_join(t1);
	thrd_join(t2);

	return 0;
}
