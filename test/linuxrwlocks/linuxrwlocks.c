#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>

#include <spec_lib.h>
#include <stdlib.h>
#include <cdsannotate.h>
#include <specannotation.h>
#include <model_memory.h>
#include "common.h"

#include "librace.h"

#define RW_LOCK_BIAS            0x00100000
#define WRITE_LOCK_CMP          RW_LOCK_BIAS

typedef union {
	atomic_int lock;
} rwlock_t;







/* All other user-defined structs */
static bool writer_lock_acquired;
static int reader_lock_cnt;
/* All other user-defined functions */
/* Definition of interface info struct: Write_Trylock */
typedef struct Write_Trylock_info {
int __RET__;
rwlock_t * rw;
} Write_Trylock_info;
/* End of info struct definition: Write_Trylock */

/* ID function of interface: Write_Trylock */
inline static call_id_t Write_Trylock_id(void *info, thread_id_t __TID__) {
	Write_Trylock_info* theInfo = (Write_Trylock_info*)info;
	int __RET__ = theInfo->__RET__;
	rwlock_t * rw = theInfo->rw;

	call_id_t __ID__ = 0;
	return __ID__;
}
/* End of ID function: Write_Trylock */

/* Check action function of interface: Write_Trylock */
inline static bool Write_Trylock_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Write_Trylock_info* theInfo = (Write_Trylock_info*)info;
	int __RET__ = theInfo->__RET__;
	rwlock_t * rw = theInfo->rw;

	if ( __RET__ == 1 ) writer_lock_acquired = true ;
	return true;
}
/* End of check action function: Write_Trylock */

/* Definition of interface info struct: Read_Trylock */
typedef struct Read_Trylock_info {
int __RET__;
rwlock_t * rw;
} Read_Trylock_info;
/* End of info struct definition: Read_Trylock */

/* ID function of interface: Read_Trylock */
inline static call_id_t Read_Trylock_id(void *info, thread_id_t __TID__) {
	Read_Trylock_info* theInfo = (Read_Trylock_info*)info;
	int __RET__ = theInfo->__RET__;
	rwlock_t * rw = theInfo->rw;

	call_id_t __ID__ = 0;
	return __ID__;
}
/* End of ID function: Read_Trylock */

/* Check action function of interface: Read_Trylock */
inline static bool Read_Trylock_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Read_Trylock_info* theInfo = (Read_Trylock_info*)info;
	int __RET__ = theInfo->__RET__;
	rwlock_t * rw = theInfo->rw;

	bool __COND_SAT__ = writer_lock_acquired == false;
	if ( __COND_SAT__ ) reader_lock_cnt ++ ;
	check_passed = __COND_SAT__ ? __RET__ == 1 : __RET__ == 0;
	if (!check_passed)
		return false;
	return true;
}
/* End of check action function: Read_Trylock */

/* Definition of interface info struct: Write_Lock */
typedef struct Write_Lock_info {
rwlock_t * rw;
} Write_Lock_info;
/* End of info struct definition: Write_Lock */

/* ID function of interface: Write_Lock */
inline static call_id_t Write_Lock_id(void *info, thread_id_t __TID__) {
	Write_Lock_info* theInfo = (Write_Lock_info*)info;
	rwlock_t * rw = theInfo->rw;

	call_id_t __ID__ = 0;
	return __ID__;
}
/* End of ID function: Write_Lock */

/* Check action function of interface: Write_Lock */
inline static bool Write_Lock_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Write_Lock_info* theInfo = (Write_Lock_info*)info;
	rwlock_t * rw = theInfo->rw;

	check_passed = ! writer_lock_acquired && reader_lock_cnt == 0;
	if (!check_passed)
		return false;
	writer_lock_acquired = true ;
	return true;
}
/* End of check action function: Write_Lock */

/* Definition of interface info struct: Write_Unlock */
typedef struct Write_Unlock_info {
rwlock_t * rw;
} Write_Unlock_info;
/* End of info struct definition: Write_Unlock */

/* ID function of interface: Write_Unlock */
inline static call_id_t Write_Unlock_id(void *info, thread_id_t __TID__) {
	Write_Unlock_info* theInfo = (Write_Unlock_info*)info;
	rwlock_t * rw = theInfo->rw;

	call_id_t __ID__ = 0;
	return __ID__;
}
/* End of ID function: Write_Unlock */

