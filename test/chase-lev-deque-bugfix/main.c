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
	if (a == ABORT) {
		printf("Steal NULL\n");
	} else {
		printf("Steal %d\n", a);
	}
}

int user_main(int argc, char **argv)
{
	__sequential_init();
	
	thrd_t t;
	q=create();
	thrd_create(&t, task, 0);
	push(q, 1);
	printf("Push 1\n");
	push(q, 2);
	printf("Push 2\n");
	push(q, 4);
	printf("Push 4\n");
	b=take(q);
	if (b == EMPTY) {
		printf("Take NULL\n");
	} else {
		printf("Take %d\n", b);
	}
	c=take(q);
	if (c == EMPTY) {
		printf("Take NULL\n");
	} else {
		printf("Take %d\n", c);
	}
	thrd_join(t);

	
	return 0;
}

