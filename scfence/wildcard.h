#ifndef _WILDCARD_H
#define _WILDCARD_H
#include "memoryorder.h"

#define memory_order_wildcard(x) ((memory_order) (0x1000+x))

#define wildcard(x) ((memory_order) (0x1000+x))

#define is_wildcard(x) (!(x >= memory_order_relaxed && x <= memory_order_seq_cst))

#define get_wildcard_id(x) ((int) (x-0x1000))

#endif
