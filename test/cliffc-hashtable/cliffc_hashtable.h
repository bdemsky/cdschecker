#ifndef CLIFFC_HASHTABLE_H
#define CLIFFC_HASHTABLE_H

#include <iostream>
#include <atomic>
#include "stdio.h" 
#ifdef STANDALONE
#include <assert.h>
#define MODEL_ASSERT assert 
#else
#include <model-assert.h>
#endif

#include <spec_lib.h>
#include <stdlib.h>
#include <cdsannotate.h>
#include <specannotation.h>
#include <model_memory.h>
#include "common.h" 

using namespace std;



template<typename TypeK, typename TypeV>
class cliffc_hashtable;


struct kvs_data {
	int _size;
	atomic<void*> *_data;
	
	kvs_data(int sz) {
		_size = sz;
		int real_size = sz * 2 + 2;
		_data = new atomic<void*>[real_size];
						int *hashes = new int[_size];
		int i;
		for (i = 0; i < _size; i++) {
			hashes[i] = 0;
		}
				for (i = 2; i < real_size; i++) {
			_data[i].store(NULL, memory_order_relaxed);
		}
		_data[1].store(hashes, memory_order_relaxed);
	}

	~kvs_data() {
		int *hashes = (int*) _data[1].load(memory_order_relaxed);
		delete hashes;
		delete[] _data;
	}
};

struct slot {
	bool _prime;
	void *_ptr;

	slot(bool prime, void *ptr) {
		_prime = prime;
		_ptr = ptr;
	}
};




template<typename TypeK, typename TypeV>
class cliffc_hashtable {
/* All other user-defined structs */
static spec_table * map;
static spec_table * id_map;
static id_tag_t * tag;
/* All other user-defined functions */
inline static bool equals_key ( void * ptr1 , void * ptr2 ) {
TypeK * key1 = ( TypeK * ) ptr1 , * key2 = ( TypeK * ) ptr2 ;
if ( key1 == NULL || key2 == NULL ) return false ;
return key1 -> equals ( key2 ) ;
}

inline static bool equals_val ( void * ptr1 , void * ptr2 ) {
if ( ptr1 == ptr2 ) return true ;
TypeV * val1 = ( TypeV * ) ptr1 , * val2 = ( TypeV * ) ptr2 ;
if ( val1 == NULL || val2 == NULL ) return false ;
return val1 -> equals ( val2 ) ;
}

inline static bool equals_id ( void * ptr1 , void * ptr2 ) {
id_tag_t * id1 = ( id_tag_t * ) ptr1 , * id2 = ( id_tag_t * ) ptr2 ;
if ( id1 == NULL || id2 == NULL ) return false ;
return ( * id1 ) . tag == ( * id2 ) . tag ;
}

inline static call_id_t getKeyTag ( TypeK * key ) {
if ( ! spec_table_contains ( id_map , key ) ) {
call_id_t cur_id = current ( tag ) ;
spec_table_put ( id_map , key , ( void * ) cur_id ) ;
next ( tag ) ;
return cur_id ;
}
else {
call_id_t res = ( call_id_t ) spec_table_get ( id_map , key ) ;
return res ;
}
}

/* Definition of interface info struct: Put */
typedef struct Put_info {
TypeV * __RET__;
TypeK * key;
TypeV * val;
} Put_info;
/* End of info struct definition: Put */

/* ID function of interface: Put */
inline static call_id_t Put_id(void *info, thread_id_t __TID__) {
	Put_info* theInfo = (Put_info*)info;
	TypeV * __RET__ = theInfo->__RET__;
	TypeK * key = theInfo->key;
	TypeV * val = theInfo->val;

	call_id_t __ID__ = getKeyTag ( key );
	return __ID__;
}
/* End of ID function: Put */

/* Check action function of interface: Put */
inline static bool Put_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Put_info* theInfo = (Put_info*)info;
	TypeV * __RET__ = theInfo->__RET__;
	TypeK * key = theInfo->key;
	TypeV * val = theInfo->val;

