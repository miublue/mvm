#ifndef MVALUE_H
#define MVALUE_H

#include <inttypes.h>

#define bool char
#define true 1
#define false 0

// TODO: maybe have a big array with data on custom types
// and if value type > NUM_BASE_TYPES, then it's a custom type
// get its information by simply doing types_info[type - NUM_BASE_TYPES]
typedef uint16_t value_type_e;
enum {
    TYPE_NULL,
    TYPE_INTEGER,
    TYPE_FLOAT,

    NUM_BASE_TYPES,
};

typedef struct value_t {
    value_type_e type;
    bool is_mut;
    bool is_ptr;

    union {
        void *as_ptr;
        int64_t as_int;
        float as_float;
    };
} value_t;

#endif
