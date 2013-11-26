#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>
#include "librace.h"
#define RW_LOCK_BIAS            0x00100000
#define WRITE_LOCK_CMP          RW_LOCK_BIAS
typedef union {
atomic_int lock ;
}
rwlock_t ;
/* Include all the header files that contains the interface declaration */
#include <stdio.h>
#include <stdatomic.h>
#include <threads.h>
#include "librace.h"

#include <stdlib.h>
#include <stdint.h>
#include <cdsannotate.h>
#include <common.h>
#include <spec_lib.h>
#include <specannotation.h>

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
static call_id_t Write_Trylock_id() {
call_id_t __ID__ = 0;
return __ID__;
}
/* End of ID function: Write_Trylock */

/* Check action function of interface: Write_Trylock */
static bool Write_Trylock_check_action(void *info, call_id_t __ID__) {
bool check_passed;
Write_Trylock_info* theInfo = (Write_Trylock_info*)info;
int __RET__ = theInfo->__RET__;
rwlock_t * rw = theInfo->rw;

bool __COND_SAT__ = ! writer_lock_acquired && reader_lock_cnt == 0;
if ( __COND_SAT__ ) writer_lock_acquired = true ;
check_passed = __COND_SAT__ ? __RET__ == 1 : __RET__ == 0;
if (!check_passed) return false;
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
static call_id_t Read_Trylock_id() {
call_id_t __ID__ = 0;
return __ID__;
}
/* End of ID function: Read_Trylock */

/* Check action function of interface: Read_Trylock */
static bool Read_Trylock_check_action(void *info, call_id_t __ID__) {
bool check_passed;
Read_Trylock_info* theInfo = (Read_Trylock_info*)info;
int __RET__ = theInfo->__RET__;
rwlock_t * rw = theInfo->rw;

bool __COND_SAT__ = writer_lock_acquired == false;
if ( __COND_SAT__ ) reader_lock_cnt ++ ;
check_passed = __COND_SAT__ ? __RET__ == 1 : __RET__ == 0;
if (!check_passed) return false;
return true;
}
/* End of check action function: Read_Trylock */

/* Definition of interface info struct: Write_Lock */
typedef struct Write_Lock_info {
rwlock_t * rw;
} Write_Lock_info;
/* End of info struct definition: Write_Lock */

/* ID function of interface: Write_Lock */
static call_id_t Write_Lock_id() {
call_id_t __ID__ = 0;
return __ID__;
}
/* End of ID function: Write_Lock */

/* Check action function of interface: Write_Lock */
static bool Write_Lock_check_action(void *info, call_id_t __ID__) {
bool check_passed;
Write_Lock_info* theInfo = (Write_Lock_info*)info;
rwlock_t * rw = theInfo->rw;

check_passed = ! writer_lock_acquired && reader_lock_cnt == 0;
if (!check_passed) return false;
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
static call_id_t Write_Unlock_id() {
call_id_t __ID__ = 0;
return __ID__;
}
/* End of ID function: Write_Unlock */

/* Check action function of interface: Write_Unlock */
static bool Write_Unlock_check_action(void *info, call_id_t __ID__) {
bool check_passed;
Write_Unlock_info* theInfo = (Write_Unlock_info*)info;
rwlock_t * rw = theInfo->rw;

check_passed = reader_lock_cnt == 0 && writer_lock_acquired;
if (!check_passed) return false;
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
static call_id_t Read_Unlock_id() {
call_id_t __ID__ = 0;
return __ID__;
}
/* End of ID function: Read_Unlock */

/* Check action function of interface: Read_Unlock */
static bool Read_Unlock_check_action(void *info, call_id_t __ID__) {
bool check_passed;
Read_Unlock_info* theInfo = (Read_Unlock_info*)info;
rwlock_t * rw = theInfo->rw;

check_passed = reader_lock_cnt > 0 && ! writer_lock_acquired;
if (!check_passed) return false;
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
static call_id_t Read_Lock_id() {
call_id_t __ID__ = 0;
return __ID__;
}
/* End of ID function: Read_Lock */

/* Check action function of interface: Read_Lock */
static bool Read_Lock_check_action(void *info, call_id_t __ID__) {
bool check_passed;
Read_Lock_info* theInfo = (Read_Lock_info*)info;
rwlock_t * rw = theInfo->rw;

check_passed = ! writer_lock_acquired;
if (!check_passed) return false;
reader_lock_cnt ++ ;
return true;
}
/* End of check action function: Read_Lock */

#define INTERFACE_SIZE 6
void** func_ptr_table;

/* Define function for sequential code initialization */
static void __sequential_init() {
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

writer_lock_acquired = false ;
reader_lock_cnt = 0 ;
/* Pass function table info */
struct anno_func_table_init *anno_func_table_init = (struct anno_func_table_init*) malloc(sizeof(struct anno_func_table_init));
anno_func_table_init->size = INTERFACE_SIZE;
anno_func_table_init->table = func_ptr_table;
struct spec_annotation *func_init = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
func_init->type = FUNC_TABLE_INIT;
func_init->annotation = anno_func_table_init;
_Z11cdsannotatemPv(SPEC_ANALYSIS, func_init);
/* Read_Unlock(true) -> Write_Lock(true) */
struct anno_hb_init *hbConditionInit0 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
hbConditionInit0->interface_num_before = 4;
hbConditionInit0->hb_condition_num_before = 0;
hbConditionInit0->interface_num_after = 1;
hbConditionInit0->hb_condition_num_after = 0;
struct spec_annotation *hb_init0 = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
hb_init0->type = HB_INIT;
hb_init0->annotation = hbConditionInit0;
_Z11cdsannotatemPv(SPEC_ANALYSIS, hb_init0);
/* Read_Unlock(true) -> Write_Trylock(HB_Write_Trylock_Succ) */
struct anno_hb_init *hbConditionInit1 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
hbConditionInit1->interface_num_before = 4;
hbConditionInit1->hb_condition_num_before = 0;
hbConditionInit1->interface_num_after = 3;
hbConditionInit1->hb_condition_num_after = 1;
struct spec_annotation *hb_init1 = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
hb_init1->type = HB_INIT;
hb_init1->annotation = hbConditionInit1;
_Z11cdsannotatemPv(SPEC_ANALYSIS, hb_init1);
/* Write_Unlock(true) -> Write_Lock(true) */
struct anno_hb_init *hbConditionInit2 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
hbConditionInit2->interface_num_before = 5;
hbConditionInit2->hb_condition_num_before = 0;
hbConditionInit2->interface_num_after = 1;
hbConditionInit2->hb_condition_num_after = 0;
struct spec_annotation *hb_init2 = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
hb_init2->type = HB_INIT;
hb_init2->annotation = hbConditionInit2;
_Z11cdsannotatemPv(SPEC_ANALYSIS, hb_init2);
/* Write_Unlock(true) -> Write_Trylock(HB_Write_Trylock_Succ) */
struct anno_hb_init *hbConditionInit3 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
hbConditionInit3->interface_num_before = 5;
hbConditionInit3->hb_condition_num_before = 0;
hbConditionInit3->interface_num_after = 3;
hbConditionInit3->hb_condition_num_after = 1;
struct spec_annotation *hb_init3 = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
hb_init3->type = HB_INIT;
hb_init3->annotation = hbConditionInit3;
_Z11cdsannotatemPv(SPEC_ANALYSIS, hb_init3);
/* Write_Unlock(true) -> Read_Lock(true) */
struct anno_hb_init *hbConditionInit4 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
hbConditionInit4->interface_num_before = 5;
hbConditionInit4->hb_condition_num_before = 0;
hbConditionInit4->interface_num_after = 0;
hbConditionInit4->hb_condition_num_after = 0;
struct spec_annotation *hb_init4 = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
hb_init4->type = HB_INIT;
hb_init4->annotation = hbConditionInit4;
_Z11cdsannotatemPv(SPEC_ANALYSIS, hb_init4);
/* Write_Unlock(true) -> Read_Trylock(HB_Read_Trylock_Succ) */
struct anno_hb_init *hbConditionInit5 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
hbConditionInit5->interface_num_before = 5;
hbConditionInit5->hb_condition_num_before = 0;
hbConditionInit5->interface_num_after = 2;
hbConditionInit5->hb_condition_num_after = 2;
struct spec_annotation *hb_init5 = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
hb_init5->type = HB_INIT;
hb_init5->annotation = hbConditionInit5;
_Z11cdsannotatemPv(SPEC_ANALYSIS, hb_init5);
}
/* End of Global construct generation in class */
static inline int read_can_lock ( rwlock_t * lock ) {
return atomic_load_explicit ( & lock -> lock , memory_order_relaxed ) > 0 ;
}
static inline int write_can_lock ( rwlock_t * lock ) {
return atomic_load_explicit ( & lock -> lock , memory_order_relaxed ) == RW_LOCK_BIAS ;
}
#include <stdlib.h>
#include <threads.h>
#include <cdsannotate.h>
#include <specannotation.h>
#include <spec_lib.h>
/* Declaration of the wrapper */
void __wrapper__read_lock(rwlock_t * rw);

void read_lock(rwlock_t * rw) {
/* Interface begins */
struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
interface_begin->interface_num = 0;
struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_interface_begin->type = INTERFACE_BEGIN;
annotation_interface_begin->annotation = interface_begin;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_interface_begin);
__wrapper__read_lock(rw);
struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
hb_condition->interface_num = 0;
hb_condition->hb_condition_num = 0;
struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_hb_condition->type = HB_CONDITION;
annotation_hb_condition->annotation = hb_condition;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_hb_condition);

Read_Lock_info* info = (Read_Lock_info*) malloc(sizeof(Read_Lock_info));
info->rw = rw;
struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
interface_end->interface_num = 0;
interface_end->info = info;
struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annoation_interface_end->type = INTERFACE_END;
annoation_interface_end->annotation = interface_end;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annoation_interface_end);
}
void __wrapper__read_lock(rwlock_t * rw) {
int priorvalue = atomic_fetch_sub_explicit ( & rw -> lock , 1 , memory_order_acquire ) ;
/* Automatically generated code for commit point define check: Read_Lock_Success_1 */
/* Include redundant headers */
#include <stdint.h>
#include <cdsannotate.h>

if (priorvalue > 0) {
struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
cp_define_check->label_num = 0;
struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_cp_define_check->type = CP_DEFINE_CHECK;
annotation_cp_define_check->annotation = cp_define_check;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_cp_define_check);
}
while ( priorvalue <= 0 ) {
atomic_fetch_add_explicit ( & rw -> lock , 1 , memory_order_relaxed ) ;
while ( atomic_load_explicit ( & rw -> lock , memory_order_relaxed ) <= 0 ) {
thrd_yield ( ) ;
}
priorvalue = atomic_fetch_sub_explicit ( & rw -> lock , 1 , memory_order_acquire ) ;
/* Automatically generated code for commit point define check: Read_Lock_Success_2 */
/* Include redundant headers */
#include <stdint.h>
#include <cdsannotate.h>

if (priorvalue > 0) {
struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
cp_define_check->label_num = 1;
struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_cp_define_check->type = CP_DEFINE_CHECK;
annotation_cp_define_check->annotation = cp_define_check;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_cp_define_check);
}
}
}
#include <stdlib.h>
#include <threads.h>
#include <cdsannotate.h>
#include <specannotation.h>
#include <spec_lib.h>
/* Declaration of the wrapper */
void __wrapper__write_lock(rwlock_t * rw);