	TypeV * _Old_Val = ( TypeV * ) spec_table_get ( map , key ) ;
	spec_table_put ( map , key , val ) ;
	bool passed = false ;
	if ( ! passed ) {
	int old = _Old_Val == NULL ? 0 : _Old_Val -> _val ;
	int ret = __RET__ == NULL ? 0 : __RET__ -> _val ;
	}
	check_passed = equals_val ( __RET__ , _Old_Val );
	if (!check_passed)
		return false;
	return true;
}
/* End of check action function: Put */

/* Definition of interface info struct: Get */
typedef struct Get_info {
TypeV * __RET__;
TypeK * key;
} Get_info;
/* End of info struct definition: Get */

/* ID function of interface: Get */
inline static call_id_t Get_id(void *info, thread_id_t __TID__) {
	Get_info* theInfo = (Get_info*)info;
	TypeV * __RET__ = theInfo->__RET__;
	TypeK * key = theInfo->key;

	call_id_t __ID__ = getKeyTag ( key );
	return __ID__;
}
/* End of ID function: Get */

/* Check action function of interface: Get */
inline static bool Get_check_action(void *info, call_id_t __ID__, thread_id_t __TID__) {
	bool check_passed;
	Get_info* theInfo = (Get_info*)info;
	TypeV * __RET__ = theInfo->__RET__;
	TypeK * key = theInfo->key;

	TypeV * _Old_Val = ( TypeV * ) spec_table_get ( map , key ) ;
	bool passed = false ;
	if ( ! passed ) {
	int old = _Old_Val == NULL ? 0 : _Old_Val -> _val ;
	int ret = __RET__ == NULL ? 0 : __RET__ -> _val ;
	}
	check_passed = equals_val ( _Old_Val , __RET__ );
	if (!check_passed)
		return false;
	return true;
}
/* End of check action function: Get */

#define INTERFACE_SIZE 2
static void** func_ptr_table;
static anno_hb_init** hb_init_table;

/* Define function for sequential code initialization */
inline static void __sequential_init() {
	/* Init func_ptr_table */
	func_ptr_table = (void**) malloc(sizeof(void*) * 2 * 2);
	func_ptr_table[2 * 1] = (void*) &Put_id;
	func_ptr_table[2 * 1 + 1] = (void*) &Put_check_action;
	func_ptr_table[2 * 0] = (void*) &Get_id;
	func_ptr_table[2 * 0 + 1] = (void*) &Get_check_action;
	/* Put(true) -> Put(true) */
	struct anno_hb_init *hbConditionInit0 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit0->interface_num_before = 1; // Put
	hbConditionInit0->hb_condition_num_before = 0; // 
	hbConditionInit0->interface_num_after = 1; // Put
	hbConditionInit0->hb_condition_num_after = 0; // 
	/* Put(true) -> Get(true) */
	struct anno_hb_init *hbConditionInit1 = (struct anno_hb_init*) malloc(sizeof(struct anno_hb_init));
	hbConditionInit1->interface_num_before = 1; // Put
	hbConditionInit1->hb_condition_num_before = 0; // 
	hbConditionInit1->interface_num_after = 0; // Get
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

	map = new_spec_table_default ( equals_key ) ;
	id_map = new_spec_table_default ( equals_id ) ;
	tag = new_id_tag ( ) ;
}
/* End of Global construct generation in class */
	

