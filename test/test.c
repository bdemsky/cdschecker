#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>
 
#include "model-assert.h"

#include "librace.h"

atomic_int x;

static void a(void *obj)
{
	atomic_store_explicit(&x, 1, memory_order_relaxed);
}

static void b(void *obj)
{
	atomic_store_explicit(&x, 2, memory_order_relaxed);
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2;

	atomic_init(&x, 0);

	printf("Main thread: creating 2 threads\n");
	thrd_create(&t1, (thrd_start_t)&a, NULL);
	thrd_create(&t2, (thrd_start_t)&b, NULL);

	thrd_join(t1);
	thrd_join(t2);

	int r1=atomic_load_explicit(&x, memory_order_relaxed);
	int r2=atomic_load_explicit(&x, memory_order_relaxed);

	MODEL_ASSERT (r1 == r2);

	printf("Main thread is finished\n");

	return 0;
}
