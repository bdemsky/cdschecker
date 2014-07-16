#ifndef _WILDCARD_H
#define _WILDCARD_H
#include "memoryorder.h"

#define memory_order_wildcard(x) ((memory_order) (0x1000+x))

#define wildcard(x) ((memory_order) (0x1000+x))

#define is_wildcard(x) (!(x >= memory_order_relaxed && x <= memory_order_seq_cst))

#define get_wildcard_id(x) ((int) (x-0x1000))

#define relaxed memory_order_relaxed
#define release memory_order_release
#define acquire memory_order_acquire
#define seqcst memory_order_seq_cst
#define acqrel memory_order_acq_rel

#endif
