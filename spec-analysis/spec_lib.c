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

/* Sequential hashtable */

spec_table* new_spec_table_default(bool (*comparator)(void*, void*)) {
	spec_table *t = (spec_table*) MODEL_MALLOC(sizeof(spec_table));
	// Init hashtable parameters
	t->_comparator = comparator;

	unsigned int initialcapacity = 1024;
	double factor = 0.5;
	// Allocate space for the hash table
	t->table = (struct spec_table_node*) MODEL_CALLOC(initialcapacity, sizeof(struct spec_table_node));
	t->loadfactor = factor;
	t->capacity = initialcapacity;
	t->capacitymask = initialcapacity - 1;

	t->threshold = (unsigned int)(initialcapacity * t->loadfactor);
	t->size = 0; // Initial number of elements in the hash
	return t;
}

spec_table* new_spec_table(bool (*comparator)(void*, void*), unsigned int
	initialcapacity, double factor) {
	spec_table *t = (spec_table*) MODEL_MALLOC(sizeof(spec_table));
	// Init hashtable parameters
	t->_comparator = comparator;

	// Allocate space for the hash table
	t->table = (struct spec_table_node*) MODEL_CALLOC(initialcapacity, sizeof(struct spec_table_node));
	t->loadfactor = factor;
	t->capacity = initialcapacity;
	t->capacitymask = initialcapacity - 1;

	t->threshold = (unsigned int)(initialcapacity * t->loadfactor);
	t->size = 0; // Initial number of elements in the hash
	return t;
}

void free_spec_table(spec_table *t) {
	MODEL_FREE(t->table);
}

void spec_table_reset(spec_table *t) {
	memset(t->table, 0, t->capacity * sizeof(struct spec_table_node));
	t->size = 0;
}

static void spec_table_resize(spec_table *t, unsigned int newsize);

void spec_table_put(spec_table *t, void *key, void *val) {
	/* HashTable cannot handle 0 as a key */
	//ASSERT(key);

	if (t->size > t->threshold)
		spec_table_resize(t, t->capacity << 1);

	struct spec_table_node *search;

	unsigned int index = (unsigned int) key;
	do {
		index &= t->capacitymask;
		search = &t->table[index];
		if (t->_comparator(search->key, key)) {
			search->val = val;
			return;
		}
		index++;
	} while (search->key);

	search->key = key;
	search->val = val;
	t->size++;
}

void* spec_table_get(spec_table *t, void *key) {
	struct spec_table_node *search;

	/* HashTable cannot handle 0 as a key */
	//ASSERT(key);

	unsigned int index = (unsigned int) key;
	do {
		index &= t->capacitymask;
		search = &t->table[index];
		if (t->_comparator(search->key, key))
			return search->val;
		index++;
	} while (search->key);
	return NULL;
}

bool spec_table_contains(spec_table *t, void *key) {
	struct spec_table_node *search;

	/* HashTable cannot handle 0 as a key */
	//ASSERT(key);

	unsigned int index = (unsigned int) key;
	do {
		index &= t->capacitymask;
		search = &t->table[index];
		if (t->_comparator(search->key, key))
			return true;
		index++;
	} while (search->key);
	return false;
}

static void spec_table_resize(spec_table *t, unsigned int newsize) {
	struct spec_table_node *oldtable = t->table;
	struct spec_table_node *newtable;
	unsigned int oldcapacity = t->capacity;

	if ((newtable = (struct spec_table_node*)MODEL_CALLOC(newsize, sizeof(struct spec_table_node))) == NULL) {
		model_print("calloc error %s %d\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}

	t->table = newtable;          // Update the global hashtable upon spec_table_resize()
	t->capacity = newsize;
	t->capacitymask = newsize - 1;

	t->threshold = (unsigned int)(newsize * t->loadfactor);

	struct spec_table_node *bin = &oldtable[0];
	struct spec_table_node *lastbin = &oldtable[oldcapacity];
	for (; bin < lastbin; bin++) {
		void *key = bin->key;

		struct spec_table_node *search;

		unsigned int index = (unsigned int) key;
		do {
			index &= t->capacitymask;
			search = &t->table[index];
			index++;
		} while (search->key);

		search->key = key;
		search->val = bin->val;
	}

	MODEL_FREE(oldtable);            // Free the memory of the old hash table
}

/* End of sequential hashtable */
