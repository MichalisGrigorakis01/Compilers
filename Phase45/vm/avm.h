#ifndef AVM_H
#define AVM_H

//#include "../quads.h"
//#include "../expressions.h"
#include "../generate.h"
#include "../binary.h"
#include <math.h>

#define AVM_MAX_INSTRUCTIONS (unsigned) nop_v
#define AVM_TABLE_HASHSIZE 211
#define AVM_ENDING_PC codeSize
#define AVM_STACKENV_SIZE 4

typedef enum avm_memcell_t{
    number_m = 0,
    string_m = 1,
    bool_m = 2,
    table_m = 3,
    userfunc_m = 4,
    libfunc_m = 5,
    nil_m = 6,
    undef_m = 7
}avm_memcell_t;

typedef struct avm_table avm_table;
typedef void (*library_func_t)(void);

typedef struct avm_memcell{
    avm_memcell_t type;
    union{
        double numVal;
        char* strVal;
        unsigned char boolVal;
        avm_table* tableVal;
        unsigned funcVal;
        library_func_t libfuncVal;
    } data;
}avm_memcell;

#define AVM_STACKSIZE 4096
#define AVM_WIPEOUT(m) memset(&(m), 0, sizeof(m))

extern avm_memcell stack[AVM_STACKSIZE];
extern avm_memcell retval;
extern unsigned top, topsp, pc;
extern unsigned char executionFinished;

extern avm_memcell ax, bx, cx;

static void avm_initstack(void);

avm_memcell* avm_translate_operand(vmarg* arg, avm_memcell* reg);

typedef struct avm_table_bucket avm_table_bucket;
typedef struct avm_table avm_table;

typedef struct avm_table_bucket{
    avm_memcell key;
    avm_memcell value;
    avm_table_bucket* next;
}avm_table_bucket;

typedef struct avm_table{
    unsigned refCounter;
    avm_table_bucket* strIndexed[AVM_TABLE_HASHSIZE];
    avm_table_bucket* numIndexed[AVM_TABLE_HASHSIZE];
    unsigned total;
}avm_table;

avm_table* avm_tablenew(void);
void avm_tabledestroy(avm_table* t);
avm_memcell* avm_tablegetelem(avm_table* t, avm_memcell* key);
void avm_tablesetelem(avm_table* t, avm_memcell* key, avm_memcell* value);

/* We support only numeric and strings keys. Bonus if we implement keys for functions, lib functions and booleans */

void avm_tableincreafcounter(avm_table* t);
void avm_tabledecrefcounter(avm_table* t);
void avm_tablebucketsinit(avm_table_bucket** p);
avm_table* avm_tablenew(void);

void avm_memcellclear(avm_memcell* m);
void avm_tablebucketsdestroy(avm_table_bucket** p);
void avm_tabledestroy(avm_table* t);

void make_numberoperand(vmarg* arg, double val);
void make_booloperand(vmarg* arg, unsigned val);
void make_retvaloperand(vmarg* arg);

typedef void (*execute_func_t) (instruction*);
typedef void (*memclear_func_t) (avm_memcell*);

extern void memclear_string(avm_memcell* m);

extern void memclear_table(avm_memcell* m);

void execute_cycle(void);
void initialize_avm(void);
extern void execute_ASSIGN(instruction* instr);
extern void execute_ADD(instruction* instr);
extern void execute_SUB(instruction* instr);
extern void execute_MUL(instruction* instr);
extern void execute_DIV(instruction* instr);
extern void execute_MOD(instruction* instr);
extern void execute_UMINUS(instruction* instr);
extern void execute_AND(instruction* instr);
extern void execute_OR(instruction* instr);
extern void execute_NOT(instruction* instr);
extern void execute_IF_EQ(instruction* instr);
extern void execute_IF_NOTEQ(instruction* instr);
extern void execute_IF_LESSEQ(instruction* instr);
extern void execute_IF_GREATEREQ(instruction* instr);
extern void execute_IF_LESS(instruction* instr);
extern void execute_IF_GREATER(instruction* instr);
extern void execute_CALL(instruction* instr);
extern void execute_PARAM(instruction* instr);
extern void execute_RETURN(instruction* instr);
extern void execute_GETRETVAL(instruction* instr);
extern void execute_FUNCSTART(instruction* instr);
extern void execute_FUNCEND(instruction* instr);
extern void execute_NEWTABLE(instruction* instr);
extern void execute_TABLEGETELEM(instruction* instr);
extern void execute_TABLESETELEM(instruction* instr);
void execute_JUMP(instruction* instr);

char* avm_tostring(avm_memcell* m);
void avm_assign(avm_memcell* lv, const avm_memcell* rv);
double consts_getnumber(unsigned index);
char* consts_getstring(unsigned index);
char* libfuncs_getused(unsigned index);
userfunc* userfuncs_getfunc(unsigned index);
void load_binary(const char* filename);


#endif