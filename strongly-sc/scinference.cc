#include "scinference.h"
#include "action.h"
#include "threads-model.h"
#include "strongsc_common.h"
#include <sys/time.h>


/**********    SCInference    **********/
SCInference::SCInference() {
    wildcardMap = new WMap;
    wildcardList = new ModelList<int>;
}

SCInference::~SCInference() {
    delete wildcardMap;
    delete wildcardList;
}

void SCInference::print() {
    wildcardList->sort();
    model_print("wildcardList size = %lu\n", wildcardList->size());
    for (SnapList<int>::iterator i = wildcardList->begin(); i !=
        wildcardList->end(); i++) {
        int wildcard = *i;
        model_print("wildcard (%d) --> %s\n", wildcard,
            get_mo_str(getWildcardMO(wildcard)));
    }
}

void SCInference::addWildcard(int wildcard, memory_order mo) {
    if (!wildcardMap->contains(wildcard))
        wildcardMap->put(wildcard, toMap(memory_order_relaxed));
}

void SCInference::imposeAcquire(int wildcard) {
    if (!wildcardMap->contains(wildcard)) {
        DPRINT("wildcard(%d): %s --> %s\n", wildcard, "relaxed", "acquire");
        wildcardList->push_back(wildcard);
        wildcardMap->put(wildcard, toMap(memory_order_acquire));
    } else {
        memory_order curMO = fromMap(wildcardMap->get(wildcard));
        if (curMO == memory_order_relaxed) {
            DPRINT("wildcard(%d): %s --> %s\n", wildcard, "relaxed", "acquire");
            wildcardMap->put(wildcard, toMap(memory_order_acquire));
        } else if (curMO == memory_order_release) {
            DPRINT("wildcard(%d): %s --> %s\n", wildcard, "release", "acq_rel");
            wildcardMap->put(wildcard, toMap(memory_order_acq_rel));
        }
    }
}


bool SCInference::imposeAcquire(const ModelAction *act) {
    memory_order mo = act->get_original_mo();
    if (is_wildcard(mo)) {
        int wildcard = get_wildcard_id(mo);
        imposeAcquire(wildcard);
        return true;
    } else {
        if (act->is_acquire())
            return true;
        else {
            model_print("**** The following action has to be acquire ****\n");
            act->print();
            return false;
        }
    }
}

void SCInference::imposeRelease(int wildcard) {
    if (!wildcardMap->contains(wildcard)) {
        DPRINT("wildcard(%d): %s --> %s\n", wildcard, "relaxed", "release");
        wildcardList->push_back(wildcard);
        wildcardMap->put(wildcard, toMap(memory_order_release));
    } else {
        memory_order curMO = fromMap(wildcardMap->get(wildcard));
        if (curMO == memory_order_relaxed) {
            DPRINT("wildcard(%d): %s --> %s\n", wildcard, "relaxed", "release");
            wildcardMap->put(wildcard, toMap(memory_order_release));
        } else if (curMO == memory_order_acquire) {
            DPRINT("wildcard(%d): %s --> %s\n", wildcard, "acquire", "acq_rel");
            wildcardMap->put(wildcard, toMap(memory_order_acq_rel));
        }
    }
}

bool SCInference::imposeRelease(const ModelAction *act) {
    memory_order mo = act->get_original_mo();
    if (is_wildcard(mo)) {
        int wildcard = get_wildcard_id(mo);
        imposeRelease(wildcard);
        return true;
    } else {
        if (act->is_release())
            return true;
        else {
            model_print("**** The following action has to be release ****\n");
            act->print();
            return false;
        }
    }
}

