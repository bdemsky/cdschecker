#include <stdatomic.h>
#include <unrelacy.h>
#include <common.h>

#include <spec_lib.h>
#include <stdlib.h>
#include <cdsannotate.h>
#include <specannotation.h>
#include <model_memory.h>
#include "common.h" 


template <typename t_element, size_t t_size>
struct mpmc_boundq_1_alt
{
private:

		t_element               m_array[t_size];

		atomic<unsigned int>    m_rdwr;
		atomic<unsigned int>    m_read;
	atomic<unsigned int>    m_written;

public:

	mpmc_boundq_1_alt()
	{
	__sequential_init();
    	
		m_rdwr = 0;
		m_read = 0;
		m_written = 0;
	}
	

/* All other user-defined structs */
static id_tag_t * tag;
/* All other user-defined functions */
/* Definition of interface info struct: Publish */
typedef struct Publish_info {
t_element * elem;
} Publish_info;
/* End of info struct definition: Publish */

/* ID function of interface: Publish */
inline static call_id_t Publish_id(void *info, thread_id_t __TID__) {
	Publish_info* theInfo = (Publish_info*)info;
	t_element * elem = theInfo->elem;

	call_id_t __ID__ = ( uint64_t ) elem;
	return __ID__;
}
/* End of ID function: Publish */

/* Check action function of interface: Publish */
inline static bool Publish_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Publish_info* theInfo = (Publish_info*)info;
	t_element * elem = theInfo->elem;

	return true;
}
/* End of check action function: Publish */

/* Definition of interface info struct: Fetch */
typedef struct Fetch_info {
t_element * __RET__;
} Fetch_info;
/* End of info struct definition: Fetch */

/* ID function of interface: Fetch */
inline static call_id_t Fetch_id(void *info, thread_id_t __TID__) {
	Fetch_info* theInfo = (Fetch_info*)info;
	t_element * __RET__ = theInfo->__RET__;

	call_id_t __ID__ = ( call_id_t ) __RET__;
	return __ID__;
}
/* End of ID function: Fetch */

/* Check action function of interface: Fetch */
inline static bool Fetch_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Fetch_info* theInfo = (Fetch_info*)info;
	t_element * __RET__ = theInfo->__RET__;

	return true;
}
/* End of check action function: Fetch */

#define INTERFACE_SIZE 2
static void** func_ptr_table;
static anno_hb_init** hb_init_table;

/* Define function for sequential code initialization */
inline static void __sequential_init() {
	/* Init func_ptr_table */
	func_ptr_table = (void**) malloc(sizeof(void*) * 2 * 2);
	func_ptr_table[2 * 1] = (void*) &Publish_id;
	func_ptr_table[2 * 1 + 1] = (void*) &Publish_check_action;
	func_ptr_table[2 * 0] = (void*) &Fetch_id;
	func_ptr_table[2 * 0 + 1] = (void*) &Fetch_check_action;
	/* Publish(true) -> Fetch(true) */
	struct anno_hb_init *hbConditionInit0 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit0->interface_num_before = 1; // Publish
	hbConditionInit0->hb_condition_num_before = 0; // 
	hbConditionInit0->interface_num_after = 0; // Fetch
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

	tag = NULL ;
}
/* End of Global construct generation in class */
	

	

t_element * read_fetch() {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 0; // Fetch
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	t_element * __RET__ = __wrapper__read_fetch();
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 0; // Fetch
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Fetch_info* info = (Fetch_info*) malloc(sizeof(Fetch_info));
	info->__RET__ = __RET__;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 0; // Fetch
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
	return __RET__;
}
	
t_element * __wrapper__read_fetch() {
		unsigned int rdwr = m_rdwr.load(mo_acquire);
	/* Automatically generated code for commit point define check: Fetch_Succ_Point1 */

	if (( rdwr >> 16 ) & 0xFFFF == rdwr & 0xFFFF) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 0;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
		
		unsigned int rd,wr;
		for(;;) {
			rd = (rdwr>>16) & 0xFFFF;
			wr = rdwr & 0xFFFF;

			if ( wr == rd ) { 				return false;
			}
			
			bool succ = m_rdwr.compare_exchange_weak(rdwr,rdwr+(1<<16),mo_acq_rel);
			
			if (succ)
				break;
			else
				thrd_yield();
		}

				rl::backoff bo;
		while ( true ) {
			unsigned int tmp = m_written.load(mo_acquire);
	/* Automatically generated code for commit point define check: Fetch_Succ_Point2 */

	if (( tmp & 0xFFFF ) == wr) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 1;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
			
			if ((tmp & 0xFFFF) == wr)
				break;
			thrd_yield();
		}

		t_element * p = & ( m_array[ rd % t_size ] );
		
		return p;
	}

	void read_consume() {
		m_read.fetch_add(1,mo_release);
	}

	
	t_element * write_prepare() {
		unsigned int rdwr = m_rdwr.load(mo_acquire);
		unsigned int rd,wr;
		for(;;) {
			rd = (rdwr>>16) & 0xFFFF;
			wr = rdwr & 0xFFFF;

			if ( wr == ((rd + t_size)&0xFFFF) ) { 				return NULL;
			}
			
			bool succ = m_rdwr.compare_exchange_weak(rdwr,(rd<<16) |
				((wr+1)&0xFFFF),mo_acq_rel);
			if (succ)
				break;
			else
				thrd_yield();
		}

				rl::backoff bo;
		while ( (m_read.load(mo_acquire) & 0xFFFF) != rd ) {
			thrd_yield();
		}

		t_element * p = & ( m_array[ wr % t_size ] );

		return p;
	}


void write_publish(t_element * elem) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 1; // Publish
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	__wrapper__write_publish(elem);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 1; // Publish
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Publish_info* info = (Publish_info*) malloc(sizeof(Publish_info));
	info->elem = elem;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 1; // Publish
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
}
	
void __wrapper__write_publish(t_element * elem)
	{
		m_written.fetch_add(1,mo_release);
	/* Automatically generated code for commit point define check: Publish_Point */

	if (true) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 2;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
		
	}

	

};
template <typename t_element, size_t t_size>
void** mpmc_boundq_1_alt<t_element, t_size>::func_ptr_table;
template <typename t_element, size_t t_size>
anno_hb_init** mpmc_boundq_1_alt<t_element, t_size>::hb_init_table;
template <typename t_element, size_t t_size>
id_tag_t * mpmc_boundq_1_alt<t_element, t_size>::tag;


