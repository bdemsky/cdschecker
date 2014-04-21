#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>
#include <model-assert.h>

#include "librace.h"

atomic_int A;
atomic_int B;
atomic_int C;

int r1, r2;

static void a(void *obj)
{
	atomic_store_explicit(&A, 1, memory_order_relaxed);
	atomic_load_explicit(&A, memory_order_relaxed);
}

static void b(void *obj)
{
	r1 = atomic_load_explicit(&A, memory_order_relaxed);
	atomic_store_explicit(&A, 2, memory_order_relaxed);
}

static void c(void *obj)
{
	r2 = atomic_load_explicit(&A, memory_order_relaxed);
	atomic_store_explicit(&A, 3, memory_order_relaxed);
}

static void d(void *obj)
{
	while (atomic_load_explicit(&B, memory_order_seq_cst) != 1) ;
	while (atomic_load_explicit(&C, memory_order_seq_cst) != 1) ;
	r2 = atomic_load_explicit(&A, memory_order_seq_cst);
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2, t3, t4;

	atomic_init(&A, 0);
	atomic_init(&B, 0);
	atomic_init(&C, 0);

	//thrd_create(&t1, (thrd_start_t)&a, NULL);
	thrd_create(&t2, (thrd_start_t)&b, NULL);
	thrd_create(&t3, (thrd_start_t)&c, NULL);
	//thrd_create(&t4, (thrd_start_t)&d, NULL);

	//thrd_join(t1);
	thrd_join(t2);
	thrd_join(t3);
	//thrd_join(t4);

	MODEL_ASSERT(!(r1 == 3 && r2 == 2));

	return 0;
}