friend class CHM;
	
	private:
	class CHM {
		friend class cliffc_hashtable;
		private:
		atomic<kvs_data*> _newkvs;
		
				atomic_int _size;
	
				atomic_int _slots;
		
				atomic_int _copy_idx;
		
				atomic_int _copy_done;
	
		public:
		CHM(int size) {
			_newkvs.store(NULL, memory_order_relaxed);
			_size.store(size, memory_order_relaxed);
			_slots.store(0, memory_order_relaxed);
	
			_copy_idx.store(0, memory_order_relaxed);
			_copy_done.store(0, memory_order_relaxed);
		}
	
		~CHM() {}
		
		private:
			
				bool table_full(int reprobe_cnt, int len) {
			return
				reprobe_cnt >= REPROBE_LIMIT &&
				_slots.load(memory_order_relaxed) >= reprobe_limit(len);
		}
	
		kvs_data* resize(cliffc_hashtable *topmap, kvs_data *kvs) {
			model_print("hey\n");
						
			kvs_data *newkvs = _newkvs.load(memory_order_acquire);
			if (newkvs != NULL)
				return newkvs;
	
						int oldlen = kvs->_size;
			int sz = _size.load(memory_order_relaxed);
			int newsz = sz;
			
						if (sz >= (oldlen >> 2)) { 				newsz = oldlen << 1; 				if (sz >= (oldlen >> 1))
					newsz = oldlen << 2; 			}
	
						if (newsz <= oldlen) newsz = oldlen << 1;
						if (newsz < oldlen) newsz = oldlen;
	
						
			newkvs = _newkvs.load(memory_order_acquire);
						if (newkvs != NULL) return newkvs;
	
			newkvs = new kvs_data(newsz);
			void *chm = (void*) new CHM(sz);
						newkvs->_data[0].store(chm, memory_order_relaxed);
	
			kvs_data *cur_newkvs; 
						
			if ((cur_newkvs = _newkvs.load(memory_order_acquire)) != NULL)
				return cur_newkvs;
						kvs_data *desired = (kvs_data*) NULL;
			kvs_data *expected = (kvs_data*) newkvs; 
			
						if (!_newkvs.compare_exchange_strong(desired, expected, memory_order_release,
					memory_order_relaxed)) {
								delete newkvs;
				
				newkvs = _newkvs.load(memory_order_acquire);
			}
			return newkvs;
		}
	
		void help_copy_impl(cliffc_hashtable *topmap, kvs_data *oldkvs,
			bool copy_all) {
			MODEL_ASSERT (get_chm(oldkvs) == this);
			
			kvs_data *newkvs = _newkvs.load(memory_order_acquire);
			int oldlen = oldkvs->_size;
			int min_copy_work = oldlen > 1024 ? 1024 : oldlen;
		
						int panic_start = -1;
			int copyidx;
			while (_copy_done.load(memory_order_relaxed) < oldlen) {
				copyidx = _copy_idx.load(memory_order_relaxed);
				if (panic_start == -1) { 					copyidx = _copy_idx.load(memory_order_relaxed);
					while (copyidx < (oldlen << 1) &&
						!_copy_idx.compare_exchange_strong(copyidx, copyidx +
							min_copy_work, memory_order_relaxed, memory_order_relaxed))
						copyidx = _copy_idx.load(memory_order_relaxed);
					if (!(copyidx < (oldlen << 1)))
						panic_start = copyidx;
				}
	
								int workdone = 0;
				for (int i = 0; i < min_copy_work; i++)
					if (copy_slot(topmap, (copyidx + i) & (oldlen - 1), oldkvs,
						newkvs))
						workdone++;
				if (workdone > 0)
					copy_check_and_promote(topmap, oldkvs, workdone);
	
				copyidx += min_copy_work;
				if (!copy_all && panic_start == -1)
					return; 			}
			copy_check_and_promote(topmap, oldkvs, 0); 		}
	
		kvs_data* copy_slot_and_check(cliffc_hashtable *topmap, kvs_data
			*oldkvs, int idx, void *should_help) {
			
			kvs_data *newkvs = _newkvs.load(memory_order_acquire);
						if (copy_slot(topmap, idx, oldkvs, newkvs))
				copy_check_and_promote(topmap, oldkvs, 1); 			return (should_help == NULL) ? newkvs : topmap->help_copy(newkvs);
		}
	
		void copy_check_and_promote(cliffc_hashtable *topmap, kvs_data*
			oldkvs, int workdone) {
			int oldlen = oldkvs->_size;
			int copyDone = _copy_done.load(memory_order_relaxed);
			if (workdone > 0) {
				while (true) {
					copyDone = _copy_done.load(memory_order_relaxed);
					if (_copy_done.compare_exchange_weak(copyDone, copyDone +
						workdone, memory_order_relaxed, memory_order_relaxed))
						break;
				}
			}
	
						if (copyDone + workdone == oldlen &&
				topmap->_kvs.load(memory_order_relaxed) == oldkvs) {
				
				kvs_data *newkvs = _newkvs.load(memory_order_acquire);
				
				topmap->_kvs.compare_exchange_strong(oldkvs, newkvs, memory_order_release,
					memory_order_relaxed);
			}
		}
	
		bool copy_slot(cliffc_hashtable *topmap, int idx, kvs_data *oldkvs,
			kvs_data *newkvs) {
			slot *key_slot;
			while ((key_slot = key(oldkvs, idx)) == NULL)
				CAS_key(oldkvs, idx, NULL, TOMBSTONE);
	
						slot *oldval = val(oldkvs, idx);
			while (!is_prime(oldval)) {
				slot *box = (oldval == NULL || oldval == TOMBSTONE)
					? TOMBPRIME : new slot(true, oldval->_ptr);
				if (CAS_val(oldkvs, idx, oldval, box)) {
					if (box == TOMBPRIME)
						return 1; 										oldval = box; 					break;
				}
				oldval = val(oldkvs, idx); 			}
	
			if (oldval == TOMBPRIME) return false; 	
			slot *old_unboxed = new slot(false, oldval->_ptr);
			int copied_into_new = (putIfMatch(topmap, newkvs, key_slot, old_unboxed,
				NULL) == NULL);
	
						while (!CAS_val(oldkvs, idx, oldval, TOMBPRIME))
				oldval = val(oldkvs, idx);
	
			return copied_into_new;
		}
	};

	

	private:
	static const int Default_Init_Size = 4; 
	static slot* const MATCH_ANY;
	static slot* const NO_MATCH_OLD;

	static slot* const TOMBPRIME;
	static slot* const TOMBSTONE;

	static const int REPROBE_LIMIT = 10; 
	atomic<kvs_data*> _kvs;

	public:
	cliffc_hashtable() {
	__sequential_init();
								
		kvs_data *kvs = new kvs_data(Default_Init_Size);
		void *chm = (void*) new CHM(0);
		kvs->_data[0].store(chm, memory_order_relaxed);
		_kvs.store(kvs, memory_order_relaxed);
	}

	cliffc_hashtable(int init_size) {
						
	__sequential_init();
		
		kvs_data *kvs = new kvs_data(init_size);
		void *chm = (void*) new CHM(0);
		kvs->_data[0].store(chm, memory_order_relaxed);
		_kvs.store(kvs, memory_order_relaxed);
	}


