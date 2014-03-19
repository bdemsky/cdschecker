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

static void task1(void * param) {
	a=steal(q);
}

static void task2(void * param) {
	a=take(q);
}

int user_main(int argc, char **argv)
{
	__sequential_init();
	
	thrd_t t1, t2;
	q=create();
	push(q, 1);
	push(q, 2);
	thrd_create(&t1, task1, 0);
	thrd_create(&t2, task2, 0);
	push(q, 4);
	b=take(q);
	c=take(q);
	thrd_join(t1);
	thrd_join(t2);

	bool correct=true;
	if (a!=1 && a!=2 && a!=4 && a!= EMPTY)
		correct=false;
	if (b!=1 && b!=2 && b!=4 && b!= EMPTY)
		correct=false;
	if (c!=1 && c!=2 && c!=4 && a!= EMPTY)
		correct=false;
	if (a!=EMPTY && b!=EMPTY && c!=EMPTY && (a+b+c)!=7)
		correct=false;
	if (!correct)
		printf("a=%d b=%d c=%d\n",a,b,c);
	
	return 0;
}

