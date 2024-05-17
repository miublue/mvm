#ifndef MVM_H
#define MVM_H

#include <stdlib.h>
#include "mlist.h"
#include "mvalue.h"

#define MVM_HEADER ".MVM"
#define MVM_INST_SIZE 12

typedef char inst_type_e;
enum {
    // Instructions that take no arguments
    INST_HALT,
    INST_POP,
    INST_ADD,
    INST_SUB,
    INST_MUL,
    INST_DIV,

    // Instructions that take one argument
    INST_CALL,
    INST_PUSH,
    INST_MOVE,
};

typedef struct inst_t {
    inst_type_e type;
    value_t value;
} inst_t;

typedef struct vm_t vm_t;
typedef void (*libfunc)(vm_t*);

LIST_DEFINE(value_t, stack_t);
LIST_DEFINE(inst_t, code_t);
LIST_DEFINE(libfunc, libfuncs_t);

#define ALLOC_STACK (stack_t) LIST_ALLOC(value_t)
#define ALLOC_CODE  (code_t) LIST_ALLOC(inst_t)
#define ALLOC_FUNCS (libfuncs_t) LIST_ALLOC(libfunc)

struct vm_t {
    size_t pos;
    code_t code;
    stack_t stack;
    libfuncs_t lib;
};

vm_t init_vm(const char *path);
code_t load_bytecode(const char *path);
void free_vm(vm_t *vm);

value_t vm_pop(vm_t *vm);
void vm_push(vm_t *vm, value_t value);
void vm_move(vm_t *vm, value_t value);
void vm_call(vm_t *vm, value_t value);
void vm_run(vm_t *vm);

#endif
