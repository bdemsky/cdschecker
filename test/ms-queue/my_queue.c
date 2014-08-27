#include <threads.h>
#include <stdlib.h>
#include "librace.h"
#include "model-assert.h"

#include "my_queue.h"

#define relaxed memory_order_relaxed
#define release memory_order_release
#define acquire memory_order_acquire

#define MAX_FREELIST 4 
#define INITIAL_FREE 3 

#define POISON_IDX 0x666

static unsigned int (*free_lists)[MAX_FREELIST];


static unsigned int new_node()
{
	int i;
	int t = get_thread_num();
	for (i = 0; i < MAX_FREELIST; i++) {
		unsigned int node = free_lists[t][i];
		//unsigned int node = load_32(&free_lists[t][i]);
				if (node) {
			//store_32(&free_lists[t][i], 0);
			free_lists[t][i] = 0;
						return node;
		}
	}
	
		return 0;
}


static void reclaim(unsigned int node)
{
	int i;
	int t = get_thread_num();

	
	
	for (i = 0; i < MAX_FREELIST; i++) {
		
		//unsigned int idx = load_32(&free_lists[t][i]);
		unsigned int idx = free_lists[t][i];
		
		
		if (idx == 0) {
			//store_32(&free_lists[t][i], node);
			free_lists[t][i] = node;
						return;
		}
	}
	
	}

void init_queue(queue_t *q, int num_threads)
{
	int i, j;

	
	
	free_lists = malloc(num_threads * sizeof(*free_lists));
	for (i = 0; i < num_threads; i++) {
		for (j = 0; j < INITIAL_FREE; j++) {
			free_lists[i][j] = 2 + i * MAX_FREELIST + j;
			atomic_init(&q->nodes[free_lists[i][j]].next, MAKE_POINTER(POISON_IDX, 0));
		}
	}

	
	atomic_init(&q->head, MAKE_POINTER(1, 0));
	atomic_init(&q->tail, MAKE_POINTER(1, 0));
	atomic_init(&q->nodes[1].next, MAKE_POINTER(0, 0));
}


void enqueue(queue_t * q,  unsigned int val) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 0; // Enqueue
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	__wrapper__enqueue(q, val);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 0; // Enqueue
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Enqueue_info* info = (Enqueue_info*) malloc(sizeof(Enqueue_info));
	info->q = q;
	info->val = val;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 0; // Enqueue
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
}

void __wrapper__enqueue(queue_t * q,  unsigned int val)
{
	int success = 0;
	unsigned int node;
	pointer tail;
	pointer next;
	pointer tmp;

	node = new_node();
	//store_32(&q->nodes[node].value, val);
	q->nodes[node].value = val;
		tmp = atomic_load_explicit(&q->nodes[node].next, relaxed);
	set_ptr(&tmp, 0); 	atomic_store_explicit(&q->nodes[node].next, tmp, relaxed);

	while (!success) {
		
		tail = atomic_load_explicit(&q->tail, acquire);
		
		next = atomic_load_explicit(&q->nodes[get_ptr(tail)].next, acquire);
				if (tail == atomic_load_explicit(&q->tail, relaxed)) {

			
			
			if (get_ptr(next) == 0) { 				pointer value = MAKE_POINTER(node, get_count(next) + 1);
				
								success = atomic_compare_exchange_strong_explicit(&q->nodes[get_ptr(tail)].next,
						&next, value, release, relaxed);
	/* Automatically generated code for commit point define check: Enqueue_Success_Point */

	if (success == true) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 0;
		cp_define_check->interface_num = 0;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
				
			}
			if (!success) {
								
				unsigned int ptr = get_ptr(atomic_load_explicit(&q->nodes[get_ptr(tail)].next, acquire));
				pointer value = MAKE_POINTER(ptr,
						get_count(tail) + 1);
				
								bool succ = false;
				succ = atomic_compare_exchange_strong_explicit(&q->tail,
						&tail, value, release, relaxed);
				if (succ) {
									}
								thrd_yield();
			}
		}
	}
	
		atomic_compare_exchange_strong_explicit(&q->tail,
			&tail,
			MAKE_POINTER(node, get_count(tail) + 1),
			release, relaxed);
}


bool dequeue(queue_t * q,  unsigned int * retVal) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 1; // Dequeue
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	bool __RET__ = __wrapper__dequeue(q, retVal);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 1; // Dequeue
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Dequeue_info* info = (Dequeue_info*) malloc(sizeof(Dequeue_info));
	info->__RET__ = __RET__;
	info->q = q;
	info->retVal = retVal;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 1; // Dequeue
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
	return __RET__;
}

bool __wrapper__dequeue(queue_t * q,  unsigned int * retVal)
{
	unsigned int value;
	int success = 0;
	pointer head;
	pointer tail;
	pointer next;

	while (!success) {
		
		head = atomic_load_explicit(&q->head, acquire);
		tail = atomic_load_explicit(&q->tail, acquire);
		
		next = atomic_load_explicit(&q->nodes[get_ptr(head)].next, acquire);
				if (atomic_load_explicit(&q->head, relaxed) == head) {
			if (get_ptr(head) == get_ptr(tail)) {

				
				
	/* Automatically generated code for commit point define check: Dequeue_Empty_Point */

	if (true) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 1;
		cp_define_check->interface_num = 1;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
				if (get_ptr(next) == 0) { 					
					return false; 				}
				
								bool succ = false;
				succ = atomic_compare_exchange_strong_explicit(&q->tail,
						&tail,
						MAKE_POINTER(get_ptr(next), get_count(tail) + 1),
						release, relaxed);
				if (succ) {
									}
								thrd_yield();
			} else {
				//*retVal = load_32(&q->nodes[get_ptr(next)].value);
				*retVal = q->nodes[get_ptr(next)].value;
								
								success = atomic_compare_exchange_strong_explicit(&q->head,
						&head,
						MAKE_POINTER(get_ptr(next), get_count(head) + 1),
						release, relaxed);
	/* Automatically generated code for commit point define check: Dequeue_Success_Point */

	if (success == true) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 2;
		cp_define_check->interface_num = 1;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
				
				if (!success)
					thrd_yield();
			}
		}
	}
	reclaim(get_ptr(head));
	return true;
}

