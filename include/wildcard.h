#ifndef _WILDCARD_H
#define _WILDCARD_H
#include "memory_order.h"

#define memory_order_wildcard(x) ((memory_order) (0x1000+x))

#endif