void write_lock(rwlock_t * rw) {
/* Interface begins */
struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
interface_begin->interface_num = 1;
struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_interface_begin->type = INTERFACE_BEGIN;
annotation_interface_begin->annotation = interface_begin;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_interface_begin);
__wrapper__write_lock(rw);
struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
hb_condition->interface_num = 1;
hb_condition->hb_condition_num = 0;
struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_hb_condition->type = HB_CONDITION;
annotation_hb_condition->annotation = hb_condition;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_hb_condition);

Write_Lock_info* info = (Write_Lock_info*) malloc(sizeof(Write_Lock_info));
info->rw = rw;
struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
interface_end->interface_num = 1;
interface_end->info = info;
struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annoation_interface_end->type = INTERFACE_END;
annoation_interface_end->annotation = interface_end;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annoation_interface_end);
}
void __wrapper__write_lock(rwlock_t * rw) {
int priorvalue = atomic_fetch_sub_explicit ( & rw -> lock , RW_LOCK_BIAS , memory_order_acquire ) ;
/* Automatically generated code for commit point define check: Write_Lock_Success_1 */
/* Include redundant headers */
#include <stdint.h>
#include <cdsannotate.h>

if (priorvalue == RW_LOCK_BIAS) {
struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
cp_define_check->label_num = 2;
struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_cp_define_check->type = CP_DEFINE_CHECK;
annotation_cp_define_check->annotation = cp_define_check;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_cp_define_check);
}
while ( priorvalue != RW_LOCK_BIAS ) {
atomic_fetch_add_explicit ( & rw -> lock , RW_LOCK_BIAS , memory_order_relaxed ) ;
while ( atomic_load_explicit ( & rw -> lock , memory_order_relaxed ) != RW_LOCK_BIAS ) {
thrd_yield ( ) ;
}
priorvalue = atomic_fetch_sub_explicit ( & rw -> lock , RW_LOCK_BIAS , memory_order_acquire ) ;
/* Automatically generated code for commit point define check: Write_Lock_Success_2 */
/* Include redundant headers */
#include <stdint.h>
#include <cdsannotate.h>

if (priorvalue == RW_LOCK_BIAS) {
struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
cp_define_check->label_num = 3;
struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_cp_define_check->type = CP_DEFINE_CHECK;
annotation_cp_define_check->annotation = cp_define_check;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_cp_define_check);
}
}
}
#include <stdlib.h>
#include <threads.h>
#include <cdsannotate.h>
#include <specannotation.h>
#include <spec_lib.h>
/* Declaration of the wrapper */
int __wrapper__read_trylock(rwlock_t * rw);

