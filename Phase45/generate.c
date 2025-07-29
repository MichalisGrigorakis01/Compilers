#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "generate.h"

#define EXPAND_INST_SIZE 1024
#define CURR_INST_SIZE (currInstr * sizeof(instruction))
#define NEW_INST_SIZE (EXPAND_INST_SIZE * sizeof(instruction) + CURR_INST_SIZE)

instruction* instructions = NULL;
unsigned currInstr = 0;
unsigned totalInstr = 0;
extern unsigned total;
extern quad* quads;

double* cumConsts = NULL;
unsigned totalNumConsts;
char** stringConsts = NULL;
unsigned totalStringConsts;
char** namedLibfuncs = NULL;
unsigned totalNamedLibfuncs;
userfunc* userFuncs = NULL;
unsigned totalUserFuncs;

incomplete_jump* ij_head = (incomplete_jump*) 0;
unsigned ij_total = 0;

char* opcode_to_string(enum vmopcode op) {
    switch(op) {
        case assign_v: return "assign_v";
        case add_v: return "add_v";
        case sub_v: return "sub_v";
        case mul_v: return "mul_v";
        case div_v: return "div_v";
        case mod_v: return "mod_v";
        case jump_v: return "jump_v";
        case jeq_v: return "jeq_v";
        case jne_v: return "jne_v";
        case jgt_v: return "jgt_v";
        case jge_v: return "jge_v";
        case jlt_v: return "jlt_v";
        case jle_v: return "jle_v";
        case call_v: return "call_v";
        case pusharg_v: return "pusharg_v";
        case funcenter_v: return "funcenter_v";
        case funcexit_v: return "funcexit_v";
        case getretval_v: return "getretval_v";
        case ret_v: return "ret_v";
        case newtable_v: return "newtable_v";
        case tablegetelem_v: return "tablegetelem_v";
        case table_setelem_v: return "table_setelem_v";
        case nop_v: return "nop_v";
        case not_v: return "not_v";
        case and_v: return "and_v";
        case or_v: return "or_v";
        case uminus_v: return "uminus_v";
        default: return "UNKNOWN";
    }
}

char* vmarg_type_to_string(vmarg_t type) {
    switch (type) {
        case label_a: return "label_a";
        case global_a: return "global_a";
        case formal_a: return "formal_a";
        case local_a: return "local_a";
        case number_a: return "number_a";
        case string_a: return "string_a";
        case bool_a: return "bool_a";
        case nil_a: return "nil_a";
        case userfunc_a: return "userfunc_a";
        case libfunc_a: return "libfunc_a";
        case retval_a: return "retval_a";
        default: return "UNKNOWN_TYPE";
    }
}


generator_func_t generators[] = {
    generate_ASSIGN,
    generate_ADD,
    generate_SUB,
    generate_MUL,
    generate_DIV,
    generate_MOD,
    generate_UMINUS,
    generate_AND,
    generate_OR,
    generate_NOT,
    generate_IF_EQ,
    generate_IF_NOTEQ,
    generate_IF_LESSEQ,
    generate_IF_GREATEREQ,
    generate_IF_LESS,
    generate_IF_GREATER,
    generate_CALL,
    generate_PARAM,
    generate_RETURN,
    generate_GETRETVAL,
    generate_FUNCSTART,
    generate_FUNCEND,
    generate_NEWTABLE,
    generate_TABLEGETELEM,
    generate_TABLESETELEM,
    generate_JUMP
};

void generate(void){
    for(unsigned i = 0; i<total; ++i){
        if (quads[i].op < sizeof(generators)/sizeof(generator_func_t)) {
            (*generators[quads[i].op]) (quads+i);
        }else{
            fprintf(stderr, "[ERROR] Unknown quad opcode: %d at index %u\n", quads[i].op, i);
        }
    }
}

unsigned const_newstring (char* s){
    for (unsigned i = 0; i < totalStringConsts; ++i) {
        if (strcmp(stringConsts[i], s) == 0)
            return i;
    }

    stringConsts = realloc(stringConsts, (totalStringConsts + 1) * sizeof(char*));
    stringConsts[totalStringConsts] = strdup(s); // store a copy
    return totalStringConsts++;
}

