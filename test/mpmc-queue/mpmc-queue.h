#include <stdatomic.h>
#include <unrelacy.h>
#include <common.h>

#include <spec_lib.h>
#include <stdlib.h>
#include <cdsannotate.h>
#include <specannotation.h>
#include <model_memory.h>


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
typedef struct elem {
t_element * pos ;
bool written ;
thread_id_t tid ;
thread_id_t fetch_tid ;
call_id_t id ;
}
elem ;

static spec_list * list;
/* All other user-defined functions */
/* Definition of interface info struct: Publish */
typedef struct Publish_info {
t_element * bin;
} Publish_info;
/* End of info struct definition: Publish */

/* ID function of interface: Publish */
inline static call_id_t Publish_id(void *info, thread_id_t __TID__) {
	Publish_info* theInfo = (Publish_info*)info;
	t_element * bin = theInfo->bin;

	call_id_t __ID__ = ( call_id_t ) bin;
	return __ID__;
}
/* End of ID function: Publish */

/* Check action function of interface: Publish */
inline static bool Publish_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Publish_info* theInfo = (Publish_info*)info;
	t_element * bin = theInfo->bin;

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

/* Definition of interface info struct: Prepare */
typedef struct Prepare_info {
t_element * __RET__;
} Prepare_info;
/* End of info struct definition: Prepare */

/* ID function of interface: Prepare */
inline static call_id_t Prepare_id(void *info, thread_id_t __TID__) {
	Prepare_info* theInfo = (Prepare_info*)info;
	t_element * __RET__ = theInfo->__RET__;

	call_id_t __ID__ = ( call_id_t ) __RET__;
	return __ID__;
}
/* End of ID function: Prepare */

/* Check action function of interface: Prepare */
inline static bool Prepare_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Prepare_info* theInfo = (Prepare_info*)info;
	t_element * __RET__ = theInfo->__RET__;

	return true;
}
/* End of check action function: Prepare */

/* Definition of interface info struct: Consume */
typedef struct Consume_info {
t_element * bin;
} Consume_info;
/* End of info struct definition: Consume */

/* ID function of interface: Consume */
inline static call_id_t Consume_id(void *info, thread_id_t __TID__) {
	Consume_info* theInfo = (Consume_info*)info;
	t_element * bin = theInfo->bin;

	call_id_t __ID__ = ( call_id_t ) bin;
	return __ID__;
}
/* End of ID function: Consume */

/* Check action function of interface: Consume */
inline static bool Consume_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Consume_info* theInfo = (Consume_info*)info;
	t_element * bin = theInfo->bin;

	return true;
}
/* End of check action function: Consume */

#define INTERFACE_SIZE 4
static void** func_ptr_table;
static anno_hb_init** hb_init_table;

/* Define function for sequential code initialization */
inline static void __sequential_init() {
	/* Init func_ptr_table */
	func_ptr_table = (void**) malloc(sizeof(void*) * 4 * 2);
	func_ptr_table[2 * 3] = (void*) &Publish_id;
	func_ptr_table[2 * 3 + 1] = (void*) &Publish_check_action;
	func_ptr_table[2 * 0] = (void*) &Fetch_id;
	func_ptr_table[2 * 0 + 1] = (void*) &Fetch_check_action;
	func_ptr_table[2 * 2] = (void*) &Prepare_id;
	func_ptr_table[2 * 2 + 1] = (void*) &Prepare_check_action;
	func_ptr_table[2 * 1] = (void*) &Consume_id;
	func_ptr_table[2 * 1 + 1] = (void*) &Consume_check_action;
	/* Consume(true) -> Prepare(true) */
	struct anno_hb_init *hbConditionInit0 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit0->interface_num_before = 1; // Consume
	hbConditionInit0->hb_condition_num_before = 0; // 
	hbConditionInit0->interface_num_after = 2; // Prepare
	hbConditionInit0->hb_condition_num_after = 0; // 
	/* Publish(true) -> Fetch(true) */
	struct anno_hb_init *hbConditionInit1 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit1->interface_num_before = 3; // Publish
	hbConditionInit1->hb_condition_num_before = 0; // 
	hbConditionInit1->interface_num_after = 0; // Fetch
	hbConditionInit1->hb_condition_num_after = 0; // 
	/* Init hb_init_table */
	hb_init_table = (anno_hb_init**) malloc(sizeof(anno_hb_init*) * 2);
	#define HB_INIT_TABLE_SIZE 2
	hb_init_table[0] = hbConditionInit0;
	hb_init_table[1] = hbConditionInit1;
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

	list = new_spec_list ( ) ;
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
	/* Automatically generated code for potential commit point: Fetch_Potential_Point */

	if (true) {
		struct anno_potential_cp_define *potential_cp_define = (struct anno_potential_cp_define*) malloc(sizeof(struct anno_potential_cp_define));
		potential_cp_define->label_num = 0;
		struct spec_annotation *annotation_potential_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_potential_cp_define->type = POTENTIAL_CP_DEFINE;
		annotation_potential_cp_define->annotation = potential_cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_potential_cp_define);
	}
				
		unsigned int rd,wr;
		for(;;) {
			rd = (rdwr>>16) & 0xFFFF;
			wr = rdwr & 0xFFFF;

			if ( wr == rd ) { 
	/* Automatically generated code for commit point define: Fetch_Empty_Point */

	if (true) {
		struct anno_cp_define *cp_define = (struct anno_cp_define*) malloc(sizeof(struct anno_cp_define));
		cp_define->label_num = 1;
		cp_define->potential_cp_label_num = 0;
		cp_define->interface_num = 0;
		struct spec_annotation *annotation_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define->type = CP_DEFINE;
		annotation_cp_define->annotation = cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define);
	}
				

				return false;
			}
			
			bool succ = m_rdwr.compare_exchange_weak(rdwr,rdwr+(1<<16),mo_acq_rel);
	/* Automatically generated code for commit point define check: Fetch_Succ_Point */

	if (succ == true) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 2;
		cp_define_check->interface_num = 0;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
			
			if (succ)
				break;
			else
				thrd_yield();
		}

				rl::backoff bo;
		while ( (m_written.load(mo_acquire) & 0xFFFF) != wr ) {
			thrd_yield();
		}

		t_element * p = & ( m_array[ rd % t_size ] );
		
		return p;
	}


