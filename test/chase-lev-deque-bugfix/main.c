#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>

#include "model-assert.h"

#include "deque.h"

Deque *q;
int a;
int b;
int c;

static void task(void * param) {
		a=steal(q);
}

int user_main(int argc, char **argv)
{
	__sequential_init();
	
	thrd_t t;
	q=create();
	thrd_create(&t, task, 0);
	push(q, 1);
	push(q, 2);
	push(q, 4);
	b=take(q);
	c=take(q);
	thrd_join(t);

	
	return 0;
}