int read_trylock(rwlock_t * rw) {
/* Interface begins */
struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
interface_begin->interface_num = 2;
struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_interface_begin->type = INTERFACE_BEGIN;
annotation_interface_begin->annotation = interface_begin;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_interface_begin);
int __RET__ = __wrapper__read_trylock(rw);
if (__RET__ == 1) {
struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
hb_condition->interface_num = 2;
hb_condition->hb_condition_num = 2;
struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_hb_condition->type = HB_CONDITION;
annotation_hb_condition->annotation = hb_condition;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_hb_condition);
}

Read_Trylock_info* info = (Read_Trylock_info*) malloc(sizeof(Read_Trylock_info));
info->__RET__ = __RET__;
info->rw = rw;
struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
interface_end->interface_num = 2;
interface_end->info = info;
struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annoation_interface_end->type = INTERFACE_END;
annoation_interface_end->annotation = interface_end;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annoation_interface_end);
return __RET__;
}
int __wrapper__read_trylock(rwlock_t * rw) {
int priorvalue = atomic_fetch_sub_explicit ( & rw -> lock , 1 , memory_order_acquire ) ;
/* Automatically generated code for commit point define check: Read_Trylock_Point */
/* Include redundant headers */
#include <stdint.h>
#include <cdsannotate.h>

if (true) {
struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
cp_define_check->label_num = 4;
struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_cp_define_check->type = CP_DEFINE_CHECK;
annotation_cp_define_check->annotation = cp_define_check;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_cp_define_check);
}
if ( priorvalue > 0 ) return 1 ;
atomic_fetch_add_explicit ( & rw -> lock , 1 , memory_order_relaxed ) ;
return 0 ;
}
#include <stdlib.h>
#include <threads.h>
#include <cdsannotate.h>
#include <specannotation.h>
#include <spec_lib.h>
/* Declaration of the wrapper */
int __wrapper__write_trylock(rwlock_t * rw);

