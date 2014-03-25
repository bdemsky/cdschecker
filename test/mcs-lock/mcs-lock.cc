#include <stdio.h>
#include <threads.h>

#include "mcs-lock.h"


#include "librace.h"

struct mcs_mutex *mutex;
static uint32_t shared;

void threadA(void *arg)
{
	mcs_mutex::guard g(mutex);
			shared = 17;
	mutex->unlock(&g);
	mutex->lock(&g);
		}

void threadB(void *arg)
{
	mcs_mutex::guard g(mutex);
			mutex->unlock(&g);
	mutex->lock(&g);
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