void read_consume(t_element * bin) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 1; // Consume
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	__wrapper__read_consume(bin);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 1; // Consume
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Consume_info* info = (Consume_info*) malloc(sizeof(Consume_info));
	info->bin = bin;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 1; // Consume
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
}
	
void __wrapper__read_consume(t_element * bin) {
		
		m_read.fetch_add(1,mo_release);
	/* Automatically generated code for commit point define check: Consume_Point */

	if (true) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 3;
		cp_define_check->interface_num = 1;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
		
	}

	

t_element * write_prepare() {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 2; // Prepare
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	t_element * __RET__ = __wrapper__write_prepare();
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 2; // Prepare
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Prepare_info* info = (Prepare_info*) malloc(sizeof(Prepare_info));
	info->__RET__ = __RET__;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 2; // Prepare
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
	return __RET__;
}
	
t_element * __wrapper__write_prepare() {
				unsigned int rdwr = m_rdwr.load(mo_acquire);
	/* Automatically generated code for potential commit point: Prepare_Potential_Point */

	if (true) {
		struct anno_potential_cp_define *potential_cp_define = (struct anno_potential_cp_define*) malloc(sizeof(struct anno_potential_cp_define));
		potential_cp_define->label_num = 4;
		struct spec_annotation *annotation_potential_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_potential_cp_define->type = POTENTIAL_CP_DEFINE;
		annotation_potential_cp_define->annotation = potential_cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_potential_cp_define);
	}
				
		unsigned int rd,wr;
		for(;;) {
			rd = (rdwr>>16) & 0xFFFF;
			wr = rdwr & 0xFFFF;

			if ( wr == ((rd + t_size)&0xFFFF) ) { 
	/* Automatically generated code for commit point define: Prepare_Full_Point */

	if (true) {
		struct anno_cp_define *cp_define = (struct anno_cp_define*) malloc(sizeof(struct anno_cp_define));
		cp_define->label_num = 5;
		cp_define->potential_cp_label_num = 4;
		cp_define->interface_num = 2;
		struct spec_annotation *annotation_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define->type = CP_DEFINE;
		annotation_cp_define->annotation = cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define);
	}
				
				return NULL;
			}
			
			bool succ = m_rdwr.compare_exchange_weak(rdwr,(rd<<16) |
				((wr+1)&0xFFFF),mo_acq_rel);
	/* Automatically generated code for commit point define check: Prepare_Succ_Point */

	if (succ == true) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 6;
		cp_define_check->interface_num = 2;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
			
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


void write_publish(t_element * bin) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 3; // Publish
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	__wrapper__write_publish(bin);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 3; // Publish
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Publish_info* info = (Publish_info*) malloc(sizeof(Publish_info));
	info->bin = bin;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 3; // Publish
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
}
	
void __wrapper__write_publish(t_element * bin)
	{
		
		m_written.fetch_add(1,mo_release);
	/* Automatically generated code for commit point define check: Publish_Point */

	if (true) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 7;
		cp_define_check->interface_num = 3;
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
spec_list * mpmc_boundq_1_alt<t_element, t_size>::list;