/* Check action function of interface: Write_Unlock */
inline static bool Write_Unlock_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Write_Unlock_info* theInfo = (Write_Unlock_info*)info;
	rwlock_t * rw = theInfo->rw;

	check_passed = reader_lock_cnt == 0 && writer_lock_acquired;
	if (!check_passed)
		return false;
	writer_lock_acquired = false ;
	return true;
}
/* End of check action function: Write_Unlock */

/* Definition of interface info struct: Read_Unlock */
typedef struct Read_Unlock_info {
rwlock_t * rw;
} Read_Unlock_info;
/* End of info struct definition: Read_Unlock */

/* ID function of interface: Read_Unlock */
inline static call_id_t Read_Unlock_id(void *info, thread_id_t __TID__) {
	Read_Unlock_info* theInfo = (Read_Unlock_info*)info;
	rwlock_t * rw = theInfo->rw;

	call_id_t __ID__ = 0;
	return __ID__;
}
/* End of ID function: Read_Unlock */

/* Check action function of interface: Read_Unlock */
inline static bool Read_Unlock_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Read_Unlock_info* theInfo = (Read_Unlock_info*)info;
	rwlock_t * rw = theInfo->rw;

	check_passed = reader_lock_cnt > 0 && ! writer_lock_acquired;
	if (!check_passed)
		return false;
	reader_lock_cnt -- ;
	return true;
}
/* End of check action function: Read_Unlock */

/* Definition of interface info struct: Read_Lock */
typedef struct Read_Lock_info {
rwlock_t * rw;
} Read_Lock_info;
/* End of info struct definition: Read_Lock */

/* ID function of interface: Read_Lock */
inline static call_id_t Read_Lock_id(void *info, thread_id_t __TID__) {
	Read_Lock_info* theInfo = (Read_Lock_info*)info;
	rwlock_t * rw = theInfo->rw;

	call_id_t __ID__ = 0;
	return __ID__;
}
/* End of ID function: Read_Lock */

/* Check action function of interface: Read_Lock */
inline static bool Read_Lock_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Read_Lock_info* theInfo = (Read_Lock_info*)info;
	rwlock_t * rw = theInfo->rw;

	check_passed = ! writer_lock_acquired;
	if (!check_passed)
		return false;
	reader_lock_cnt ++ ;
	return true;
}
/* End of check action function: Read_Lock */

#define INTERFACE_SIZE 6
static void** func_ptr_table;
static anno_hb_init** hb_init_table;

