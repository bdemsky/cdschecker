#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>

#include "librace.h"

#include "wildcard.h"

atomic_int x;
atomic_int y;

/** Peterson Lock
  * This pattern involves two threads and two locations. The only solution for
  * this pattern is to make all ordering parameters seq_cst
  */

static void a(void *obj)
{
	atomic_store_explicit(&x, 1, memory_order_wildcard(1));
	int r1=atomic_load_explicit(&y, memory_order_wildcard(2));
}

static void b(void *obj)
{
	atomic_store_explicit(&y, 1, memory_order_wildcard(3));
	int r2=atomic_load_explicit(&x, memory_order_wildcard(4));
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2;

	atomic_init(&x, 0);
	atomic_init(&y, 0);

	printf("Main thread: creating 2 threads\n");
	thrd_create(&t1, (thrd_start_t)&a, NULL);
	thrd_create(&t2, (thrd_start_t)&b, NULL);

	thrd_join(t1);
	thrd_join(t2);
	printf("Main thread is finished\n");

	return 0;
}
