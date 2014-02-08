#include <atomic>
#include <threads.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>

#include "librace.h"

using namespace std;

/**
	This is an example about how to specify the correctness of the
	read-copy-update synchronization mechanism.
*/

typedef struct Data {
	int data1;
	int data2;
	int data3;
} Data;

atomic<Data*> data;
	
/**
	@Begin
	@Global_define:
		@DeclareVar:
			Data *_cur_data;
		@InitVar:
			_cur_data = NULL;
		
		@DefineFunc:
		bool equals(Data *ptr1, Data *ptr2) {
			if (ptr1->data1 == ptr2->data2
				&& ptr1->data2 == ptr2->data2
				&& ptr1->data3 == ptr2->data3) {
				return true;
			} else {
				return false;
			}
		}

	@Happens_before:
		Write -> Read
		Write -> Write
	@End
*/

/**
	@Begin
	@Interface: Read
	@Commit_point_set: Read_Success_Point
	@Action:
		Data *_Old_Data = _cur_data;
	@Post_check:
		equals(__RET__, _Old_Data)
	@End
*/
Data* read() {
	Data *res = data.load(memory_order_acquire);
	/**
		@Begin
		@Commit_point_define_check: true
		@Label: Read_Success_Point
		@End
	*/
	return res;
}

/**
	@Begin
	@Interface: Write
	@Commit_point_set: Write_Success_Point
	@Action:
		_cur_data = new_data;
	@End
*/
void write(Data *new_data) {
	while (true) {
		Data *prev = data.load(memory_order_relaxed);
		bool succ = data.compare_exchange_strong(prev, new_data,
			memory_order_release, memory_order_relaxed); 
		/**
			@Begin
			@Commit_point_define_check: succ == true
			@Label: Write_Success_Point
			@End
		*/
		if (succ) {
			break;
		}
	}
}

void threadA(void *arg) {
	Data *dataA = (Data*) malloc(sizeof(Data));
	dataA->data1 = 3;
	dataA->data2 = 2;
	dataA->data3 = 1;
	write(dataA);
}

void threadB(void *arg) {
	Data *dataB = read();
	printf("ThreadB data1: %d\n", dataB->data1);
	printf("ThreadB data2: %d\n", dataB->data2);
	printf("ThreadB data3: %d\n", dataB->data3);
}

int user_main(int argc, char **argv) {
	/**
		@Begin
		@Entry_point
		@End
	*/
	
	thrd_t t1, t2;
	Data *data_init = (Data*) malloc(sizeof(Data));
	data_init->data1 = 1;
	data_init->data2 = 2;
	data_init->data3 = 3;
	write(data_init);

	thrd_create(&t1, threadA, NULL);
	thrd_create(&t2, threadB, NULL);

	thrd_join(t1);
	thrd_join(t2);

	return 0;
}
