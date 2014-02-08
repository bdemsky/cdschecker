#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>

#include <spec_lib.h>
#include "librace.h"
#include "common.h"

atomic_int x;
atomic_int y;

static void a(void *obj)
{
	int r1=atomic_load_explicit(&y, memory_order_relaxed);
	atomic_store_explicit(&x, r1, memory_order_relaxed);
	printf("r1=%d\n",r1);
}

static void b(void *obj)
{
	int r2=atomic_load_explicit(&x, memory_order_relaxed);
	atomic_store_explicit(&y, 42, memory_order_relaxed);
	printf("r2=%d\n",r2);
}

bool equals(void *o1, void *o2) {
	if (o1 == NULL || o2 == NULL)
		return false;
	int *i1 = (int*) o1,
		*i2 = (int*) o2;
	if (*i1 == *i2)
		return true;
	else
		return false;
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2;

	spec_table *table = new_spec_table_default(equals);
	int k1 = 3, k2 = 4, v1 = 1, v2 = 6;
	spec_table_put(table, &k1, &v1);
	spec_table_put(table, &k2, &v2);
	
	int *res = (int*) spec_table_get(table, &k1);
	if (res != NULL) {
		model_print("%d -> %d\n", k1, *res);
	}
	model_print("size: %d\n", table->size);
	
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
