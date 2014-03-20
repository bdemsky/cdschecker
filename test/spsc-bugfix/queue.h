#ifndef _QUEUE_H
#define _QUEUE_H

#include <unrelacy.h>
#include <atomic>

#include <spec_lib.h>
#include <stdlib.h>
#include <cdsannotate.h>
#include <specannotation.h>
#include <model_memory.h>
#include "common.h" 

#include "eventcount.h"


template<typename T>
class spsc_queue
{
	
public:

	
	spsc_queue()
	{

	__sequential_init();
		

		node* n = new node ();
		head = n;
		tail = n;
	}

	~spsc_queue()
	{
		RL_ASSERT(head == tail);
		delete ((node*)head($));
	}

/* All other user-defined structs */
typedef struct tag_elem {
call_id_t id ;
T data ;
}
tag_elem_t ;

static spec_list * __queue;
static id_tag_t * tag;
/* All other user-defined functions */
inline static tag_elem_t * new_tag_elem ( call_id_t id , T data ) {
tag_elem_t * e = ( tag_elem_t * ) MODEL_MALLOC ( sizeof ( tag_elem_t ) ) ;
e -> id = id ;
e -> data = data ;
return e ;
}

inline static call_id_t get_id ( void * wrapper ) {
return ( ( tag_elem_t * ) wrapper ) -> id ;
}

inline static unsigned int get_data ( void * wrapper ) {
return ( ( tag_elem_t * ) wrapper ) -> data ;
}

/* Definition of interface info struct: Dequeue */
typedef struct Dequeue_info {
T __RET__;
} Dequeue_info;
/* End of info struct definition: Dequeue */

/* ID function of interface: Dequeue */
inline static call_id_t Dequeue_id(void *info, thread_id_t __TID__) {
	Dequeue_info* theInfo = (Dequeue_info*)info;
	T __RET__ = theInfo->__RET__;

	call_id_t __ID__ = get_id ( front ( __queue ) );
	return __ID__;
}
/* End of ID function: Dequeue */

/* Check action function of interface: Dequeue */
inline static bool Dequeue_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Dequeue_info* theInfo = (Dequeue_info*)info;
	T __RET__ = theInfo->__RET__;

	T _Old_Val = get_data ( front ( __queue ) ) ;
	pop_front ( __queue ) ;
	check_passed = _Old_Val == __RET__;
	if (!check_passed)
		return false;
	return true;
}
/* End of check action function: Dequeue */

/* Definition of interface info struct: Enqueue */
typedef struct Enqueue_info {
T data;
} Enqueue_info;
/* End of info struct definition: Enqueue */

/* ID function of interface: Enqueue */
inline static call_id_t Enqueue_id(void *info, thread_id_t __TID__) {
	Enqueue_info* theInfo = (Enqueue_info*)info;
	T data = theInfo->data;

	call_id_t __ID__ = get_and_inc ( tag );
	return __ID__;
}
/* End of ID function: Enqueue */

/* Check action function of interface: Enqueue */
inline static bool Enqueue_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Enqueue_info* theInfo = (Enqueue_info*)info;
	T data = theInfo->data;

	tag_elem_t * elem = new_tag_elem ( __ID__ , data ) ;
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
	

	

void enqueue(T data) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 0; // Enqueue
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	__wrapper__enqueue(data);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 0; // Enqueue
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Enqueue_info* info = (Enqueue_info*) malloc(sizeof(Enqueue_info));
	info->data = data;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 0; // Enqueue
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
}
	
void __wrapper__enqueue(T data)
	{
		node* n = new node (data);
		head($)->next.store(n, std::memory_order_release);
	/* Automatically generated code for commit point define check: Enqueue_Point */

	if (true) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 0;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
		
		head = n;
				ec.signal();
	}

T dequeue() {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 1; // Dequeue
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	T __RET__ = __wrapper__dequeue();
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 1; // Dequeue
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Dequeue_info* info = (Dequeue_info*) malloc(sizeof(Dequeue_info));
	info->__RET__ = __RET__;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 1; // Dequeue
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
	return __RET__;
}
	
T __wrapper__dequeue()
	{
		T data = try_dequeue();
		while (0 == data)
		{
			int cmp = ec.get();
			data = try_dequeue();
			if (data)
				break;
			ec.wait(cmp);
			data = try_dequeue();
			if (data)
				break;
		}
		return data;
	}

private:
	struct node
	{
		std::atomic<node*> next;
		rl::var<T> data;

		node(T data = T())
			: data(data)
		{
			next = 0;
		}
	};

	rl::var<node*> head;
	rl::var<node*> tail;

	eventcount ec;

	T try_dequeue()
	{
		node* t = tail($);
		node* n = t->next.load(std::memory_order_acquire);
	/* Automatically generated code for commit point define check: Dequeue_Point */

	if (n != 0) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 1;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
		
		if (0 == n)
			return 0;
		T data = n->data($);
		delete (t);
		tail = n;
		return data;
	}
};
template<typename T>
void** spsc_queue<T>::func_ptr_table;
template<typename T>
anno_hb_init** spsc_queue<T>::hb_init_table;
template<typename T>
spec_list * spsc_queue<T>::__queue;
template<typename T>
id_tag_t * spsc_queue<T>::tag;


#endif

