/*
 * Multiple readers, multiple writers
 */
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

#include "librace.h"

atomic_int x;

static int N = 2;

static void a(void *obj)
{
	int i;
	for (i = 0 ; i < N; i++)
		printf("r%d = %d\n", i, atomic_load_explicit(&x, memory_order_relaxed));
}

static void b(void *obj)
{
	int i;
	for (i = 0 ; i < N; i++)
		atomic_store_explicit(&x, i + 1, memory_order_relaxed);
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2;

	if (argc > 1)
		N = atoi(argv[1]);

	atomic_init(&x, 0);

	printf("Main thread: creating 2 threads\n");
	thrd_create(&t1, (thrd_start_t)&a, NULL);
	thrd_create(&t2, (thrd_start_t)&b, NULL);

	thrd_join(t1);
	thrd_join(t2);
	printf("Main thread is finished\n");

	return 0;
}
