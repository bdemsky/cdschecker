#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>

#include "librace.h"
#include "wildcard.h"

atomic_int x;
atomic_int y;
atomic_int z;

static int r1, r2, r3;

static void a(void *obj)
{
	atomic_store_explicit(&z, 1, wildcard(1)); // SC
	atomic_store_explicit(&x, 1, wildcard(2)); // SC
	atomic_store_explicit(&y, 1, wildcard(3)); // release
}

static void b(void *obj)
{
	atomic_store_explicit(&x, 2, wildcard(4)); // SC
    // It can read from the old z=0
	r3=atomic_load_explicit(&z, wildcard(5)); // SC
}
static void c(void *obj)
{
    // It reads from 'w3'
	r1=atomic_load_explicit(&y, wildcard(6)); // acquire
    // It reads from 'w4'
	r2=atomic_load_explicit(&x, wildcard(7)); // relaxed
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2,t3;

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