/* Define function for sequential code initialization */
inline static void __sequential_init() {
	/* Init func_ptr_table */
	func_ptr_table = (void**) malloc(sizeof(void*) * 6 * 2);
	func_ptr_table[2 * 3] = (void*) &Write_Trylock_id;
	func_ptr_table[2 * 3 + 1] = (void*) &Write_Trylock_check_action;
	func_ptr_table[2 * 2] = (void*) &Read_Trylock_id;
	func_ptr_table[2 * 2 + 1] = (void*) &Read_Trylock_check_action;
	func_ptr_table[2 * 1] = (void*) &Write_Lock_id;
	func_ptr_table[2 * 1 + 1] = (void*) &Write_Lock_check_action;
	func_ptr_table[2 * 5] = (void*) &Write_Unlock_id;
	func_ptr_table[2 * 5 + 1] = (void*) &Write_Unlock_check_action;
	func_ptr_table[2 * 4] = (void*) &Read_Unlock_id;
	func_ptr_table[2 * 4 + 1] = (void*) &Read_Unlock_check_action;
	func_ptr_table[2 * 0] = (void*) &Read_Lock_id;
	func_ptr_table[2 * 0 + 1] = (void*) &Read_Lock_check_action;
	/* Read_Unlock(true) -> Write_Lock(true) */
	struct anno_hb_init *hbConditionInit0 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit0->interface_num_before = 4; // Read_Unlock
	hbConditionInit0->hb_condition_num_before = 0; // 
	hbConditionInit0->interface_num_after = 1; // Write_Lock
	hbConditionInit0->hb_condition_num_after = 0; // 
	/* Read_Unlock(true) -> Write_Trylock(HB_Write_Trylock_Succ) */
	struct anno_hb_init *hbConditionInit1 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit1->interface_num_before = 4; // Read_Unlock
	hbConditionInit1->hb_condition_num_before = 0; // 
	hbConditionInit1->interface_num_after = 3; // Write_Trylock
	hbConditionInit1->hb_condition_num_after = 2; // HB_Write_Trylock_Succ
	/* Read_Unlock(true) -> Read_Lock(true) */
	struct anno_hb_init *hbConditionInit2 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit2->interface_num_before = 4; // Read_Unlock
	hbConditionInit2->hb_condition_num_before = 0; // 
	hbConditionInit2->interface_num_after = 0; // Read_Lock
	hbConditionInit2->hb_condition_num_after = 0; // 
	/* Read_Unlock(true) -> Read_Trylock(HB_Read_Trylock_Succ) */
	struct anno_hb_init *hbConditionInit3 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit3->interface_num_before = 4; // Read_Unlock
	hbConditionInit3->hb_condition_num_before = 0; // 
	hbConditionInit3->interface_num_after = 2; // Read_Trylock
	hbConditionInit3->hb_condition_num_after = 3; // HB_Read_Trylock_Succ
	/* Write_Unlock(true) -> Write_Lock(true) */
	struct anno_hb_init *hbConditionInit4 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit4->interface_num_before = 5; // Write_Unlock
	hbConditionInit4->hb_condition_num_before = 0; // 
	hbConditionInit4->interface_num_after = 1; // Write_Lock
	hbConditionInit4->hb_condition_num_after = 0; // 
	/* Write_Unlock(true) -> Write_Trylock(HB_Write_Trylock_Succ) */
	struct anno_hb_init *hbConditionInit5 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit5->interface_num_before = 5; // Write_Unlock
	hbConditionInit5->hb_condition_num_before = 0; // 
	hbConditionInit5->interface_num_after = 3; // Write_Trylock
	hbConditionInit5->hb_condition_num_after = 2; // HB_Write_Trylock_Succ
	/* Write_Unlock(true) -> Read_Lock(true) */
	struct anno_hb_init *hbConditionInit6 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit6->interface_num_before = 5; // Write_Unlock
	hbConditionInit6->hb_condition_num_before = 0; // 
	hbConditionInit6->interface_num_after = 0; // Read_Lock
	hbConditionInit6->hb_condition_num_after = 0; // 
	/* Write_Unlock(true) -> Read_Trylock(HB_Read_Trylock_Succ) */
	struct anno_hb_init *hbConditionInit7 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit7->interface_num_before = 5; // Write_Unlock
	hbConditionInit7->hb_condition_num_before = 0; // 
	hbConditionInit7->interface_num_after = 2; // Read_Trylock
	hbConditionInit7->hb_condition_num_after = 3; // HB_Read_Trylock_Succ
	/* Init hb_init_table */
	hb_init_table = (anno_hb_init**) malloc(sizeof(anno_hb_init*) * 8);
	#define HB_INIT_TABLE_SIZE 8
	hb_init_table[0] = hbConditionInit0;
	hb_init_table[1] = hbConditionInit1;
	hb_init_table[2] = hbConditionInit2;
	hb_init_table[3] = hbConditionInit3;
	hb_init_table[4] = hbConditionInit4;
	hb_init_table[5] = hbConditionInit5;
	hb_init_table[6] = hbConditionInit6;
	hb_init_table[7] = hbConditionInit7;
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

	writer_lock_acquired = false ;
	reader_lock_cnt = 0 ;
}
/* End of Global construct generation in class */


static inline int read_can_lock(rwlock_t *lock)
{
	return atomic_load_explicit(&lock->lock, memory_order_relaxed) > 0;
}

static inline int write_can_lock(rwlock_t *lock)
{
	return atomic_load_explicit(&lock->lock, memory_order_relaxed) == RW_LOCK_BIAS;
}


void __wrapper__read_lock(rwlock_t * rw);

void read_lock(rwlock_t * rw) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 0; // Read_Lock
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	__wrapper__read_lock(rw);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 0; // Read_Lock
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Read_Lock_info* info = (Read_Lock_info*) malloc(sizeof(Read_Lock_info));
	info->rw = rw;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 0; // Read_Lock
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
}

void __wrapper__read_lock(rwlock_t * rw)
{
	int priorvalue = atomic_fetch_sub_explicit(&rw->lock, 1, memory_order_acquire);
	/* Automatically generated code for commit point define check: Read_Lock_Success_1 */

	if (priorvalue > 0) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 0;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
	
	while (priorvalue <= 0) {
		atomic_fetch_add_explicit(&rw->lock, 1, memory_order_relaxed);
		while (atomic_load_explicit(&rw->lock, memory_order_relaxed) <= 0) {
			thrd_yield();
		}
		priorvalue = atomic_fetch_sub_explicit(&rw->lock, 1, memory_order_acquire);
	/* Automatically generated code for commit point define check: Read_Lock_Success_2 */

	if (priorvalue > 0) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 1;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
		
	}
}