int write_trylock(rwlock_t * rw) {
/* Interface begins */
struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
interface_begin->interface_num = 3;
struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_interface_begin->type = INTERFACE_BEGIN;
annotation_interface_begin->annotation = interface_begin;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_interface_begin);
int __RET__ = __wrapper__write_trylock(rw);
if (__RET__ == 1) {
struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
hb_condition->interface_num = 3;
hb_condition->hb_condition_num = 1;
struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_hb_condition->type = HB_CONDITION;
annotation_hb_condition->annotation = hb_condition;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_hb_condition);
}

Write_Trylock_info* info = (Write_Trylock_info*) malloc(sizeof(Write_Trylock_info));
info->__RET__ = __RET__;
info->rw = rw;
struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
interface_end->interface_num = 3;
interface_end->info = info;
struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annoation_interface_end->type = INTERFACE_END;
annoation_interface_end->annotation = interface_end;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annoation_interface_end);
return __RET__;
}
int __wrapper__write_trylock(rwlock_t * rw) {
int priorvalue = atomic_fetch_sub_explicit ( & rw -> lock , RW_LOCK_BIAS , memory_order_acquire ) ;
/* Automatically generated code for commit point define check: Write_Trylock_Point */
/* Include redundant headers */
#include <stdint.h>
#include <cdsannotate.h>

if (true) {
struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
cp_define_check->label_num = 5;
struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_cp_define_check->type = CP_DEFINE_CHECK;
annotation_cp_define_check->annotation = cp_define_check;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_cp_define_check);
}
if ( priorvalue == RW_LOCK_BIAS ) return 1 ;
atomic_fetch_add_explicit ( & rw -> lock , RW_LOCK_BIAS , memory_order_relaxed ) ;
return 0 ;
}
#include <stdlib.h>
#include <threads.h>
#include <cdsannotate.h>
#include <specannotation.h>
#include <spec_lib.h>
/* Declaration of the wrapper */
void __wrapper__read_unlock(rwlock_t * rw);

