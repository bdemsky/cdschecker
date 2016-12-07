#include <stdio.h>
#include <stdatomic.h>

#include "librace.h"
#include "cdsspec-generated.h"

atomic_int x;

int Load(atomic_int *loc);

void Store(atomic_int *loc, int val);