void __wrapper__write_lock(rwlock_t * rw);

void write_lock(rwlock_t * rw) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 1; // Write_Lock
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	__wrapper__write_lock(rw);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 1; // Write_Lock
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Write_Lock_info* info = (Write_Lock_info*) malloc(sizeof(Write_Lock_info));
	info->rw = rw;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 1; // Write_Lock
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
}

void __wrapper__write_lock(rwlock_t * rw)
{
	int priorvalue = atomic_fetch_sub_explicit(&rw->lock, RW_LOCK_BIAS, memory_order_acquire);
	/* Automatically generated code for commit point define check: Write_Lock_Success_1 */

	if (priorvalue == RW_LOCK_BIAS) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 2;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
	
	while (priorvalue != RW_LOCK_BIAS) {
		atomic_fetch_add_explicit(&rw->lock, RW_LOCK_BIAS, memory_order_relaxed);
		while (atomic_load_explicit(&rw->lock, memory_order_relaxed) != RW_LOCK_BIAS) {
			thrd_yield();
		}
		priorvalue = atomic_fetch_sub_explicit(&rw->lock, RW_LOCK_BIAS, memory_order_acquire);
	/* Automatically generated code for commit point define check: Write_Lock_Success_2 */

	if (priorvalue == RW_LOCK_BIAS) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 3;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
		
	}
}

int __wrapper__read_trylock(rwlock_t * rw);

int read_trylock(rwlock_t * rw) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 2; // Read_Trylock
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	int __RET__ = __wrapper__read_trylock(rw);
	if (__RET__ == 1) {
		struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
		hb_condition->interface_num = 2; // Read_Trylock
		hb_condition->hb_condition_num = 3;
		struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_hb_condition->type = HB_CONDITION;
		annotation_hb_condition->annotation = hb_condition;
		cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);
	}

	Read_Trylock_info* info = (Read_Trylock_info*) malloc(sizeof(Read_Trylock_info));
	info->__RET__ = __RET__;
	info->rw = rw;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 2; // Read_Trylock
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
	return __RET__;
}

int __wrapper__read_trylock(rwlock_t * rw)
{
	int priorvalue = atomic_fetch_sub_explicit(&rw->lock, 1, memory_order_acquire);
	/* Automatically generated code for potential commit point: Potential_Read_Trylock_Point */

	if (true) {
		struct anno_potential_cp_define *potential_cp_define = (struct anno_potential_cp_define*) malloc(sizeof(struct anno_potential_cp_define));
		potential_cp_define->label_num = 4;
		struct spec_annotation *annotation_potential_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_potential_cp_define->type = POTENTIAL_CP_DEFINE;
		annotation_potential_cp_define->annotation = potential_cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_potential_cp_define);
	}
	
	if (priorvalue > 0) {
	/* Automatically generated code for commit point define: Read_Trylock_Succ_Point */

	if (true) {
		struct anno_cp_define *cp_define = (struct anno_cp_define*) malloc(sizeof(struct anno_cp_define));
		cp_define->label_num = 5;
		cp_define->potential_cp_label_num = 4;
		struct spec_annotation *annotation_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define->type = CP_DEFINE;
		annotation_cp_define->annotation = cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define);
	}
		
		return 1;
	}
	/* Automatically generated code for commit point define: Read_Trylock_Fail_Point */

	if (true) {
		struct anno_cp_define *cp_define = (struct anno_cp_define*) malloc(sizeof(struct anno_cp_define));
		cp_define->label_num = 6;
		cp_define->potential_cp_label_num = 4;
		struct spec_annotation *annotation_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define->type = CP_DEFINE;
		annotation_cp_define->annotation = cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define);
	}
	
	atomic_fetch_add_explicit(&rw->lock, 1, memory_order_relaxed);
	return 0;
}

int __wrapper__write_trylock(rwlock_t * rw);

int write_trylock(rwlock_t * rw) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 3; // Write_Trylock
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	int __RET__ = __wrapper__write_trylock(rw);
	if (__RET__ == 1) {
		struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
		hb_condition->interface_num = 3; // Write_Trylock
		hb_condition->hb_condition_num = 2;
		struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_hb_condition->type = HB_CONDITION;
		annotation_hb_condition->annotation = hb_condition;
		cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);
	}

	Write_Trylock_info* info = (Write_Trylock_info*) malloc(sizeof(Write_Trylock_info));
	info->__RET__ = __RET__;
	info->rw = rw;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 3; // Write_Trylock
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
	return __RET__;
}

