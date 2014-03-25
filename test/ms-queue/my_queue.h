#ifndef _MY_QUEUE_H
#define _MY_QUEUE_H

#include <stdatomic.h>

#include <spec_lib.h>
#include <stdlib.h>
#include <cdsannotate.h>
#include <specannotation.h>
#include <model_memory.h>
#include "common.h" 

#define MAX_NODES			0xf

typedef unsigned long long pointer;
typedef atomic_ullong pointer_t;

#define MAKE_POINTER(ptr, count)	((((pointer)count) << 32) | ptr)
#define PTR_MASK 0xffffffffLL
#define COUNT_MASK (0xffffffffLL << 32)

static inline void set_count(pointer *p, unsigned int val) { *p = (*p & ~COUNT_MASK) | ((pointer)val << 32); }
static inline void set_ptr(pointer *p, unsigned int val) { *p = (*p & ~PTR_MASK) | val; }
static inline unsigned int get_count(pointer p) { return (p & COUNT_MASK) >> 32; }
static inline unsigned int get_ptr(pointer p) { return p & PTR_MASK; }

typedef struct node {
	unsigned int value;
	pointer_t next;
} node_t;

typedef struct {
	pointer_t head;
	pointer_t tail;
	node_t nodes[MAX_NODES + 1];
} queue_t;

void init_queue(queue_t *q, int num_threads);

/* All other user-defined structs */
typedef struct tag_elem {
call_id_t id ;
unsigned int data ;
}
tag_elem_t ;

static spec_list * __queue;
static id_tag_t * tag;
/* All other user-defined functions */
inline static tag_elem_t * new_tag_elem ( call_id_t id , unsigned int data ) {
tag_elem_t * e = ( tag_elem_t * ) CMODEL_MALLOC ( sizeof ( tag_elem_t ) ) ;
e -> id = id ;
e -> data = data ;
return e ;
}

inline static void free_tag_elem ( tag_elem_t * e ) {
free ( e ) ;
}

inline static call_id_t get_id ( void * wrapper ) {
if ( wrapper == NULL ) return 0 ;
return ( ( tag_elem_t * ) wrapper ) -> id ;
}

inline static unsigned int get_data ( void * wrapper ) {
return ( ( tag_elem_t * ) wrapper ) -> data ;
}

/* Definition of interface info struct: Dequeue */
typedef struct Dequeue_info {
 unsigned int __RET__;
queue_t * q;
} Dequeue_info;
/* End of info struct definition: Dequeue */

/* ID function of interface: Dequeue */
inline static call_id_t Dequeue_id(void *info, thread_id_t __TID__) {
	Dequeue_info* theInfo = (Dequeue_info*)info;
	 unsigned int __RET__ = theInfo->__RET__;
	queue_t * q = theInfo->q;

	call_id_t __ID__ = get_id ( front ( __queue ) );
	return __ID__;
}
/* End of ID function: Dequeue */

/* Check action function of interface: Dequeue */
inline static bool Dequeue_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Dequeue_info* theInfo = (Dequeue_info*)info;
	 unsigned int __RET__ = theInfo->__RET__;
	queue_t * q = theInfo->q;

	unsigned int _Old_Val = 0 ;
	if ( size ( __queue ) > 0 ) {
	_Old_Val = get_data ( front ( __queue ) ) ;
	pop_front ( __queue ) ;
	}
	else {
	_Old_Val = 0 ;
	}
	check_passed = _Old_Val == __RET__;
	if (!check_passed)
		return false;
	return true;
}
/* End of check action function: Dequeue */

/* Definition of interface info struct: Enqueue */
typedef struct Enqueue_info {
queue_t * q;
 unsigned int val;
} Enqueue_info;
/* End of info struct definition: Enqueue */

/* ID function of interface: Enqueue */
inline static call_id_t Enqueue_id(void *info, thread_id_t __TID__) {
	Enqueue_info* theInfo = (Enqueue_info*)info;
	queue_t * q = theInfo->q;
	 unsigned int val = theInfo->val;

	call_id_t __ID__ = get_and_inc ( tag );
	return __ID__;
}
/* End of ID function: Enqueue */

/* Check action function of interface: Enqueue */
inline static bool Enqueue_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Enqueue_info* theInfo = (Enqueue_info*)info;
	queue_t * q = theInfo->q;
	 unsigned int val = theInfo->val;

	tag_elem_t * elem = new_tag_elem ( __ID__ , val ) ;
	push_back ( __queue , elem ) ;
	return true;
}
/* End of check action function: Enqueue */

#define INTERFACE_SIZE 2
static void** func_ptr_table;
static anno_hb_init** hb_init_table;

/* Define function for sequential code initialization */
inline static void __sequential_init() {
	/* Init func_ptr_table */
	func_ptr_table = (void**) malloc(sizeof(void*) * 2 * 2);
	func_ptr_table[2 * 1] = (void*) &Dequeue_id;
	func_ptr_table[2 * 1 + 1] = (void*) &Dequeue_check_action;
	func_ptr_table[2 * 0] = (void*) &Enqueue_id;
	func_ptr_table[2 * 0 + 1] = (void*) &Enqueue_check_action;
	/* Enqueue(true) -> Dequeue(true) */
	struct anno_hb_init *hbConditionInit0 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit0->interface_num_before = 0; // Enqueue
	hbConditionInit0->hb_condition_num_before = 0; // 
	hbConditionInit0->interface_num_after = 1; // Dequeue
	hbConditionInit0->hb_condition_num_after = 0; // 
	/* Init hb_init_table */
	hb_init_table = (anno_hb_init**) malloc(sizeof(anno_hb_init*) * 1);
	#define HB_INIT_TABLE_SIZE 1
	hb_init_table[0] = hbConditionInit0;
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

	__queue = new_spec_list ( ) ;
	tag = new_id_tag ( ) ;
}
/* End of Global construct generation in class */




void __wrapper__enqueue(queue_t * q,  unsigned int val);

void __wrapper__enqueue(queue_t * q,  unsigned int val) ;

 unsigned int __wrapper__dequeue(queue_t * q);

 unsigned int __wrapper__dequeue(queue_t * q) ;
int get_thread_num();

#endif