unsigned const_newnumber(double n){
    for (unsigned i = 0; i < totalNumConsts; ++i) {
        if (cumConsts[i] == n)
            return i;
    }

    cumConsts = realloc(cumConsts, (totalNumConsts + 1) * sizeof(double));
    cumConsts[totalNumConsts] = n;
    return totalNumConsts++;
}

unsigned libfuncs_newused(char* s){
    for (unsigned i = 0; i < totalNamedLibfuncs; ++i) {
        if (strcmp(namedLibfuncs[i], s) == 0)
            return i;
    }

    namedLibfuncs = realloc(namedLibfuncs, (totalNamedLibfuncs + 1) * sizeof(char*));
    namedLibfuncs[totalNamedLibfuncs] = strdup(s);
    return totalNamedLibfuncs++;
}

void make_operand(expr* e, vmarg* arg){
    if (!e || !arg) return;

    memset(arg, 0, sizeof(vmarg));

    switch(e->type){
        /* All those below use a variable for storage */
        case assignexpr_e:
        case var_e:
        case tableitem_e:
        case arithexpr_e:
        case boolexpr_e:
        case newtable_e: {
            assert(e->sym);
            arg->val = e->sym->offset;
            switch(e->sym->space){
                case programvar: arg->type = global_a; break;
                case functionlocal:{
                    //printf("[DEBUG] var: %s, scope: %d, offset: %u → ", e->sym->name, e->sym->scope, e->sym->offset);
                    if (e->sym->scope == 0){
                        arg->type = global_a;
                        //printf("global_a\n");
                    }else{
                        arg->type = local_a;
                        //printf("local_a\n");
                    }
                        break;
                }
                case formalarg: arg->type = formal_a; break;
                default: assert(0);
            }
            break;
        }
        /* Constants */
        case constbool_e: {
            arg->val = e->boolConst;
            arg->type = bool_a; 
            break;
        }
        case conststring_e: {
            arg->val = const_newstring(e->strConst);
            arg->type = string_a;
            break;
        }
        case costnum_e: {
            arg->val = const_newnumber(e->numConst);
            arg->type = number_a;
            break;
        }
        case nil_e: arg->type = nil_a; break;

        /* Functions */
        case programfunc_e: {
            assert(e->sym);

            unsigned index = 0;
            for (; index < totalUserFuncs; ++index) {
                if (strcmp(userFuncs[index].id, e->sym->name) == 0)
                    break;
            }

            if (index == totalUserFuncs) {
                // New user function — add it
                userFuncs = realloc(userFuncs, sizeof(userfunc) * (totalUserFuncs + 1));
                userFuncs[totalUserFuncs].address = e->sym->taddress;
                userFuncs[totalUserFuncs].localSize = 0;
                userFuncs[totalUserFuncs].id = strdup(e->sym->name);
                index = totalUserFuncs++;
            }

            arg->type = userfunc_a;
            arg->val = index;
            break;
        }
        case libraryfunc_e: {
            assert(e->sym);
            //printf("make_operand: expr type = %d, symbol = %s\n", e->type, e->sym ? e->sym->name : "NULL");
            //printf("[DEBUG] make_operand(): programfunc_e with taddress=%u\n", e->sym->taddress);
            arg->type = libfunc_a;
            arg->val = libfuncs_newused(e->sym->name);
            break;
        }
        default: fprintf(stderr, "[make_operand] ERROR: Unhandled expr type: %d\n", e->type); assert(0);
    }
}

void patch_incomplete_jumps(){
    incomplete_jump* tmp = ij_head;
    while (tmp) {
        if (tmp->iaddress == nextquad())
            instructions[tmp->instrNo].result.val = currInstr;
        else
            instructions[tmp->instrNo].result.val = quads[tmp->iaddress].label;

        tmp = tmp->next;
    }
}

void emit_instruction(instruction* instr) {
    if (currInstr == totalInstr) {
        instruction* new_space = malloc(NEW_INST_SIZE);
        if (instructions)
            memcpy(new_space, instructions, CURR_INST_SIZE);
        free(instructions);
        instructions = new_space;
        totalInstr += EXPAND_INST_SIZE;
    }
    instructions[currInstr++] = *instr;
}

