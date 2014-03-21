#ifndef _SPEC_LIB_H
#define _SPEC_LIB_H


#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* This file defines the basic sequential data structures that can be used by
 * the specification
 */

#ifdef __cplusplus
extern "C"{
#endif

/* Some wrapper for basic types */
typedef struct int_wrapper {
	int data;
} int_wrapper;

int_wrapper* wrap_int(int data);

int unwrap_int(void *wrapper);

typedef struct uint_wrapper {
	unsigned data;
} uint_wrapper;

uint_wrapper* wrap_uint(unsigned int data);

unsigned int unwrap_uint(void *wrapper);

/* ID of interface call */
typedef uint64_t call_id_t;

#define DEFAULT_CALL_ID 0

typedef struct id_tag {
	call_id_t tag;
} id_tag_t;


id_tag_t* new_id_tag();

void free_id_tag(id_tag_t *tag);

call_id_t current(id_tag_t *tag);

call_id_t get_and_inc(id_tag_t *tag);

void next(id_tag_t *tag);

// Default ID for those interface spec without defining ID function
call_id_t default_id();


/* End of call ID */

/* Sequential List */
struct spec_list;
struct list_node;

typedef struct list_node {
	struct list_node *prev;
	struct list_node *next;
	void *data;
} list_node;

typedef struct spec_list {
	list_node *head;
	list_node *tail;
	int size;
} spec_list;


spec_list* new_spec_list();

void free_spec_list(spec_list *list);

void* front(spec_list *list);

void* back(spec_list *list);

void push_back(spec_list *list, void *elem);

void push_front(spec_list *list, void *elem);

int size(spec_list *list);

bool pop_back(spec_list *list);

bool pop_front(spec_list *list);

bool insert_at_index(spec_list *list, int idx, void *elem);

bool remove_at_index(spec_list *list, int idx);

void* elem_at_index(spec_list *list, int idx);

bool has_elem(spec_list *list, void *elem);

spec_list* sub_list(spec_list *list, int from, int to);

/* End of sequential list */

/* Sequential hashtable */

/**
 * @brief HashTable node
 */
typedef struct spec_table_node{
	void *key;
	void *val;
} spec_table_node;

typedef struct spec_table {
	bool (*_comparator)(void*, void*);
	struct spec_table_node *table;
	unsigned int capacity;
	unsigned int size;
	unsigned int capacitymask;
	unsigned int threshold;
	double loadfactor;
} spec_table;

spec_table* new_spec_table_default(bool (*comparator)(void*, void*));

spec_table* new_spec_table(bool (*comparator)(void*, void*), unsigned int initialcapacity, double factor);

void free_spec_table(spec_table *t);

void spec_table_reset(spec_table *t);

void spec_table_put(spec_table *t, void *key, void *val);

void* spec_table_get(spec_table *t, void *key);
	
bool spec_table_contains(spec_table *t, void *key);
	
/* End of hashtable */

#ifdef __cplusplus
} /* extern "C" */
#endif 

#endif
