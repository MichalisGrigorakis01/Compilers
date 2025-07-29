#ifndef GENERATE_H
#define GENERATE_H

//#include "expressions.h"
#include "quads.h"

unsigned const_newstring (char* s);
unsigned const_newnumber(double n);
unsigned libfuncs_newused(char* s);



typedef void (*generator_func_t) (quad*);

void generate(void);
void generate_ADD (quad*);
void generate_SUB (quad*);
void generate_MUL (quad*);
void generate_DIV (quad*);
void generate_MOD (quad*);
void generate_NEWTABLE (quad*);
void generate_TABLEGETELEM (quad*);
void generate_TABLESETELEM (quad*);
void generate_ASSIGN (quad*);
void generate_NOP (quad*);
void generate_JUMP (quad*);
void generate_IF_EQ (quad*);
void generate_IF_NOTEQ (quad*);
void generate_IF_GREATER (quad*);
void generate_IF_GREATEREQ (quad*);
void generate_IF_LESS (quad*);
void generate_IF_LESSEQ (quad*);
void generate_NOT (quad*);
void generate_OR (quad*);
void generate_AND (quad*);
void generate_PARAM (quad*);
void generate_CALL (quad*);
void generate_GETRETVAL (quad*);
void generate_FUNCSTART (quad*);
void generate_RETURN (quad*);
void generate_FUNCEND (quad*);
void generate_UMINUS (quad*);

typedef enum vmopcode{
    assign_v,
    add_v,
    sub_v,
    mul_v,
    div_v,
    mod_v,
    uminus_v,
    and_v,
    or_v,
    not_v,
    jeq_v,
    jne_v,
    jle_v,
    jge_v,
    jlt_v,
    jgt_v,
    call_v,
    pusharg_v,
    ret_v,
    getretval_v,
    funcenter_v,
    funcexit_v,
    newtable_v,
    tablegetelem_v,
    table_setelem_v,
    jump_v,
    nop_v
}vmopcode;

typedef enum vmarg_t{
    label_a = 0,
    global_a = 1,
    formal_a = 2,
    local_a = 3,
    number_a = 4,
    string_a = 5,
    bool_a = 6,
    nil_a = 7,
    userfunc_a = 8,
    libfunc_a = 9,
    retval_a = 10
}vmarg_t;

typedef struct vmarg{
    vmarg_t type;
    unsigned val;
}vmarg;

typedef struct instruction{
    vmopcode opcode;
    vmarg result;
    vmarg arg1;
    vmarg arg2;
    unsigned srcLine;
}instruction;

typedef struct userfunc{
    unsigned address;
    unsigned localSize;
    char* id;
}userfunc;

typedef struct incomplete_jump incomplete_jump;

struct incomplete_jump{
    unsigned instrNo; // The jump instruction number.
    unsigned iaddress; // The i-code jump-target address.
    incomplete_jump* next; // A trivial linked list.
};

extern instruction* instructions;
extern unsigned currInstr;
extern unsigned totalInstr;

extern double* cumConsts;
extern unsigned totalNumConsts;

extern char** stringConsts;
extern unsigned totalStringConsts;

extern char** namedLibfuncs;
extern unsigned totalNamedLibfuncs;

extern userfunc* userFuncs;
extern unsigned totalUserFuncs;

void make_operand(expr* e, vmarg* arg);
void add_incomplete_jump(unsigned instrNo, unsigned iaddress);
void emit_instruction(instruction* instr);
void generate_ALL(quad* q, enum vmopcode op);
void generate_ALLJUMPS (quad* q, enum vmopcode op);
char* opcode_to_string(enum vmopcode op);
char* vmarg_type_to_string(vmarg_t type);
void print_vmarg(vmarg* arg);
void print_instructions();
void patch_incomplete_jumps();

#endif