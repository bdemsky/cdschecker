#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>

#include "librace.h"

atomic_int x;
atomic_int y;

static void a(void *obj)
{
    atomic_store_explicit(&x, 1, memory_order_seq_cst);
    atomic_store_explicit(&y, 1, memory_order_relaxed);
}

static void b(void *obj)
{
    atomic_store_explicit(&x, 2, memory_order_seq_cst);
    int r1=atomic_load_explicit(&y, memory_order_relaxed);
	if (r1 == 1) {
    	int r2=atomic_store_explicit(&x, 1, memory_order_relaxed);
		printf("r2=%d\n", r2);
	}
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