int __wrapper__write_trylock(rwlock_t * rw)
{
	int priorvalue = atomic_fetch_sub_explicit(&rw->lock, RW_LOCK_BIAS, memory_order_acquire);
	/* Automatically generated code for potential commit point: Potential_Write_Trylock_Point */

	if (true) {
		struct anno_potential_cp_define *potential_cp_define = (struct anno_potential_cp_define*) malloc(sizeof(struct anno_potential_cp_define));
		potential_cp_define->label_num = 7;
		struct spec_annotation *annotation_potential_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_potential_cp_define->type = POTENTIAL_CP_DEFINE;
		annotation_potential_cp_define->annotation = potential_cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_potential_cp_define);
	}
		
	if (priorvalue == RW_LOCK_BIAS) {
	/* Automatically generated code for commit point define: Write_Trylock_Succ_Point */

	if (true) {
		struct anno_cp_define *cp_define = (struct anno_cp_define*) malloc(sizeof(struct anno_cp_define));
		cp_define->label_num = 8;
		cp_define->potential_cp_label_num = 7;
		struct spec_annotation *annotation_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define->type = CP_DEFINE;
		annotation_cp_define->annotation = cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define);
	}
		
		return 1;
	}
	/* Automatically generated code for commit point define: Write_Trylock_Fail_Point */

	if (true) {
		struct anno_cp_define *cp_define = (struct anno_cp_define*) malloc(sizeof(struct anno_cp_define));
		cp_define->label_num = 9;
		cp_define->potential_cp_label_num = 7;
		struct spec_annotation *annotation_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define->type = CP_DEFINE;
		annotation_cp_define->annotation = cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define);
	}
	
	atomic_fetch_add_explicit(&rw->lock, RW_LOCK_BIAS, memory_order_relaxed);
	return 0;
}

void __wrapper__read_unlock(rwlock_t * rw);

void read_unlock(rwlock_t * rw) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 4; // Read_Unlock
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	__wrapper__read_unlock(rw);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 4; // Read_Unlock
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Read_Unlock_info* info = (Read_Unlock_info*) malloc(sizeof(Read_Unlock_info));
	info->rw = rw;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 4; // Read_Unlock
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
}

void __wrapper__read_unlock(rwlock_t * rw)
{
	atomic_fetch_add_explicit(&rw->lock, 1, memory_order_release);
	/* Automatically generated code for commit point define check: Read_Unlock_Point */

	if (true) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 10;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
	
}

void __wrapper__write_unlock(rwlock_t * rw);

void write_unlock(rwlock_t * rw) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 5; // Write_Unlock
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	__wrapper__write_unlock(rw);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 5; // Write_Unlock
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Write_Unlock_info* info = (Write_Unlock_info*) malloc(sizeof(Write_Unlock_info));
	info->rw = rw;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 5; // Write_Unlock
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
}

void __wrapper__write_unlock(rwlock_t * rw)
{
	atomic_fetch_add_explicit(&rw->lock, RW_LOCK_BIAS, memory_order_release);
	/* Automatically generated code for commit point define check: Write_Unlock_Point */

	if (true) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 11;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
	
}

rwlock_t mylock;
int shareddata;

static void a(void *obj)
{
	int i;
	for(i = 0; i < 2; i++) {
		if ((i % 2) == 0) {
			read_lock(&mylock);
						printf("%d\n", shareddata);
			read_unlock(&mylock);
		} else {
			write_lock(&mylock);
						shareddata = 47;
			write_unlock(&mylock);
		}
	}
}

static void b(void *obj)
{
	int i;
	for(i = 0; i < 2; i++) {
		if ((i % 2) == 0) {
			if (read_trylock(&mylock) == 1) {
				printf("%d\n", shareddata);
				read_unlock(&mylock);
			}
		} else {
			if (write_trylock(&mylock) == 1) {
				shareddata = 47;
				write_unlock(&mylock);
			}
		}
	}
}

int user_main(int argc, char **argv)
{
	__sequential_init();
	
	thrd_t t1, t2;
	atomic_init(&mylock.lock, RW_LOCK_BIAS);

	thrd_create(&t1, (thrd_start_t)&a, NULL);
	thrd_create(&t2, (thrd_start_t)&b, NULL);

	thrd_join(t1);
	thrd_join(t2);

	return 0;
}

