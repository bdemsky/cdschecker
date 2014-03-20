
#include <stdatomic.h>
#include <unrelacy.h>

#include <spec_lib.h>
#include <stdlib.h>
#include <cdsannotate.h>
#include <specannotation.h>
#include <model_memory.h>
#include "common.h" 

struct mcs_node {
	std::atomic<mcs_node *> next;
	std::atomic<int> gate;

	mcs_node() {
		next.store(0);
		gate.store(0);
	}
};


struct mcs_mutex {
public:
		std::atomic<mcs_node *> m_tail;

	mcs_mutex() {
	__sequential_init();
		
		m_tail.store( NULL );
	}
	~mcs_mutex() {
			}

		class guard {
	public:
		mcs_mutex * m_t;
		mcs_node    m_node; 
		guard(mcs_mutex * t) : m_t(t) { t->lock(this); }
		~guard() { m_t->unlock(this); }
	};

/* All other user-defined structs */
static bool _lock_acquired;
/* All other user-defined functions */
/* Definition of interface info struct: Unlock */
typedef struct Unlock_info {
guard * I;
} Unlock_info;
/* End of info struct definition: Unlock */

/* ID function of interface: Unlock */
inline static call_id_t Unlock_id(void *info, thread_id_t __TID__) {
	Unlock_info* theInfo = (Unlock_info*)info;
	guard * I = theInfo->I;

	call_id_t __ID__ = 0;
	return __ID__;
}
/* End of ID function: Unlock */

/* Check action function of interface: Unlock */
inline static bool Unlock_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Unlock_info* theInfo = (Unlock_info*)info;
	guard * I = theInfo->I;

	check_passed = _lock_acquired == true;
	if (!check_passed)
		return false;
	_lock_acquired = false ;
	return true;
}
/* End of check action function: Unlock */

/* Definition of interface info struct: Lock */
typedef struct Lock_info {
guard * I;
} Lock_info;
/* End of info struct definition: Lock */

/* ID function of interface: Lock */
inline static call_id_t Lock_id(void *info, thread_id_t __TID__) {
	Lock_info* theInfo = (Lock_info*)info;
	guard * I = theInfo->I;

	call_id_t __ID__ = 0;
	return __ID__;
}
/* End of ID function: Lock */

/* Check action function of interface: Lock */
inline static bool Lock_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Lock_info* theInfo = (Lock_info*)info;
	guard * I = theInfo->I;

	check_passed = _lock_acquired == false ;;
	if (!check_passed)
		return false;
	_lock_acquired = true ;
	return true;
}
/* End of check action function: Lock */

#define INTERFACE_SIZE 2
static void** func_ptr_table;
static anno_hb_init** hb_init_table;

/* Define function for sequential code initialization */
inline static void __sequential_init() {
	/* Init func_ptr_table */
	func_ptr_table = (void**) malloc(sizeof(void*) * 2 * 2);
	func_ptr_table[2 * 1] = (void*) &Unlock_id;
	func_ptr_table[2 * 1 + 1] = (void*) &Unlock_check_action;
	func_ptr_table[2 * 0] = (void*) &Lock_id;
	func_ptr_table[2 * 0 + 1] = (void*) &Lock_check_action;
	/* Unlock(true) -> Lock(true) */
	struct anno_hb_init *hbConditionInit0 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit0->interface_num_before = 1; // Unlock
	hbConditionInit0->hb_condition_num_before = 0; // 
	hbConditionInit0->interface_num_after = 0; // Lock
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

	_lock_acquired = false ;
}
/* End of Global construct generation in class */
	


void lock(guard * I) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 0; // Lock
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	__wrapper__lock(I);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 0; // Lock
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Lock_info* info = (Lock_info*) malloc(sizeof(Lock_info));
	info->I = I;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 0; // Lock
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
}
	
void __wrapper__lock(guard * I) {
		mcs_node * me = &(I->m_node);

						me->next.store(NULL, std::mo_relaxed );
		me->gate.store(1, std::mo_relaxed );

				mcs_node * pred = m_tail.exchange(me, std::mo_acq_rel);
	/* Automatically generated code for commit point define check: Lock_Enqueue_Point1 */

	if (pred == NULL) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 0;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
		
		if ( pred != NULL ) {
						
						pred->next.store(me, std::mo_release );

			
									rl::linear_backoff bo;
			int my_gate = 1;
			while (my_gate ) {
				my_gate = me->gate.load(std::mo_acquire);
	/* Automatically generated code for commit point define check: Lock_Enqueue_Point2 */

	if (my_gate == 0) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 1;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
				
				thrd_yield();
			}
		}
	}


void unlock(guard * I) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 1; // Unlock
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	__wrapper__unlock(I);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 1; // Unlock
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Unlock_info* info = (Unlock_info*) malloc(sizeof(Unlock_info));
	info->I = I;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 1; // Unlock
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
}
	
void __wrapper__unlock(guard * I) {
		mcs_node * me = &(I->m_node);

		mcs_node * next = me->next.load(std::mo_acquire);
		if ( next == NULL )
		{
			mcs_node * tail_was_me = me;
			bool success;
			success = m_tail.compare_exchange_strong(
				tail_was_me,NULL,std::mo_acq_rel);
	/* Automatically generated code for commit point define check: Unlock_Point_Success_1 */

	if (success == true) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 2;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
			
			if (success) {
				
								return;
			}

						rl::linear_backoff bo;
			for(;;) {
				next = me->next.load(std::mo_acquire);
				if ( next != NULL )
					break;
				thrd_yield();
			}
		}

				
				next->gate.store( 0, std::mo_release );
	/* Automatically generated code for commit point define check: Unlock_Point_Success_2 */

	if (true) {
		struct anno_cp_define_check *cp_define_check = (struct anno_cp_define_check*) malloc(sizeof(struct anno_cp_define_check));
		cp_define_check->label_num = 3;
		struct spec_annotation *annotation_cp_define_check = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define_check->type = CP_DEFINE_CHECK;
		annotation_cp_define_check->annotation = cp_define_check;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define_check);
	}
		
	}
};
void** mcs_mutex::func_ptr_table;
anno_hb_init** mcs_mutex::hb_init_table;
bool mcs_mutex::_lock_acquired;


