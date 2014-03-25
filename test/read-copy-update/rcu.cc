#include <atomic>
#include <threads.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>

#include <spec_lib.h>
#include <stdlib.h>
#include <cdsannotate.h>
#include <specannotation.h>
#include <model_memory.h>
#include "common.h" 

#include "librace.h"



typedef struct Data {
	int data1;
	int data2;
	int data3;
} Data;

atomic<Data*> data;
	
/* All other user-defined structs */
static int data1;
static int data2;
static int data3;
/* All other user-defined functions */
/* Definition of interface info struct: Write */
typedef struct Write_info {
Data * __RET__;
int d1;
int d2;
int d3;
} Write_info;
/* End of info struct definition: Write */

/* ID function of interface: Write */
inline static call_id_t Write_id(void *info, thread_id_t __TID__) {
	Write_info* theInfo = (Write_info*)info;
	Data * __RET__ = theInfo->__RET__;
	int d1 = theInfo->d1;
	int d2 = theInfo->d2;
	int d3 = theInfo->d3;

	call_id_t __ID__ = 0;
	return __ID__;
}
/* End of ID function: Write */

/* Check action function of interface: Write */
inline static bool Write_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Write_info* theInfo = (Write_info*)info;
	Data * __RET__ = theInfo->__RET__;
	int d1 = theInfo->d1;
	int d2 = theInfo->d2;
	int d3 = theInfo->d3;

	data1 += d1 ;
	data2 += d2 ;
	data3 += d3 ;
	return true;
}
/* End of check action function: Write */

/* Definition of interface info struct: Read */
typedef struct Read_info {
Data * __RET__;
} Read_info;
/* End of info struct definition: Read */

/* ID function of interface: Read */
inline static call_id_t Read_id(void *info, thread_id_t __TID__) {
	Read_info* theInfo = (Read_info*)info;
	Data * __RET__ = theInfo->__RET__;

	call_id_t __ID__ = 0;
	return __ID__;
}
/* End of ID function: Read */

/* Check action function of interface: Read */
inline static bool Read_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Read_info* theInfo = (Read_info*)info;
	Data * __RET__ = theInfo->__RET__;

	check_passed = data1 == __RET__ -> data1 && data2 == __RET__ -> data2 && data3 == __RET__ -> data3;
	if (!check_passed)
		return false;
	return true;
}
/* End of check action function: Read */

#define INTERFACE_SIZE 2
static void** func_ptr_table;
static anno_hb_init** hb_init_table;

/* Define function for sequential code initialization */
inline static void __sequential_init() {
	/* Init func_ptr_table */
	func_ptr_table = (void**) malloc(sizeof(void*) * 2 * 2);
	func_ptr_table[2 * 1] = (void*) &Write_id;
	func_ptr_table[2 * 1 + 1] = (void*) &Write_check_action;
	func_ptr_table[2 * 0] = (void*) &Read_id;
	func_ptr_table[2 * 0 + 1] = (void*) &Read_check_action;
	/* Write(true) -> Read(true) */
	struct anno_hb_init *hbConditionInit0 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit0->interface_num_before = 1; // Write
	hbConditionInit0->hb_condition_num_before = 0; // 
	hbConditionInit0->interface_num_after = 0; // Read
	hbConditionInit0->hb_condition_num_after = 0; // 
	/* Write(true) -> Write(true) */
	struct anno_hb_init *hbConditionInit1 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit1->interface_num_before = 1; // Write
	hbConditionInit1->hb_condition_num_before = 0; // 
	hbConditionInit1->interface_num_after = 1; // Write
	hbConditionInit1->hb_condition_num_after = 0; // 
	/* Init hb_init_table */
	hb_init_table = (anno_hb_init**) malloc(sizeof(anno_hb_init*) * 2);
	#define HB_INIT_TABLE_SIZE 2
	hb_init_table[0] = hbConditionInit0;
	hb_init_table[1] = hbConditionInit1;
	/* Pass init info, including function table info & HB rules */
	struct anno_init *anno_init = (struct anno_init*) malloc(sizeof(struct anno_init));
	anno_init->func_table = func_ptr_table;
	anno_init->func_table_size = INTERFACE_SIZE;
	anno_init->hb_init_table = hb_init_table;
	anno_init->hb_init_table_size = HB_INIT_TABLE_SIZE;
	struct spec_annotation *init = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	init->type = INIT;
	init->annotation = anno_init;
	cdsannotate(SPEC_ANALYSIS, init);

	data1 = 0 ;
	data2 = 0 ;
	data3 = 0 ;
}
/* End of Global construct generation in class */


Data * __wrapper__read();

Data * read() {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 0; // Read
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	Data * __RET__ = __wrapper__read();
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 0; // Read
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Read_info* info = (Read_info*) malloc(sizeof(Read_info));
	info->__RET__ = __RET__;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 0; // Read
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
	return __RET__;
}

Data * __wrapper__read() {
	Data *res = data.load(memory_order_acquire);
	/* Automatically generated code for commit point define check: Read_Success_Point */

	if (true) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 0;
		cp_define_check->interface_num = 0;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
		
		return res;
}

Data * __wrapper__write(int d1, int d2, int d3);

Data * write(int d1, int d2, int d3) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 1; // Write
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	Data * __RET__ = __wrapper__write(d1, d2, d3);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 1; // Write
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Write_info* info = (Write_info*) malloc(sizeof(Write_info));
	info->__RET__ = __RET__;
	info->d1 = d1;
	info->d2 = d2;
	info->d3 = d3;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 1; // Write
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
	return __RET__;
}

Data * __wrapper__write(int d1, int d2, int d3) {
	bool succ = false;
	Data *tmp = (Data*) malloc(sizeof(Data));
	do {
		Data *prev = data.load(memory_order_acquire);
                tmp->data1 = prev->data1 + d1;
	    tmp->data2 = prev->data2 + d2;
	    tmp->data3 = prev->data3 + d3;
        succ = data.compare_exchange_strong(prev, tmp,
            memory_order_acq_rel, memory_order_relaxed);
	/* Automatically generated code for commit point define check: Write_Success_Point */

	if (succ) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 1;
		cp_define_check->interface_num = 1;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
		
	} while (!succ);
		return tmp;
}


void threadA(void *arg) {
	write(1, 0, 0);
}

void threadB(void *arg) {
	write(0, 1, 0);
}

void threadC(void *arg) {
	write(0, 0, 1);
}

void threadD(void *arg) {
	Data *dataC = read();
}

int user_main(int argc, char **argv) {
	__sequential_init();
	
	
	thrd_t t1, t2, t3, t4;
	Data *dataInit = (Data*) malloc(sizeof(Data));
	dataInit->data1 = 0;
	dataInit->data2 = 0;
	dataInit->data3 = 0;
	atomic_init(&data, dataInit);
	write(0, 0, 0);

	thrd_create(&t1, threadA, NULL);
	thrd_create(&t2, threadB, NULL);
	thrd_create(&t3, threadC, NULL);
	thrd_create(&t4, threadD, NULL);

	thrd_join(t1);
	thrd_join(t2);
	thrd_join(t3);
	thrd_join(t4);

	return 0;
}

