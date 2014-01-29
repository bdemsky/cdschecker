#include "spec_lib.h"
#include "model_memory.h"
#include "common.h"

/* Wrapper of basic types */
int_wrapper* wrap_int(int data) {
	int_wrapper *res = (int_wrapper*) MODEL_MALLOC(sizeof(int_wrapper));
	res->data = data;
	return res;
}

int unwrap_int(void *wrapper) {
	return ((int_wrapper*) wrapper)->data;
}

uint_wrapper* wrap_uint(unsigned int data) {
	uint_wrapper *res = (uint_wrapper*) MODEL_MALLOC(sizeof(uint_wrapper));
	res->data = data;
	return res;
}

unsigned int unwrap_uint(void *wrapper) {
	return ((uint_wrapper*) wrapper)->data;
}

/* End of wrapper of basic types */

/* ID of interface call */
id_tag_t* new_id_tag() {
	id_tag_t *res = (id_tag_t*) MODEL_MALLOC(sizeof(id_tag_t));
	res->tag = 1;
	return res;
}

void free_id_tag(id_tag_t *tag) {
	MODEL_FREE(tag);
}

call_id_t current(id_tag_t *tag) {
	return tag->tag;
}

call_id_t get_and_inc(id_tag_t *tag) {
	call_id_t cur = tag->tag;
	tag->tag++;
	return cur;
}

void next(id_tag_t *tag) {
	tag->tag++;
}

call_id_t default_id() {
	return DEFAULT_CALL_ID;
}

/* End of interface ID */

/* Sequential List */
spec_list* new_spec_list() {
	spec_list *list = (spec_list*) MODEL_MALLOC(sizeof(spec_list));
	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
	return list;
}

void free_spec_list(spec_list *list) {
	list_node *cur = list->head, *next;
	// Free all the nodes
	while (cur != NULL) {
		next = cur->next;
		MODEL_FREE(cur);
		cur = next;
	}
	MODEL_FREE(list);
}

void* front(spec_list *list) {
	return list->head == NULL ? NULL : list->head->data;
}

void* back(spec_list *list) {
	return list->tail == NULL ? NULL : list->tail->data;
}

void push_back(spec_list *list, void *elem) {
	list_node *new_node = (list_node*) MODEL_MALLOC(sizeof(list_node));
	new_node->data = elem;
	new_node->next = NULL;
	new_node->prev = list->tail; // Might be null here for an empty list
	list->size++;
	if (list->tail != NULL) {
		list->tail->next = new_node;
	} else { // An empty list
		list->head = new_node;
	}
	list->tail = new_node;
}

void push_front(spec_list *list, void *elem) {
	list_node *new_node = (list_node*) MODEL_MALLOC(sizeof(list_node));
	new_node->data = elem;
	new_node->prev = NULL;
	new_node->next = list->head; // Might be null here for an empty list
	list->size++;
	if (list->head != NULL) {
		list->head->prev = new_node;
	} else { // An empty list
		list->tail = new_node;
	}
	list->head = new_node;
}

int size(spec_list *list) {
	return list->size;
}

bool pop_back(spec_list *list) {
	if (list->size == 0) {
		return false;
	}
	list->size--; // Decrease size first
	if (list->head == list->tail) { // Only 1 element
		MODEL_FREE(list->head);
		list->head = NULL;
		list->tail = NULL;
	} else { // More than 1 element
		list_node *to_delete = list->tail;
		list->tail = list->tail->prev;
		MODEL_FREE(to_delete); // Don't forget to free the node
	}
	return true;
}

bool pop_front(spec_list *list) {
	if (list->size == 0) {
		return false;
	}
	list->size--; // Decrease size first
	if (list->head == list->tail) { // Only 1 element
		MODEL_FREE(list->head);
		list->head = NULL;
		list->tail = NULL;
	} else { // More than 1 element
		list_node *to_delete = list->head;
		list->head = list->head->next;
		MODEL_FREE(to_delete); // Don't forget to free the node
	}
	return true;
}

bool insert_at_index(spec_list *list, int idx, void *elem) {
	if (idx > list->size) return false;
	if (idx == 0) { // Insert from front
		push_front(list, elem);
	} else if (idx == list->size) { // Insert from back
		push_back(list, elem);
	} else { // Insert in the middle
		list_node *cur = list->head;
		for (int i = 1; i < idx; i++) {
			cur = cur->next;
		}
		list_node *new_node = (list_node*) MODEL_MALLOC(sizeof(list_node));
		new_node->prev = cur;
		new_node->next = cur->next;
		new_node->next->prev = new_node;
		cur->next = new_node;
		list->size++;
	}
	return true;
}

bool remove_at_index(spec_list *list, int idx) {
	if (idx < 0 && idx >= list->size)
		return false;
	if (idx == 0) { // Pop the front
		return pop_front(list);
	} else if (idx == list->size - 1) { // Pop the back
		return pop_back(list);
	} else { // Pop in the middle
		list_node *to_delete = list->head;
		for (int i = 0; i < idx; i++) {
			to_delete = to_delete->next;
		}
		to_delete->prev->next = to_delete->next;
		to_delete->next->prev = to_delete->prev;
		MODEL_FREE(to_delete);
		return true;
	}
}

static list_node* node_at_index(spec_list *list, int idx) {
	if (idx < 0 || idx >= list->size || list->size == 0)
		return NULL;
	list_node *cur = list->head;
	for (int i = 0; i < idx; i++) {
		cur = cur->next;
	}
	return cur;
}

void* elem_at_index(spec_list *list, int idx) {
	list_node *n = node_at_index(list, idx);
	return n == NULL ? NULL : n->data;
}

spec_list* sub_list(spec_list *list, int from, int to) {
	if (from < 0 || to > list->size || from >= to)
		return NULL;
	spec_list *new_list = new_spec_list();
	if (list->size == 0) {
		return new_list;
	} else {
		list_node *cur = node_at_index(list, from);
		for (int i = from; i < to; i++) {
			push_back(new_list, cur->data);
			cur = cur->next;
		}
	}
	return new_list;
}

/* End of sequential list */
