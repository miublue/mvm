#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mlist.h"
#include "mvalue.h"
#include "mvm.h"
#include "mstdlib.h"

#include "mfile.h"

static libfuncs_t load_stdlib();
static void vm_math(vm_t *vm, int op);

static libfuncs_t
load_stdlib()
{
    libfuncs_t lib = ALLOC_FUNCS;
    LIST_ADD(lib, lib.size, lib_print);
    return lib;
}

static void
vm_math(vm_t *vm, int op)
{
    value_t a = vm_pop(vm);
    value_t b = vm_pop(vm);

    if (a.type != TYPE_INTEGER || b.type != TYPE_INTEGER) {
        fprintf(stderr, "runtime error: expected integer\n");
        exit(1);
    }

    value_t res = (value_t) {
        .type = TYPE_INTEGER,
        .is_mut = false,
        .is_ptr = false,
    };

    switch (op) {
    case INST_ADD:
        res.as_int = b.as_int + a.as_int;
        break;
    case INST_SUB:
        res.as_int = b.as_int - a.as_int;
        break;
    case INST_MUL:
        res.as_int = b.as_int * a.as_int;
        break;
    case INST_DIV:
        res.as_int = b.as_int / a.as_int;
        break;
    default:
        fprintf(stderr, "runtime error: unexpected instruction\n");
        exit(1);
    }

    vm_push(vm, res);
}

#define LEN(x) (sizeof(x) / sizeof((x)[0]))

vm_t
init_vm(const char *path)
{
    vm_t vm = {0};
    /* vm.code = ALLOC_CODE; */
    vm.stack = ALLOC_STACK;
    vm.lib = load_stdlib();
    vm.code = load_bytecode(path);

    return vm;
}

void
free_vm(vm_t *vm)
{
    LIST_FREE(vm->code);
    LIST_FREE(vm->stack);
    LIST_FREE(vm->lib);
}

value_t
vm_pop(vm_t *vm)
{
    if (vm->stack.size < 1) {
        fprintf(stderr, "runtime error: stack underflow\n");
        exit(1);
    }

    LIST_POP(vm->stack, vm->stack.size);
    return vm->stack.data[vm->stack.size];
}

void
vm_push(vm_t *vm, value_t value)
{
    LIST_ADD(vm->stack, vm->stack.size, value);
}

void
vm_move(vm_t *vm, value_t value)
{
    if (value.type != TYPE_INTEGER)
        goto fail;

    value_t val = vm_pop(vm);

    if (value.as_int < 0 || value.as_int > vm->stack.size)
        goto fail;

    vm->stack.data[value.as_int] = val;
    return;
fail:
    fprintf(stderr, "runtime error: invalid address\n");
    exit(1);
}

void
vm_call(vm_t *vm, value_t value)
{
    if (value.type != TYPE_INTEGER) {
        fprintf(stderr, "runtime error: expected function index\n");
        exit(1);
    }

    int64_t idx = value.as_int;
    libfunc fn = vm->lib.data[idx];
    fn(vm);
}

void
vm_run(vm_t *vm)
{
    for (vm->pos = 0; vm->pos < vm->code.size; ++vm->pos) {
        inst_t inst = vm->code.data[vm->pos];

        switch (inst.type) {
        case INST_HALT:
            return;
        case INST_CALL:
            vm_call(vm, inst.value);
            break;
        case INST_PUSH:
            vm_push(vm, inst.value);
            break;
        case INST_MOVE:
            vm_move(vm, inst.value);
            break;
        case INST_POP:
            vm_pop(vm);
            break;
        case INST_ADD:
        case INST_SUB:
        case INST_MUL:
        case INST_DIV:
            vm_math(vm, inst.type);
            break;
        default:
            fprintf(stderr, "runtime error: unexpected instruction %d\n", inst.type);
            break;
        }
    }
}

code_t
load_bytecode(const char *path)
{
    FILE *f = fopen(path, "rb");
    code_t code = ALLOC_CODE;

    size_t nbytes;
    char *bytes;

    fseek(f, 0, SEEK_END);
    nbytes = ftell(f);
    rewind(f);

    bytes = calloc(1, nbytes);
    fread(bytes, 1, nbytes, f);
    fclose(f);

    if (strncmp(bytes, MVM_HEADER, strlen(MVM_HEADER)) != 0) {
        fprintf(stderr, "error: invalid bytecode\n");
        goto fail;
    }

    uint8_t inst_size = MVM_INST_SIZE;
    for (size_t n = strlen(MVM_HEADER); n < nbytes; n += inst_size) {
        inst_size = 2;
        uint16_t inst_type = (bytes[n+0] << 8) | bytes[n+1];
        uint16_t value_type = 0;
        int64_t value = 0;

        // Only read argument if instruction accepts it
        if (inst_type >= INST_CALL) {
            inst_size = MVM_INST_SIZE;
            value_type = (bytes[n+2] << 8) | bytes[n+3];

            // LMFAO WTF
            value = bytes[n+4] << (64-8)
                            | bytes[n+5]  << (64-(8*2))
                            | bytes[n+6]  << (64-(8*3))
                            | bytes[n+7]  << (64-(8*4))
                            | bytes[n+8]  << (64-(8*5))
                            | bytes[n+9]  << (64-(8*6))
                            | bytes[n+10] << (64-(8*7))
                            | bytes[n+11];
        }

        // fprintf(stdout, "inst { %d, %d, %ld }\n",
        //         inst_type, value_type, value);

        inst_t inst = (inst_t) {
            .type = inst_type,
            .value = {
                .type = value_type,
                .as_int = value,
            },
        };
        LIST_ADD(code, code.size, inst);
    }

fail:
    free(bytes);
    return code;
}

int
main(int argc, const char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <file>\n", argv[0]);
        return 1;
    }

    vm_t vm = init_vm(argv[1]);

    vm_run(&vm);
    free_vm(&vm);
    return 0;
}
