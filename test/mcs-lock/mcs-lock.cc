#include <stdio.h>
#include <threads.h>

#include "mcs-lock.h"


#include "librace.h"

struct mcs_mutex *mutex;
static uint32_t shared;

void threadA(void *arg)
{
	mcs_mutex::guard g(mutex);
	printf("store: %d\n", 17);
		shared = 17;
	mutex->unlock(&g);
	mutex->lock(&g);
		printf("load: %u\n", shared);
}

void threadB(void *arg)
{
	mcs_mutex::guard g(mutex);
		printf("load: %u\n", shared);
	mutex->unlock(&g);
	mutex->lock(&g);
	printf("store: %d\n", 17);
	shared = 17;
	}

int user_main(int argc, char **argv)
{
	thrd_t A, B;

	mutex = new mcs_mutex();

	thrd_create(&A, &threadA, NULL);
	thrd_create(&B, &threadB, NULL);
	thrd_join(A);
	thrd_join(B);
	return 0;
}

