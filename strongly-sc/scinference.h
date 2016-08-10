#ifndef _SCINFERENCE_H 
#define _SCINFERENCE_H 
#include "wildcard.h"
#include "hashtable.h"
#include "action.h"
#include "stl-model.h"

typedef HashTable<int, memory_order, uintptr_t, 4, model_malloc, model_calloc,
model_free> WMap;

struct SCInference {
    WMap *wildcardMap;
    ModelList<int> *wildcardList;

    SCInference();
    ~SCInference();
    void print();
    void addWildcard(int wildcard, memory_order mo);
    void imposeAcquire(int wildcard);
    bool imposeAcquire(const ModelAction *act);
    void imposeRelease(int wildcard);
    bool imposeRelease(const ModelAction *act);
    void imposeSC(int wildcard);
    bool imposeSC(const ModelAction *act);

    memory_order getWildcardMO(const ModelAction *act);
    memory_order getWildcardMO(int wildcard);

    bool is_seqcst(const ModelAction *act);
    bool is_release(const ModelAction *act);
    bool is_acquire(const ModelAction *act);

    // These two are just to get around the fact that the HashTable won't allow
    // NULL or '0' values
    memory_order toMap(memory_order mo);
    memory_order fromMap(memory_order mo);

    MEMALLOC
};

#endif