TypeV * get(TypeK * key) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 0; // Get
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	TypeV * __RET__ = __wrapper__get(key);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 0; // Get
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Get_info* info = (Get_info*) malloc(sizeof(Get_info));
	info->__RET__ = __RET__;
	info->key = key;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 0; // Get
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
	return __RET__;
}
	
TypeV * __wrapper__get(TypeK * key) {
		slot *key_slot = new slot(false, key);
		int fullhash = hash(key_slot);
		
		kvs_data *kvs = _kvs.load(memory_order_acquire);
		
		slot *V = get_impl(this, kvs, key_slot, fullhash);
		if (V == NULL) return NULL;
		MODEL_ASSERT (!is_prime(V));
		return (TypeV*) V->_ptr;
	}


TypeV * put(TypeK * key, TypeV * val) {
	/* Interface begins */
	struct anno_interface_begin *interface_begin = (struct anno_interface_begin*) malloc(sizeof(struct anno_interface_begin));
	interface_begin->interface_num = 1; // Put
	struct spec_annotation *annotation_interface_begin = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_interface_begin->type = INTERFACE_BEGIN;
	annotation_interface_begin->annotation = interface_begin;
	cdsannotate(SPEC_ANALYSIS, annotation_interface_begin);
	TypeV * __RET__ = __wrapper__put(key, val);
	struct anno_hb_condition *hb_condition = (struct anno_hb_condition*) malloc(sizeof(struct anno_hb_condition));
	hb_condition->interface_num = 1; // Put
	hb_condition->hb_condition_num = 0;
	struct spec_annotation *annotation_hb_condition = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annotation_hb_condition->type = HB_CONDITION;
	annotation_hb_condition->annotation = hb_condition;
	cdsannotate(SPEC_ANALYSIS, annotation_hb_condition);

	Put_info* info = (Put_info*) malloc(sizeof(Put_info));
	info->__RET__ = __RET__;
	info->key = key;
	info->val = val;
	struct anno_interface_end *interface_end = (struct anno_interface_end*) malloc(sizeof(struct anno_interface_end));
	interface_end->interface_num = 1; // Put
	interface_end->info = info;
	struct spec_annotation *annoation_interface_end = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
	annoation_interface_end->type = INTERFACE_END;
	annoation_interface_end->annotation = interface_end;
	cdsannotate(SPEC_ANALYSIS, annoation_interface_end);
	return __RET__;
}
	
