#ifndef _WILDCARD_H
#define _WILDCARD_H
#include "memoryorder.h"

#define MAX_WILDCARD_NUM 100

#define memory_order_wildcard(x) ((memory_order) (0x1000+x))

#define wildcard(x) ((memory_order) (0x1000+x))
#define get_wildcard_id(x) ((int) (x-0x1000))
#define get_wildcard_id_zero(x) ((get_wildcard_id(x)) > 0 ? get_wildcard_id(x) : 0)

#define INIT_WILDCARD_NUM 20
#define WILDCARD_NONEXIST (memory_order) -1
#define INFERENCE_INCOMPARABLE(x) (!(-1 <= (x) <= 1))

#define is_wildcard(x) (!(x >= memory_order_relaxed && x <= memory_order_seq_cst))
#define is_normal_mo_infer(x) ((x >= memory_order_relaxed && x <= memory_order_seq_cst) || x == WILDCARD_NONEXIST)
#define is_normal_mo(x) (x >= memory_order_relaxed && x <= memory_order_seq_cst)

#define assert_infer(x) for (int i = 0; i <= wildcardNum; i++)\
	ASSERT(is_normal_mo_infer((x[i])));

#define assert_infers(x) for (ModelList<memory_order *>::iterator iter =\
	(x)->begin(); iter != (x)->end(); iter++)\
	assert_infer((*iter));

#define relaxed memory_order_relaxed
#define release memory_order_release
#define acquire memory_order_acquire
#define seqcst memory_order_seq_cst
#define acqrel memory_order_acq_rel

#define LOAD atomic_load_explicit
#define STORE atomic_store_explicit
#define CAS atomic_compare_exchange_strong_explicit

#endif