void read_unlock(rwlock_t * rw) {
/* Interface begins */
struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
interface_begin->interface_num = 4;
struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_interface_begin->type = INTERFACE_BEGIN;
annotation_interface_begin->annotation = interface_begin;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_interface_begin);
__wrapper__read_unlock(rw);
struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
hb_condition->interface_num = 4;
hb_condition->hb_condition_num = 0;
struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_hb_condition->type = HB_CONDITION;
annotation_hb_condition->annotation = hb_condition;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_hb_condition);

Read_Unlock_info* info = (Read_Unlock_info*) malloc(sizeof(Read_Unlock_info));
info->rw = rw;
struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
interface_end->interface_num = 4;
interface_end->info = info;
struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annoation_interface_end->type = INTERFACE_END;
annoation_interface_end->annotation = interface_end;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annoation_interface_end);
}
void __wrapper__read_unlock(rwlock_t * rw) {
atomic_fetch_add_explicit ( & rw -> lock , 1 , memory_order_release ) ;
/* Automatically generated code for commit point define check: Read_Unlock_Point */
/* Include redundant headers */
#include <stdint.h>
#include <cdsannotate.h>

if (true) {
struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
cp_define_check->label_num = 6;
struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_cp_define_check->type = CP_DEFINE_CHECK;
annotation_cp_define_check->annotation = cp_define_check;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_cp_define_check);
}
}
#include <stdlib.h>
#include <threads.h>
#include <cdsannotate.h>
#include <specannotation.h>
#include <spec_lib.h>
/* Declaration of the wrapper */
void __wrapper__write_unlock(rwlock_t * rw);

void write_unlock(rwlock_t * rw) {
/* Interface begins */
struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
interface_begin->interface_num = 5;
struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_interface_begin->type = INTERFACE_BEGIN;
annotation_interface_begin->annotation = interface_begin;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_interface_begin);
__wrapper__write_unlock(rw);
struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
hb_condition->interface_num = 5;
hb_condition->hb_condition_num = 0;
struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_hb_condition->type = HB_CONDITION;
annotation_hb_condition->annotation = hb_condition;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_hb_condition);

Write_Unlock_info* info = (Write_Unlock_info*) malloc(sizeof(Write_Unlock_info));
info->rw = rw;
struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
interface_end->interface_num = 5;
interface_end->info = info;
struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annoation_interface_end->type = INTERFACE_END;
annoation_interface_end->annotation = interface_end;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annoation_interface_end);
}
void __wrapper__write_unlock(rwlock_t * rw) {
atomic_fetch_add_explicit ( & rw -> lock , RW_LOCK_BIAS , memory_order_release ) ;
/* Automatically generated code for commit point define check: Write_Unlock_Point */
/* Include redundant headers */
#include <stdint.h>
#include <cdsannotate.h>

if (true) {
struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
cp_define_check->label_num = 7;
struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
annotation_cp_define_check->type = CP_DEFINE_CHECK;
annotation_cp_define_check->annotation = cp_define_check;
_Z11cdsannotatemPv(SPEC_ANALYSIS, annotation_cp_define_check);
}
}
rwlock_t mylock ;
int shareddata ;
static void a ( void * obj ) {
int i ;
for ( i = 0 ;
i < 2 ;
i ++ ) {
if ( ( i % 2 ) == 0 ) {
read_lock ( & mylock ) ;
load_32 ( & shareddata ) ;
read_unlock ( & mylock ) ;
}
else {
write_lock ( & mylock ) ;
store_32 ( & shareddata , ( unsigned int ) i ) ;
write_unlock ( & mylock ) ;
}
}
}
int user_main ( int argc , char * * argv ) {
__sequential_init();
thrd_t t1 , t2 ;
atomic_init ( & mylock . lock , RW_LOCK_BIAS ) ;
thrd_create ( & t1 , ( thrd_start_t ) & a , NULL ) ;
thrd_create ( & t2 , ( thrd_start_t ) & a , NULL ) ;
thrd_join ( t1 ) ;
thrd_join ( t2 ) ;
return 0 ;
}
