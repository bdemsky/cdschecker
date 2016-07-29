#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>

#include "librace.h"
#include "model-assert.h"

atomic_int x;
atomic_int y;
int r0, r1, r2;

static void a(void *obj)
{
	atomic_store_explicit(&x, 1, memory_order_relaxed);

	atomic_store_explicit(&y, 1, memory_order_relaxed);
	atomic_thread_fence(memory_order_seq_cst);
	r1 = atomic_load_explicit(&x, memory_order_relaxed);
	printf("r1=%d\n", r1);
}

static void b(void *obj)
{
	r0 = atomic_fetch_add_explicit(&x, 1, memory_order_seq_cst);
}

static void c(void *obj)
{
	atomic_thread_fence(memory_order_seq_cst);
	r2 = atomic_load_explicit(&y, memory_order_relaxed);
	printf("r0=%d, r2=%d\n", r0, r2);
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2, t3;

	atomic_init(&x, 0);
	atomic_init(&y, 0);

	printf("Main thread: creating 2 threads\n");
	thrd_create(&t1, (thrd_start_t)&a, NULL);
	thrd_create(&t2, (thrd_start_t)&b, NULL);
	thrd_create(&t3, (thrd_start_t)&c, NULL);

	thrd_join(t1);
	thrd_join(t2);
	thrd_join(t3);
	printf("Main thread is finishing\n");
	MODEL_ASSERT(!(r0 == 1 && r1 == 1 && r2 == 0));
	return 0;
}
