/*
 * This test performs some relaxes, release, acquire opeations on a single
 * atomic variable. It is designed for creating a difficult set of pending
 * release sequences to resolve at the end of an execution. However, it
 * utilizes 6 threads, so it blows up into a lot of executions quickly.
 */

#include <stdio.h>

#include "libthreads.h"
#include "librace.h"
#include "stdatomic.h"

atomic_int x;

static void a(void *obj)
{
	atomic_store_explicit(&x, *((int *)obj), memory_order_release);
	atomic_store_explicit(&x, *((int *)obj) + 1, memory_order_relaxed);
}

static void b2(void *obj)
{
	int r = atomic_load_explicit(&x, memory_order_acquire);
	printf("r = %u\n", r);
}

static void b1(void *obj)
{
	thrd_t t3, t4;
	int i = 7;
	int r = atomic_load_explicit(&x, memory_order_acquire);
	printf("r = %u\n", r);
	thrd_create(&t3, (thrd_start_t)&a, &i);
	thrd_create(&t4, (thrd_start_t)&b2, NULL);
	thrd_join(t3);
	thrd_join(t4);
}

static void c(void *obj)
{
	atomic_store_explicit(&x, 22, memory_order_relaxed);
}

void user_main()
{
	thrd_t t1, t2, t5;
	int i = 4;

	atomic_init(&x, 0);

	thrd_create(&t1, (thrd_start_t)&a, &i);
	thrd_create(&t2, (thrd_start_t)&b1, NULL);
	thrd_create(&t5, (thrd_start_t)&c, NULL);

	thrd_join(t1);
	thrd_join(t2);
	thrd_join(t5);
}