void generate_ALL(quad* q, enum vmopcode op){
    instruction t;
    memset(&t, 0, sizeof(instruction));
    t.arg1.type = nil_a;
    t.arg2.type = nil_a;
    t.result.type = nil_a;
    t.opcode = op;

    if (!q) return;

    switch (op) {
        case pusharg_v:
            if (q->arg1) make_operand(q->arg1, &t.arg1);
            break;

        case getretval_v:
            if (q->result) make_operand(q->result, &t.result);
            break;

        default:
            if (q->arg1) make_operand(q->arg1, &t.arg1);
            if (q->arg2) make_operand(q->arg2, &t.arg2);
            if (q->result) make_operand(q->result, &t.result);
            break;
    }

    t.srcLine = q->line;
    emit_instruction(&t);
}

void generate_ADD(quad* q){
    generate_ALL(q, add_v);
}

void generate_SUB (quad* q){
    generate_ALL(q, sub_v);
}
void generate_MUL (quad* q){
    generate_ALL(q, mul_v);
}
void generate_DIV (quad* q){
    generate_ALL(q, div_v);
}
void generate_MOD (quad* q){
    generate_ALL(q, mod_v);
}

void generate_UMINUS(quad* q){
    generate_ALL(q, uminus_v);
}

void generate_NEWTABLE (quad* q){
    generate_ALL(q, newtable_v);
}
void generate_TABLEGETELEM (quad* q){
    generate_ALL(q, tablegetelem_v);
}
void generate_TABLESETELEM (quad* q){
    generate_ALL(q, table_setelem_v);
}

void generate_ASSIGN (quad* q){
    
    if (!q || !q->arg1 || !q->result) {
       // fprintf(stderr, "[generate_ASSIGN] ERROR: Missing arg1 or result in assign at line %u\n", q ? q->line : 0);
        return;
    }

    instruction t;
    memset(&t, 0, sizeof(instruction));
    t.opcode = assign_v;

    make_operand(q->arg1, &t.arg1);
    make_operand(q->result, &t.result);

    if (t.result.type == nil_a) {
        fprintf(stderr, "[generate_ASSIGN] Invalid target: result is nil_a at line %u\n", q->line);
        return;
    }

    t.srcLine = q->line;
    emit_instruction(&t);
}

void generate_NOP (quad* q){
    generate_ALL(q, nop_v);
}

void add_incomplete_jump(unsigned instrNo, unsigned iaddress) {
    incomplete_jump* new_jump = (incomplete_jump*) malloc(sizeof(incomplete_jump));
    new_jump->instrNo = instrNo;
    new_jump->iaddress = iaddress;
    new_jump->next = ij_head;
    ij_head = new_jump;
    ++ij_total;
}

void generate_JUMP (quad* q){
    instruction t;
    memset(&t, 0, sizeof(instruction));
    t.opcode = jump_v;
    t.result.type = label_a;
    t.result.val = q->label;  // might need patching
    t.srcLine = q->line;

    emit_instruction(&t);

    if (q->label >= nextquad()){
        add_incomplete_jump(currInstr - 1, q->label);
    }
    //printf("[JUMP] emit jump to label %u (srcLine: %u)\n", q->label, q->line);
}

void generate_ALL_IF(quad* q, enum vmopcode op) {
    if (!q || (!q->arg1 && !q->result)) return;
    instruction t;
    memset(&t, 0, sizeof(instruction));
    t.arg1.type = nil_a;
    t.arg2.type = nil_a;
    t.result.type = nil_a;

    t.opcode = op;

    if (q->arg1) make_operand(q->arg1, &t.arg1);
    if (q->arg2) make_operand(q->arg2, &t.arg2);

    t.result.type = label_a;
    t.result.val = q->label;  // jump target
    t.srcLine = q->line;

    emit_instruction(&t);

    // Handle forward jumps that may need patching
    if (q->label >= nextquad()) {
        add_incomplete_jump(currInstr - 1, q->label);
    }
}

