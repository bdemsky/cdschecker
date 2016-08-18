#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>

#include "librace.h"
#include "wildcard.h"

atomic_int x;
atomic_int y;

static int r1, r2, r3;

static void a(void *obj)
{
	atomic_store_explicit(&y, 1, relaxed); // SC
	atomic_store_explicit(&y, 2, release); // SC
}

static void b(void *obj)
{
	r1=atomic_load_explicit(&y, seqcst); // SC
	r2=atomic_load_explicit(&x, seqcst); // SC
}
static void c(void *obj)
{
	atomic_store_explicit(&x, 1, seqcst); // SC
	r3=atomic_load_explicit(&y, seqcst); // SC 
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2,t3;

	atomic_init(&x, 0);
	atomic_init(&y, 0);

	thrd_create(&t1, (thrd_start_t)&a, NULL);
	thrd_create(&t2, (thrd_start_t)&b, NULL);
	thrd_create(&t3, (thrd_start_t)&c, NULL);

	thrd_join(t1);
	thrd_join(t2);
	thrd_join(t3);

	return 0;
}
