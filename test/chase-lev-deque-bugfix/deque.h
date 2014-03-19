#ifndef DEQUE_H
#define DEQUE_H
#include <spec_lib.h>
#include <stdlib.h>
#include <cdsannotate.h>
#include <specannotation.h>
#include <model_memory.h>
#include "common.h"

typedef struct {
	atomic_size_t size;
	atomic_int buffer[];
} Array;

typedef struct {
	atomic_size_t top, bottom;
	atomic_uintptr_t array; 
} Deque;

#define EMPTY 0xffffffff
#define ABORT 0xfffffffe

/* All other user-defined structs */
typedef struct tag_elem {
call_id_t id ;
int data ;
}
tag_elem_t ;

static spec_list * __deque;
static id_tag_t * tag;
/* All other user-defined functions */
inline static tag_elem_t * new_tag_elem ( call_id_t id , int data ) {
tag_elem_t * e = ( tag_elem_t * ) CMODEL_MALLOC ( sizeof ( tag_elem_t ) ) ;
e -> id = id ;
e -> data = data ;
return e ;
}

inline static call_id_t get_id ( void * wrapper ) {
return ( ( tag_elem_t * ) wrapper ) -> id ;
}

inline static int get_data ( void * wrapper ) {
return ( ( tag_elem_t * ) wrapper ) -> data ;
}

/* Definition of interface info struct: Steal */
typedef struct Steal_info {
int __RET__;
Deque * q;
} Steal_info;
/* End of info struct definition: Steal */

/* ID function of interface: Steal */
inline static call_id_t Steal_id(void *info, thread_id_t __TID__) {
	Steal_info* theInfo = (Steal_info*)info;
	int __RET__ = theInfo->__RET__;
	Deque * q = theInfo->q;

	call_id_t __ID__ = size ( __deque ) == 0 ? DEFAULT_CALL_ID : get_id ( front ( __deque ) );
	return __ID__;
}
/* End of ID function: Steal */

/* Check action function of interface: Steal */
inline static bool Steal_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Steal_info* theInfo = (Steal_info*)info;
	int __RET__ = theInfo->__RET__;
	Deque * q = theInfo->q;

	int _Old_Val = EMPTY ;
	if ( size ( __deque ) > 0 ) {
	_Old_Val = get_data ( front ( __deque ) ) ;
	pop_front ( __deque ) ;
	}
	check_passed = _Old_Val == __RET__;
	if (!check_passed)
		return false;
	return true;
}
/* End of check action function: Steal */

/* Definition of interface info struct: Take */
typedef struct Take_info {
int __RET__;
Deque * q;
} Take_info;
/* End of info struct definition: Take */

/* ID function of interface: Take */
inline static call_id_t Take_id(void *info, thread_id_t __TID__) {
	Take_info* theInfo = (Take_info*)info;
	int __RET__ = theInfo->__RET__;
	Deque * q = theInfo->q;

	call_id_t __ID__ = size ( __deque ) == 0 ? DEFAULT_CALL_ID : get_id ( back ( __deque ) );
	return __ID__;
}
/* End of ID function: Take */

/* Check action function of interface: Take */
inline static bool Take_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Take_info* theInfo = (Take_info*)info;
	int __RET__ = theInfo->__RET__;
	Deque * q = theInfo->q;

	int _Old_Val = EMPTY ;
	if ( size ( __deque ) > 0 ) {
	_Old_Val = get_data ( back ( __deque ) ) ;
	pop_back ( __deque ) ;
	}
	check_passed = _Old_Val == __RET__;
	if (!check_passed)
		return false;
	return true;
}
/* End of check action function: Take */

/* Definition of interface info struct: Push */
typedef struct Push_info {
Deque * q;
int x;
} Push_info;
/* End of info struct definition: Push */

/* ID function of interface: Push */
inline static call_id_t Push_id(void *info, thread_id_t __TID__) {
	Push_info* theInfo = (Push_info*)info;
	Deque * q = theInfo->q;
	int x = theInfo->x;

	call_id_t __ID__ = get_and_inc ( tag ) ;;
	return __ID__;
}
/* End of ID function: Push */

/* Check action function of interface: Push */
inline static bool Push_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Push_info* theInfo = (Push_info*)info;
	Deque * q = theInfo->q;
	int x = theInfo->x;

	tag_elem_t * elem = new_tag_elem ( __ID__ , x ) ;
	push_back ( __deque , elem ) ;
	return true;
}
/* End of check action function: Push */

#define INTERFACE_SIZE 3
static void** func_ptr_table;
static anno_hb_init** hb_init_table;

/* Define function for sequential code initialization */
inline static void __sequential_init() {
	/* Init func_ptr_table */
	func_ptr_table = (void**) malloc(sizeof(void*) * 3 * 2);
	func_ptr_table[2 * 2] = (void*) &Steal_id;
	func_ptr_table[2 * 2 + 1] = (void*) &Steal_check_action;
	func_ptr_table[2 * 0] = (void*) &Take_id;
	func_ptr_table[2 * 0 + 1] = (void*) &Take_check_action;
	func_ptr_table[2 * 1] = (void*) &Push_id;
	func_ptr_table[2 * 1 + 1] = (void*) &Push_check_action;
	/* Push(true) -> Steal(true) */
	struct anno_hb_init *hbConditionInit0 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit0->interface_num_before = 1; // Push
	hbConditionInit0->hb_condition_num_before = 0; // 
	hbConditionInit0->interface_num_after = 2; // Steal
	hbConditionInit0->hb_condition_num_after = 0; // 
	/* Push(true) -> Take(true) */
	struct anno_hb_init *hbConditionInit1 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit1->interface_num_before = 1; // Push
	hbConditionInit1->hb_condition_num_before = 0; // 
	hbConditionInit1->interface_num_after = 0; // Take
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

	__deque = new_spec_list ( ) ;
	tag = new_id_tag ( ) ;
}
/* End of Global construct generation in class */



Deque * create();
void resize(Deque *q);

int __wrapper__take(Deque * q);

int __wrapper__take(Deque * q) ;

void __wrapper__push(Deque * q, int x);

void __wrapper__push(Deque * q, int x) ;

int __wrapper__steal(Deque * q);

int __wrapper__steal(Deque * q) ;

#endif