void generate_IF_EQ (quad* q){ generate_ALL_IF(q, jeq_v); }
void generate_IF_NOTEQ (quad* q){ generate_ALL_IF(q, jne_v); }
void generate_IF_GREATER (quad* q){ generate_ALL_IF(q, jgt_v); }
void generate_IF_GREATEREQ (quad* q){ generate_ALL_IF(q, jge_v); }
void generate_IF_LESS (quad* q){ generate_ALL_IF(q, jlt_v); }
void generate_IF_LESSEQ (quad* q){ generate_ALL_IF(q, jle_v); }


void generate_NOT (quad* q){
    generate_ALL(q, not_v);
}
void generate_OR (quad* q){
    generate_ALL(q, or_v);
}
void generate_AND (quad* q){
    generate_ALL(q, and_v);
}
void generate_PARAM (quad* q){
    generate_ALL(q, pusharg_v);
}

void generate_CALL(quad* q) {
    if (!q || !q->arg1) return;

    instruction t;
    memset(&t, 0, sizeof(instruction));
    t.opcode = call_v;
    make_operand(q->arg1, &t.arg1);
    t.result.type = nil_a;
    t.arg2.type = nil_a;
    t.srcLine = q->line;
    emit_instruction(&t);

    // Special handling for library functions
    if (q->arg1 && q->arg1->type == libraryfunc_e) {
        const char* name = q->arg1->sym->name;

        if (strcmp(name, "print") == 0) return;

        if (
            strcmp(name, "input") == 0 ||
            strcmp(name, "typeof") == 0 ||
            strcmp(name, "totalarguments") == 0 ||
            strcmp(name, "argument") == 0 ||
            strcmp(name, "strtonum") == 0 ||
            strcmp(name, "sqrt") == 0 ||
            strcmp(name, "cos") == 0 ||
            strcmp(name, "sin") == 0 ||
            strcmp(name, "objectcopy") == 0 ||
            strcmp(name, "objectmemberkeys") == 0 ||
            strcmp(name, "objecttotalmembers") == 0
        ) {
            if (q->result) {
                generate_ALL(q, getretval_v);
            }
            return;
        }

        return;
    }

    // For user functions or unknown types
    if (q->result) {
        generate_ALL(q, getretval_v);
    }
}

void generate_GETRETVAL(quad* q) {
    assert(q && q->result);
    instruction t;
    memset(&t, 0, sizeof(instruction));
    t.opcode = getretval_v;
    make_operand(q->result, &t.result);
    t.arg1.type = nil_a;
    t.arg2.type = nil_a;
    t.srcLine = q->line;
    emit_instruction(&t);
}

void generate_FUNCSTART(quad* q) {
    assert(q->result && q->result->sym);
    q->result->sym->taddress = currInstr;

    userFuncs = realloc(userFuncs, sizeof(userfunc) * (totalUserFuncs + 1));
    userFuncs[totalUserFuncs].address = currInstr;
    userFuncs[totalUserFuncs].localSize = q->result->sym->totalLocals;
    userFuncs[totalUserFuncs].id = strdup(q->result->sym->name);
    q->result->sym->taddress = totalUserFuncs;
    totalUserFuncs++;

    generate_ALL(q, funcenter_v);
}


void generate_RETURN (quad* q){
    instruction t;
    memset(&t, 0, sizeof(instruction));
    t.arg1.type = nil_a;
    t.arg2.type = nil_a;
    t.result.type = nil_a;
    t.opcode = assign_v;
    if (q->arg1) make_operand(q->arg1, &t.arg1);
    t.result.type = retval_a;
    t.srcLine = q->line;
    emit_instruction(&t);
}
void generate_FUNCEND (quad* q){
    generate_ALL(q, funcexit_v);
}

void print_vmarg(vmarg* arg) {
    //if (arg->type == nil_a) return;
    printf("[type=%s, val=%u] ", vmarg_type_to_string(arg->type), arg->val);
}

void print_instructions() {
    printf("\n==== GENERATED INSTRUCTIONS ====\n");
    for (unsigned i = 0; i < currInstr; ++i) {
        printf("%3u: %-12s ", i, opcode_to_string(instructions[i].opcode));
        print_vmarg(&instructions[i].result);
        print_vmarg(&instructions[i].arg1);
        print_vmarg(&instructions[i].arg2);
        printf("(srcLine: %u)\n", instructions[i].srcLine);
    }
}


