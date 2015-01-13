#ifndef _SPECANNOTATION_H
#define _SPECANNOTATION_H

#include "spec_lib.h"
#include "modeltypes.h"

#define SPEC_ANALYSIS 1

typedef call_id_t (*id_func_t)(void *info, thread_id_t tid);
typedef bool (*check_action_func_t)(void *info, call_id_t id, thread_id_t tid);
//typedef bool (*check_action_func_t)(void *info, call_id_t id);

typedef enum spec_anno_type {
	INIT, INTERFACE_BEGIN, POTENTIAL_CP_DEFINE, CP_DEFINE,
	CP_DEFINE_CHECK, CP_CLEAR, INTERFACE_END, HB_CONDITION
} spec_anno_type;

typedef
struct spec_annotation {
	spec_anno_type type;
	void *annotation;
} spec_annotation;


/**
	The interfaces and their HB conditions are represented by integers.
*/
typedef
struct anno_hb_init {
	int interface_num_before;
	int hb_condition_num_before;
	int interface_num_after;
	int hb_condition_num_after;
} anno_hb_init;


typedef
struct anno_init {
	void **func_table;
	int func_table_size;
	anno_hb_init **hb_init_table;
	int hb_init_table_size;
} anno_init;


inline void free_anno_init(anno_init *tgt) {
	free(tgt->func_table);
	for (int i = 0; i < tgt->hb_init_table_size; i++) {
		free(tgt->hb_init_table[i]);
	}
	free(tgt->hb_init_table);
}


typedef
struct anno_interface_begin {
	int interface_num;
	const char *interface_name;
	char *msg; // For debugging only
} anno_interface_begin;

/* 
	The follwoing function pointers can be put in a static table, not
	necessarily stored in every function call.

	id_func_t id;
	check_action_func_t check_action;
*/
typedef
struct anno_interface_end {
	int interface_num;
	void *info; // Return value & arguments
} anno_interface_end;

typedef
struct anno_potential_cp_define {
	int label_num;
	const char *label_name;
} anno_potential_cp_define;

typedef
struct anno_cp_define {
	int label_num;
	const char *label_name;
	int potential_cp_label_num;
	const char *potential_label_name;
	int interface_num;
} anno_cp_define;

typedef
struct anno_cp_define_check {
	int label_num;
	const char *label_name;
	int interface_num;
} anno_cp_define_check;

typedef
struct anno_cp_clear {
	//int label_num;
	//int interface_num;
} anno_cp_clear;


typedef
struct anno_hb_condition {
	int interface_num;
	int hb_condition_num;
} anno_hb_condition;

#endif