void SCInference::imposeSC(int wildcard) {
    if (!wildcardMap->contains(wildcard)) {
        DPRINT("wildcard(%d): %s --> %s\n", wildcard, "relaxed", "seq_cst");
        wildcardList->push_back(wildcard);
        wildcardMap->put(wildcard, toMap(memory_order_seq_cst));
    } else {
        memory_order curMO = fromMap(wildcardMap->get(wildcard));
        if (curMO == memory_order_relaxed) {
            DPRINT("wildcard(%d): %s --> %s\n", wildcard, "relaxed", "seq_cst");
            wildcardMap->put(wildcard, toMap(memory_order_seq_cst));
        } else if (curMO == memory_order_acquire) {
            DPRINT("wildcard(%d): %s --> %s\n", wildcard, "acquire", "seq_cst");
            wildcardMap->put(wildcard, toMap(memory_order_seq_cst));
        } else if (curMO == memory_order_release) {
            DPRINT("wildcard(%d): %s --> %s\n", wildcard, "release", "seq_cst");
            wildcardMap->put(wildcard, toMap(memory_order_seq_cst));
        } else if (curMO == memory_order_acq_rel) {
            DPRINT("wildcard(%d): %s --> %s\n", wildcard, "acq_rel", "seq_cst");
            wildcardMap->put(wildcard, toMap(memory_order_seq_cst));
        }
    }

}

bool SCInference::imposeSC(const ModelAction *act) {
    memory_order mo = act->get_original_mo();
    if (is_wildcard(mo)) {
        int wildcard = get_wildcard_id(mo);
        imposeSC(wildcard);
        return true;
    } else {
        if (act->is_seqcst())
            return true;
        else {
            model_print("**** The following action has to be seq_cst ****\n");
            act->print();
            return false;
        }
    }
}

memory_order SCInference::getWildcardMO(int wildcard) {
    if (wildcardMap->contains(wildcard)) {
        return fromMap(wildcardMap->get(wildcard));
    } else {
        return memory_order_relaxed;
    }
}

memory_order SCInference::getWildcardMO(const ModelAction *act) {
    memory_order mo = act->get_original_mo();
    if (is_wildcard(mo)) {
        int wildcard = get_wildcard_id(mo);
        return getWildcardMO(wildcard);
    } else {
        // If the wildcard has never been strengthened, it should be relaxed
        return memory_order_relaxed;
    }   
}

bool SCInference::is_seqcst(const ModelAction *act) {
    memory_order wildcardMO = act->get_original_mo();
    if (is_wildcard(wildcardMO)) {
        int wildcard = get_wildcard_id(wildcardMO);
        ASSERT (wildcard > 0);

        if (!wildcardMap->contains(wildcard))
            return false;
        memory_order curMO = fromMap(wildcardMap->get(wildcard));
        return curMO == memory_order_seq_cst;
    } else {
        return act->is_seqcst();
    }
}

bool SCInference::is_release(const ModelAction *act) {
    memory_order wildcardMO = act->get_original_mo();
    if (is_wildcard(wildcardMO)) {
        int wildcard = get_wildcard_id(wildcardMO);
        ASSERT (wildcard > 0);

        if (!wildcardMap->contains(wildcard))
            return false;
        memory_order curMO = fromMap(wildcardMap->get(wildcard));
        return curMO == memory_order_release || curMO == memory_order_acq_rel ||
            curMO == memory_order_seq_cst;
    } else {
        return act->is_release();
    }
}

bool SCInference::is_acquire(const ModelAction *act) {
    memory_order wildcardMO = act->get_original_mo();
    if (is_wildcard(wildcardMO)) {
        int wildcard = get_wildcard_id(wildcardMO);
        ASSERT (wildcard > 0);

        if (!wildcardMap->contains(wildcard))
            return false;
        memory_order curMO = fromMap(wildcardMap->get(wildcard));
        return curMO == memory_order_acquire || curMO == memory_order_acq_rel ||
            curMO == memory_order_seq_cst;
    } else {
        return act->is_acquire();
    }
}


memory_order SCInference::toMap(memory_order mo) {
    int tmp = (int) mo;
    tmp++;
    return (memory_order) tmp;
}

memory_order SCInference::fromMap(memory_order mo) {
    int tmp = (int) mo;
    tmp--;
    return (memory_order) tmp;
}
