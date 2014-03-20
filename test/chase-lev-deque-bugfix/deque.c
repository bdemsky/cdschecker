#include <stdatomic.h>
#include <inttypes.h>
#include "deque.h"
#include <stdlib.h>
#include <stdio.h>

Deque * create() {
	Deque * q = (Deque *) calloc(1, sizeof(Deque));
	Array * a = (Array *) calloc(1, sizeof(Array)+2*sizeof(atomic_int));
	atomic_store_explicit(&q->array, a, memory_order_relaxed);
	atomic_store_explicit(&q->top, 0, memory_order_relaxed);
	atomic_store_explicit(&q->bottom, 0, memory_order_relaxed);
	atomic_store_explicit(&a->size, 2, memory_order_relaxed);
	return q;
}



int take(Deque * q) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 0; // Take
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	int __RET__ = __wrapper__take(q);
	model_print("take: %d\n", __RET__);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 0; // Take
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Take_info* info = (Take_info*) malloc(sizeof(Take_info));
	info->__RET__ = __RET__;
	info->q = q;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 0; // Take
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
	return __RET__;
}

int __wrapper__take(Deque * q) {
	size_t b = atomic_load_explicit(&q->bottom, memory_order_relaxed) - 1;
	Array *a = (Array *) atomic_load_explicit(&q->array, memory_order_relaxed);
	atomic_store_explicit(&q->bottom, b, memory_order_relaxed);
	atomic_thread_fence(memory_order_seq_cst);
	size_t t = atomic_load_explicit(&q->top, memory_order_relaxed);
	/* Automatically generated code for commit point define check: Take_Point_1 */

	if (t > b) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 0;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
	
	int x;
	if (t <= b) {
		
		int size = atomic_load_explicit(&a->size,memory_order_relaxed);
		x = atomic_load_explicit(&a->buffer[b % size], memory_order_relaxed);
	/* Automatically generated code for commit point define check: Take_Point2 */

	if (t != b) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 1;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
		
		if (t == b) {
			
			bool succ = atomic_compare_exchange_strong_explicit(&q->top, &t, t +
				1, memory_order_seq_cst, memory_order_relaxed);
	/* Automatically generated code for commit point define check: Take_Point3 */

	if (succ) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 2;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
			
			if (!succ) {
				
				x = EMPTY;
			}
			atomic_store_explicit(&q->bottom, b + 1, memory_order_relaxed);
		}
	} else { 
		x = EMPTY;
		atomic_store_explicit(&q->bottom, b + 1, memory_order_relaxed);
	}
	return x;
}

void resize(Deque *q) {
	Array *a = (Array *) atomic_load_explicit(&q->array, memory_order_relaxed);
	size_t size=atomic_load_explicit(&a->size, memory_order_relaxed);
	size_t new_size=size << 1;
	Array *new_a = (Array *) calloc(1, new_size * sizeof(atomic_int) + sizeof(Array));
	size_t top=atomic_load_explicit(&q->top, memory_order_relaxed);
	size_t bottom=atomic_load_explicit(&q->bottom, memory_order_relaxed);
	atomic_store_explicit(&new_a->size, new_size, memory_order_relaxed);
	size_t i;
	for(i=top; i < bottom; i++) {
		atomic_store_explicit(&new_a->buffer[i % new_size], atomic_load_explicit(&a->buffer[i % size], memory_order_relaxed), memory_order_relaxed);
	}
	atomic_store_explicit(&q->array, new_a, memory_order_release);
	printf("resize\n");
}


void push(Deque * q, int x) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 1; // Push
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	__wrapper__push(q, x);
	model_print("push: %d\n", x);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 1; // Push
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Push_info* info = (Push_info*) malloc(sizeof(Push_info));
	info->q = q;
	info->x = x;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 1; // Push
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
}

void __wrapper__push(Deque * q, int x) {
	size_t b = atomic_load_explicit(&q->bottom, memory_order_relaxed);
	size_t t = atomic_load_explicit(&q->top, memory_order_acquire);
	Array *a = (Array *) atomic_load_explicit(&q->array, memory_order_relaxed);
	if (b - t > atomic_load_explicit(&a->size, memory_order_relaxed) - 1)  {
		resize(q);
				a = (Array *) atomic_load_explicit(&q->array, memory_order_relaxed);
	}
	int size = atomic_load_explicit(&a->size, memory_order_relaxed);
	atomic_store_explicit(&a->buffer[b % size], x, memory_order_relaxed);
	/* Automatically generated code for commit point define check: Push_Point */

	if (true) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 3;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
	
	atomic_thread_fence(memory_order_release);
	
	atomic_store_explicit(&q->bottom, b + 1, memory_order_relaxed);
	
}


int steal(Deque * q) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 2; // Steal
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	int __RET__ = __wrapper__steal(q);
	model_print("steal: %d\n", __RET__);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 2; // Steal
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Steal_info* info = (Steal_info*) malloc(sizeof(Steal_info));
	info->__RET__ = __RET__;
	info->q = q;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 2; // Steal
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
	return __RET__;
}

int __wrapper__steal(Deque * q) {
	size_t t = atomic_load_explicit(&q->top, memory_order_acquire);
	atomic_thread_fence(memory_order_seq_cst);
	size_t b = atomic_load_explicit(&q->bottom, memory_order_acquire);
	/* Automatically generated code for commit point define check: Steal_Point1 */

	if (t >= b) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 4;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
	
	int x = EMPTY;
	if (t < b) {
		
		Array *a = (Array *) atomic_load_explicit(&q->array, memory_order_acquire);
		int size = atomic_load_explicit(&a->size, memory_order_relaxed);
		x = atomic_load_explicit(&a->buffer[t % size], memory_order_relaxed);
	/* Automatically generated code for potential commit point: Potential_Steal */

	if (true) {
		struct anno_potential_cp_define *potential_cp_define = (struct anno_potential_cp_define*) malloc(sizeof(struct anno_potential_cp_define));
		potential_cp_define->label_num = 5;
		struct spec_annotation *annotation_potential_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_potential_cp_define->type = POTENTIAL_CP_DEFINE;
		annotation_potential_cp_define->annotation = potential_cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_potential_cp_define);
	}
		
		
		bool succ = atomic_compare_exchange_strong_explicit(&q->top, &t, t + 1,
			memory_order_seq_cst, memory_order_relaxed);
	/* Automatically generated code for commit point define check: Steal_Point4 */

	if (! succ) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 6;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
		

	/* Automatically generated code for commit point define: Steal_Point3 */

	if (succ) {
		struct anno_cp_define *cp_define = (struct anno_cp_define*) malloc(sizeof(struct anno_cp_define));
		cp_define->label_num = 7;
		cp_define->potential_cp_label_num = 5;
		struct spec_annotation *annotation_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define->type = CP_DEFINE;
		annotation_cp_define->annotation = cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define);
	}
		
		if (!succ) {
			return ABORT;
		}
	}
	return x;
}

