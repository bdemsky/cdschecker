#ifndef _MY_QUEUE_H
#define _MY_QUEUE_H

#include <stdatomic.h>

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

/**
	@Begin
	@Global_define:
		@DeclareStruct:
		typedef struct tag_elem {
			call_id_t id;
			unsigned int data;
		} tag_elem_t;
		
		@DeclareVar:
		spec_list *__queue;
		id_tag_t *tag;
		@InitVar:
			__queue = new_spec_list();
			tag = new_id_tag(); // Beginning of available id
		@DefineFunc:
			tag_elem_t* new_tag_elem(call_id_t id, unsigned int data) {
				tag_elem_t *e = (tag_elem_t*) MODEL_MALLOC(sizeof(tag_elem_t));
				e->id = id;
				e->data = data;
				return e;
			}
		@DefineFunc:
			void free_tag_elem(tag_elem_t *e) {
				free(e);
			}
		@DefineFunc:
			call_id_t get_id(void *wrapper) {
				return ((tag_elem_t*) wrapper)->id;
			}
		@DefineFunc:
			unsigned int get_data(void *wrapper) {
				return ((tag_elem_t*) wrapper)->data;
			}
	@Happens_before:
		# Only check the happens-before relationship according to the id of the
		# commit_point_set. For commit_point_set that has same ID, A -> B means
		# B happens after the previous A.
		Enqueue -> Dequeue
	@End
*/



/**
	@Begin
	@Interface: Enqueue
	@Commit_point_set: Enqueue_Success_Point
	@ID: get_and_inc(tag);
	@Action:
		# __ID__ is an internal macro that refers to the id of the current
		# interface call
		tag_elem_t *elem = new_tag_elem(__ID__, val);
		push_back(__queue, elem);
	@End
*/
void enqueue(queue_t *q, unsigned int val);

/**
	@Begin
	@Interface: Dequeue
	@Commit_point_set: Dequeue_Success_Point | Dequeue_Empty_Point
	@ID: get_id(back(__queue))
	@Action:
		unsigned int _Old_Val = get_data(front(__queue));
		pop_front(__queue);
	@Post_check:
		_Old_Val == __RET__
	@End
*/
unsigned int dequeue(queue_t *q);
int get_thread_num();

#endif
