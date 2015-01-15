#include <stdlib.h>
#include <stdio.h>
#include <threads.h>

#include "my_queue.h"
#include "model-assert.h"

static int procs = 3;
static queue_t *queue;
static thrd_t *threads;
static unsigned int *input;
static unsigned int *output;
static int num_threads;

int get_thread_num()
{
	thrd_t curr = thrd_current();
	int i;
	for (i = 0; i < num_threads; i++)
		if (curr.priv == threads[i].priv)
			return i;
		return -1;
}


bool succ1, succ2;
unsigned int output1, output2;

static void main_task(void *param)
{
	int pid = *((int *)param);
	if (pid % 3 == 0) {
		output1 = 1;
		succ1 = dequeue(queue, &output1);
		if (succ1)
			printf("Thrd 2: Dequeue %d.\n", output1);
		else
			printf("Thrd 2: Dequeue NULL.\n");
	} else if (pid % 3 == 1) {
		output2 = 2;
		succ2 = dequeue(queue, &output2);
		if (succ2)
			printf("Thrd 3: Dequeue %d.\n", output2);
		else
			printf("Thrd 3: Dequeue NULL.\n");
	} else if (pid %3 == 2) {
		int input = 47;
		enqueue(queue, input);
		printf("Thrd 4 Enqueue %d.\n", input);
	}
}

int user_main(int argc, char **argv)
{
	__sequential_init();
	
	int i;
	int *param;
	unsigned int in_sum = 0, out_sum = 0;

	queue = calloc(1, sizeof(*queue));
	
	num_threads = procs;
	threads = malloc(num_threads * sizeof(thrd_t));
	param = malloc(num_threads * sizeof(*param));
	input = calloc(num_threads, sizeof(*input));
	output = calloc(num_threads, sizeof(*output));

	init_queue(queue, num_threads);
	for (i = 0; i < num_threads; i++) {
		param[i] = i;
		thrd_create(&threads[i], main_task, &param[i]);
	}
	for (i = 0; i < num_threads; i++)
		thrd_join(threads[i]);


	free(param);
	free(threads);
	free(queue);

	return 0;
}

