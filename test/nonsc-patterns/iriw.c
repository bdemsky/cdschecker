#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>

#include "librace.h"

#include "wildcard.h"

atomic_int x;
atomic_int y;

/** Independent Reads & Independent Writes
  * This pattern contains four threads and two different memory locations. The
  * only solution for this pattern is to make read1 and read3 acquire and all
  * other operations seq_cst
  */

static void a(void *obj)
{
	atomic_store_explicit(&x, 1, memory_order_seq_cst);
}

static void b(void *obj)
{
	atomic_store_explicit(&y, 1, memory_order_seq_cst);
}

static void c(void *obj)
{
	int r1=atomic_load_explicit(&x, memory_order_relaxed);
	int r2=atomic_load_explicit(&y, memory_order_seq_cst);
}

static void d(void *obj)
{
	int r3=atomic_load_explicit(&y, memory_order_acquire);
	int r4=atomic_load_explicit(&x, memory_order_seq_cst);
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