TypeV * __wrapper__put(TypeK * key, TypeV * val) {
		return putIfMatch(key, val, NO_MATCH_OLD);
	}

	
	TypeV* putIfAbsent(TypeK *key, TypeV *value) {
		return putIfMatch(key, val, TOMBSTONE);
	}

	
	TypeV* remove(TypeK *key) {
		return putIfMatch(key, TOMBSTONE, NO_MATCH_OLD);
	}

	
	bool remove(TypeK *key, TypeV *val) {
		slot *val_slot = val == NULL ? NULL : new slot(false, val);
		return putIfMatch(key, TOMBSTONE, val) == val;

	}

	
	TypeV* replace(TypeK *key, TypeV *val) {
		return putIfMatch(key, val, MATCH_ANY);
	}

	
	bool replace(TypeK *key, TypeV *oldval, TypeV *newval) {
		return putIfMatch(key, newval, oldval) == oldval;
	}

	private:
	static CHM* get_chm(kvs_data* kvs) {
		CHM *res = (CHM*) kvs->_data[0].load(memory_order_relaxed);
		return res;
	}

	static int* get_hashes(kvs_data *kvs) {
		return (int *) kvs->_data[1].load(memory_order_relaxed);
	}
	
		static inline slot* key(kvs_data *kvs, int idx) {
		MODEL_ASSERT (idx >= 0 && idx < kvs->_size);
						slot *res = (slot*) kvs->_data[idx * 2 + 2].load(memory_order_relaxed);
	/* Automatically generated code for potential commit point: Read_Key_Point */

	if (true) {
		struct anno_potential_cp_define *potential_cp_define = (struct anno_potential_cp_define*) malloc(sizeof(struct anno_potential_cp_define));
		potential_cp_define->label_num = 0;
		struct spec_annotation *annotation_potential_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_potential_cp_define->type = POTENTIAL_CP_DEFINE;
		annotation_potential_cp_define->annotation = potential_cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_potential_cp_define);
	}
		
		return res;
	}

	
		static inline slot* val(kvs_data *kvs, int idx) {
		MODEL_ASSERT (idx >= 0 && idx < kvs->_size);
						
		slot *res = (slot*) kvs->_data[idx * 2 + 3].load(memory_order_acquire);
	/* Automatically generated code for potential commit point: Read_Val_Point */

	if (true) {
		struct anno_potential_cp_define *potential_cp_define = (struct anno_potential_cp_define*) malloc(sizeof(struct anno_potential_cp_define));
		potential_cp_define->label_num = 1;
		struct spec_annotation *annotation_potential_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_potential_cp_define->type = POTENTIAL_CP_DEFINE;
		annotation_potential_cp_define->annotation = potential_cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_potential_cp_define);
	}
		
		return res;


	}

	static int hash(slot *key_slot) {
		MODEL_ASSERT(key_slot != NULL && key_slot->_ptr != NULL);
		TypeK* key = (TypeK*) key_slot->_ptr;
		int h = key->hashCode();
				h += (h << 15) ^ 0xffffcd7d;
		h ^= (h >> 10);
		h += (h << 3);
		h ^= (h >> 6);
		h += (h << 2) + (h << 14);
		return h ^ (h >> 16);
	}
	
				static int reprobe_limit(int len) {
		return REPROBE_LIMIT + (len >> 2);
	}
	
	static inline bool is_prime(slot *val) {
		return (val != NULL) && val->_prime;
	}

			static bool keyeq(slot *K, slot *key_slot, int *hashes, int hash,
		int fullhash) {
				MODEL_ASSERT (K != NULL);
		TypeK* key_ptr = (TypeK*) key_slot->_ptr;
		return
			K == key_slot ||
				((hashes[hash] == 0 || hashes[hash] == fullhash) &&
				K != TOMBSTONE &&
				key_ptr->equals(K->_ptr));
	}

	static bool valeq(slot *val_slot1, slot *val_slot2) {
		MODEL_ASSERT (val_slot1 != NULL);
		TypeK* ptr1 = (TypeV*) val_slot1->_ptr;
		if (val_slot2 == NULL || ptr1 == NULL) return false;
		return ptr1->equals(val_slot2->_ptr);
	}
	
			static inline bool CAS_key(kvs_data *kvs, int idx, void *expected, void *desired) {
		bool res = kvs->_data[2 * idx + 2].compare_exchange_strong(expected,
			desired, memory_order_relaxed, memory_order_relaxed);
	/* Automatically generated code for potential commit point: Write_Key_Point */

	if (res) {
		struct anno_potential_cp_define *potential_cp_define = (struct anno_potential_cp_define*) malloc(sizeof(struct anno_potential_cp_define));
		potential_cp_define->label_num = 2;
		struct spec_annotation *annotation_potential_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_potential_cp_define->type = POTENTIAL_CP_DEFINE;
		annotation_potential_cp_define->annotation = potential_cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_potential_cp_define);
	}
		
		return res;
	}

	
			static inline bool CAS_val(kvs_data *kvs, int idx, void *expected, void
		*desired) {
		
		bool res =  kvs->_data[2 * idx + 3].compare_exchange_strong(expected,
			desired, memory_order_acq_rel, memory_order_relaxed);
	/* Automatically generated code for potential commit point: Write_Val_Point */

	if (res) {
		struct anno_potential_cp_define *potential_cp_define = (struct anno_potential_cp_define*) malloc(sizeof(struct anno_potential_cp_define));
		potential_cp_define->label_num = 3;
		struct spec_annotation *annotation_potential_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_potential_cp_define->type = POTENTIAL_CP_DEFINE;
		annotation_potential_cp_define->annotation = potential_cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_potential_cp_define);
	}
		
		return res;
	}

	slot* get_impl(cliffc_hashtable *topmap, kvs_data *kvs, slot* key_slot, int
		fullhash) {
		int len = kvs->_size;
		CHM *chm = get_chm(kvs);
		int *hashes = get_hashes(kvs);

		int idx = fullhash & (len - 1);
		int reprobe_cnt = 0;
		while (true) {
			slot *K = key(kvs, idx);
	/* Automatically generated code for commit point define: Get_Point1 */

	if (K == NULL) {
		struct anno_cp_define *cp_define = (struct anno_cp_define*) malloc(sizeof(struct anno_cp_define));
		cp_define->label_num = 4;
		cp_define->potential_cp_label_num = 0;
		cp_define->interface_num = 0;
		struct spec_annotation *annotation_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define->type = CP_DEFINE;
		annotation_cp_define->annotation = cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define);
	}
			
			slot *V = val(kvs, idx);
			
			if (K == NULL) {
								return NULL; 			}
			
			if (keyeq(K, key_slot, hashes, idx, fullhash)) {
								if (!is_prime(V)) {
	/* Automatically generated code for commit point clear: Get_Clear */

	if (true) {
		struct anno_cp_clear *cp_clear = (struct anno_cp_clear*) malloc(sizeof(struct anno_cp_clear));
		cp_clear->label_num = 5;
		cp_clear->interface_num = 0;
		struct spec_annotation *annotation_cp_clear = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_clear->type = CP_CLEAR;
		annotation_cp_clear->annotation = cp_clear;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_clear);
	}
					

	/* Automatically generated code for commit point define: Get_Point2 */

	if (true) {
		struct anno_cp_define *cp_define = (struct anno_cp_define*) malloc(sizeof(struct anno_cp_define));
		cp_define->label_num = 6;
		cp_define->potential_cp_label_num = 1;
		cp_define->interface_num = 0;
		struct spec_annotation *annotation_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define->type = CP_DEFINE;
		annotation_cp_define->annotation = cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define);
	}
					
					return (V == TOMBSTONE) ? NULL : V; 				}
								return get_impl(topmap, chm->copy_slot_and_check(topmap, kvs,
					idx, key_slot), key_slot, fullhash);
			}

			if (++reprobe_cnt >= REPROBE_LIMIT ||
				key_slot == TOMBSTONE) {
												
				kvs_data *newkvs = chm->_newkvs.load(memory_order_acquire);
				
				return newkvs == NULL ? NULL : get_impl(topmap,
					topmap->help_copy(newkvs), key_slot, fullhash);
			}

			idx = (idx + 1) & (len - 1); 		}
	}

		TypeV* putIfMatch(TypeK *key, TypeV *value, slot *old_val) {
				if (old_val == NULL) {
			return NULL;
		}
		slot *key_slot = new slot(false, key);

		slot *value_slot = new slot(false, value);
		
		kvs_data *kvs = _kvs.load(memory_order_acquire);
		
		slot *res = putIfMatch(this, kvs, key_slot, value_slot, old_val);
				MODEL_ASSERT (res != NULL); 
		MODEL_ASSERT (!is_prime(res));
		return res == TOMBSTONE ? NULL : (TypeV*) res->_ptr;
	}

	
	static slot* putIfMatch(cliffc_hashtable *topmap, kvs_data *kvs, slot
		*key_slot, slot *val_slot, slot *expVal) {
		MODEL_ASSERT (val_slot != NULL);
		MODEL_ASSERT (!is_prime(val_slot));
		MODEL_ASSERT (!is_prime(expVal));

		int fullhash = hash(key_slot);
		int len = kvs->_size;
		CHM *chm = get_chm(kvs);
		int *hashes = get_hashes(kvs);
		int idx = fullhash & (len - 1);

				int reprobe_cnt = 0;
		slot *K;
		slot *V;
		kvs_data *newkvs;
		
		while (true) { 			K = key(kvs, idx);
			V = val(kvs, idx);
			if (K == NULL) { 				if (val_slot == TOMBSTONE) return val_slot;
								if (CAS_key(kvs, idx, NULL, key_slot)) {
	/* Automatically generated code for commit point define: Put_WriteKey */

	if (true) {
		struct anno_cp_define *cp_define = (struct anno_cp_define*) malloc(sizeof(struct anno_cp_define));
		cp_define->label_num = 7;
		cp_define->potential_cp_label_num = 2;
		cp_define->interface_num = 1;
		struct spec_annotation *annotation_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define->type = CP_DEFINE;
		annotation_cp_define->annotation = cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define);
	}
					
					chm->_slots.fetch_add(1, memory_order_relaxed); 					hashes[idx] = fullhash; 					break;
				}
				K = key(kvs, idx); 				MODEL_ASSERT (K != NULL);
			}

						if (keyeq(K, key_slot, hashes, idx, fullhash))
				break; 			
												if (++reprobe_cnt >= reprobe_limit(len) ||
				K == TOMBSTONE) { 				newkvs = chm->resize(topmap, kvs);
												if (expVal != NULL) topmap->help_copy(newkvs);
				return putIfMatch(topmap, newkvs, key_slot, val_slot, expVal);
			}

			idx = (idx + 1) & (len - 1); 		} 
		if (val_slot == V) return V; 	
						
		newkvs = chm->_newkvs.load(memory_order_acquire);
		
		if (newkvs == NULL &&
			((V == NULL && chm->table_full(reprobe_cnt, len)) || is_prime(V))) {
						newkvs = chm->resize(topmap, kvs); 		}
		
				if (newkvs != NULL)
			return putIfMatch(topmap, chm->copy_slot_and_check(topmap, kvs, idx,
				expVal), key_slot, val_slot, expVal);
		
				while (true) {
			MODEL_ASSERT (!is_prime(V));

			if (expVal != NO_MATCH_OLD &&
				V != expVal &&
				(expVal != MATCH_ANY || V == TOMBSTONE || V == NULL) &&
				!(V == NULL && expVal == TOMBSTONE) &&
				(expVal == NULL || !valeq(expVal, V))) {
				
				
				
				return V; 			}

			if (CAS_val(kvs, idx, V, val_slot)) {
	/* Automatically generated code for commit point define: Put_Point */

	if (true) {
		struct anno_cp_define *cp_define = (struct anno_cp_define*) malloc(sizeof(struct anno_cp_define));
		cp_define->label_num = 8;
		cp_define->potential_cp_label_num = 3;
		cp_define->interface_num = 1;
		struct spec_annotation *annotation_cp_define = (struct spec_annotation*) malloc(sizeof(struct spec_annotation));
		annotation_cp_define->type = CP_DEFINE;
		annotation_cp_define->annotation = cp_define;
		cdsannotate(SPEC_ANALYSIS, annotation_cp_define);
	}
				
				if (expVal != NULL) { 																				if ((V == NULL || V == TOMBSTONE) &&
						val_slot != TOMBSTONE)
						chm->_size.fetch_add(1, memory_order_relaxed);
					if (!(V == NULL || V == TOMBSTONE) &&
						val_slot == TOMBSTONE)
						chm->_size.fetch_add(-1, memory_order_relaxed);
				}
				return (V == NULL && expVal != NULL) ? TOMBSTONE : V;
			}
						V = val(kvs, idx);
			if (is_prime(V))
				return putIfMatch(topmap, chm->copy_slot_and_check(topmap, kvs,
					idx, expVal), key_slot, val_slot, expVal);
		}
	}

		kvs_data* help_copy(kvs_data *helper) {
		
		kvs_data *topkvs = _kvs.load(memory_order_acquire);
		CHM *topchm = get_chm(topkvs);
				if (topchm->_newkvs.load(memory_order_relaxed) == NULL) return helper;
		topchm->help_copy_impl(this, topkvs, false);
		return helper;
	}
};
template<typename TypeK, typename TypeV>
void** cliffc_hashtable<TypeK, TypeV>::func_ptr_table;
template<typename TypeK, typename TypeV>
anno_hb_init** cliffc_hashtable<TypeK, TypeV>::hb_init_table;
template<typename TypeK, typename TypeV>
spec_table * cliffc_hashtable<TypeK, TypeV>::map;
template<typename TypeK, typename TypeV>
spec_table * cliffc_hashtable<TypeK, TypeV>::id_map;
template<typename TypeK, typename TypeV>
id_tag_t * cliffc_hashtable<TypeK, TypeV>::tag;


#endif

