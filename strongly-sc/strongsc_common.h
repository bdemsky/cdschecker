#ifndef _STRONGSC_COMMON_H
#define _STRONGSC_COMMON_H

#include "action.h"
#include "mydebug.h"
#include "common.h"

inline const char * get_mo_str(memory_order order) {
    switch (order) {
        case std::memory_order_relaxed: return "relaxed";
        case std::memory_order_acquire: return "acquire";
        case std::memory_order_release: return "release";
        case std::memory_order_acq_rel: return "acq_rel";
        case std::memory_order_seq_cst: return "seq_cst";
        default: return "unknown";
    }
}

#endif
