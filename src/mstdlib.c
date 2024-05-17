#include <stdio.h>

#include "mvalue.h"
#include "mvm.h"
#include "mstdlib.h"

void
lib_print(vm_t *vm)
{
    value_t v = vm_pop(vm);

    switch (v.type) {
    default:
        printf("%ld\n", v.as_int);
        break;
    }
}
