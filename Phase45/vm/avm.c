#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "avm.h"
#include "exec_func/exec_libfunc.h"

unsigned char executionFinished = 0;
unsigned pc = 0;
unsigned currLine = 0;
unsigned codeSize = 0;
instruction* code = (instruction*) 0;
unsigned totalActuals = 0;
int insideFunction = 0;

char** runtimeLibfuncs = NULL;
unsigned runtimeLibfuncCount = 0;

#define GUARD_VAL 0xABCDEF42
unsigned int libfuncs_guard = GUARD_VAL;

avm_memcell ax, bx, cx;
avm_memcell retval;
unsigned top, topsp;
avm_memcell stack[AVM_STACKSIZE];

static unsigned hash_string(const char* key){
    unsigned hash = 0;
    while (*key)
        hash = hash * 33 + *key++;
    return hash % AVM_TABLE_HASHSIZE;
}

static unsigned hash_number(double n) {
    return ((unsigned long)n) % AVM_TABLE_HASHSIZE;
}


execute_func_t executeFuncs[] = {
    execute_ASSIGN,
    execute_ADD,
    execute_SUB,
    execute_MUL,
    execute_DIV,
    execute_MOD,
    execute_UMINUS,
    execute_AND,
    execute_OR,
    execute_NOT,
    execute_IF_EQ,
    execute_IF_NOTEQ,
    execute_IF_LESSEQ,
    execute_IF_GREATEREQ,
    execute_IF_LESS,
    execute_IF_GREATER,
    execute_CALL,
    execute_PARAM,
    execute_RETURN,
    execute_GETRETVAL,
    execute_FUNCSTART,
    execute_FUNCEND,
    execute_NEWTABLE,
    execute_TABLEGETELEM,
    execute_TABLESETELEM,
    execute_JUMP
};

void execute_cycle(void){
    //printf("Executing opcode: %d at PC: %d\n", code[pc].opcode, pc);
    if(executionFinished) return;
    else
    if(pc == AVM_ENDING_PC){
        //printf("AVM_ENDING_PC\n");
        executionFinished = 1;
        return;
    }else{
        //printf("We good\n");
        assert(pc<AVM_ENDING_PC);
        instruction* instr = code + pc;
        assert(
            instr->opcode >= 0 &&
            instr->opcode <= AVM_MAX_INSTRUCTIONS
        );
        if(instr->srcLine) currLine = instr->srcLine;
        unsigned oldPC = pc;
        (*executeFuncs[instr->opcode]) (instr);
        if(pc == oldPC) ++pc;
    }
}

static void avm_initstack(void){
    for(unsigned i=0; i<AVM_STACKSIZE; ++i){
        AVM_WIPEOUT(stack[i]);
        stack[i].type = undef_m;
    }
    top = AVM_STACKSIZE - 1000 - AVM_STACKENV_SIZE; //gloabal reserved :D
    topsp = AVM_STACKSIZE - 1;
}

void initialize_avm(void) {
    avm_initstack();
    retval.type = undef_m;
    executionFinished = 0;

    if (libfuncs_guard != GUARD_VAL) {
        fprintf(stderr, "[CORRUPTION] libfuncs_guard was overwritten!\n");
        exit(1);
    }

    libfunc_index = 0;

    avm_register_default_libfuncs();

    pc = 0;
    executionFinished = 0;
}

memclear_func_t memclearFuncs[]={
    0, /*Number*/
    memclear_string,
    0, /*Bool*/
    memclear_table,
    0, /*UserFunction*/
    0, /*LibFunction*/
    0, /*nil*/
    0, /*undef*/
};

char* libfuncs_getused(unsigned index) {
    assert(index < totalNamedLibfuncs);
    return namedLibfuncs[index];
}

char* consts_getstring(unsigned index) {
    //printf("[DEBUG] consts_getstring(%u), total = %u\n", index, totalStringConsts);
    assert(index < totalStringConsts);

    if (!stringConsts[index]) {
        fprintf(stderr, "[ERROR] stringConsts[%u] is NULL!\n", index);
        exit(1);
    }

    return stringConsts[index];
}

double consts_getnumber(unsigned index) {
    assert(index < totalNumConsts);
    if (index >= totalNumConsts) {
        fprintf(stderr, "[BUG] consts_getnumber(%u) out of bounds (totalNumConsts = %u)\n", index, totalNumConsts);
        exit(1);
    }
    return cumConsts[index];
}

avm_memcell* avm_translate_operand(vmarg* arg, avm_memcell* reg){
    //printf("[translate_operand] arg->type = %d, val = %u\n", arg->type, arg->val);
    switch(arg->type){
        /* Variables */
        case global_a: {
            unsigned index = AVM_STACKSIZE - 1 - arg->val;
            if (index >= AVM_STACKSIZE) {
                fprintf(stderr, "[FATAL] Stack overflow: global_a offset=%u yields index=%u\n", arg->val, index);
                exit(1);
            }
            return &stack[index];
        }
        case local_a: return &stack[topsp-arg->val];
        case formal_a: return &stack[topsp+AVM_STACKENV_SIZE+1+arg->val];

        case retval_a: return &retval;

        /* Constants */
        case number_a: {
            //printf("mhxiou\n");
            reg->type = number_m;
            reg->data.numVal = consts_getnumber(arg->val);
            return reg;
        }
        case string_a: {
            const char* s = consts_getstring(arg->val);
            if (reg->type == string_m && reg->data.strVal != NULL) {
                free(reg->data.strVal);
            }
            reg->type = string_m;
            reg->data.strVal = strdup(s);
            return reg;
        }
        case bool_a: {
            reg->type = bool_m;
            reg->data.boolVal = arg->val;
            return reg;
        }
        case nil_a: reg->type = nil_m; return reg;
        
        /* Functions */
        case userfunc_a: {
            reg->type = userfunc_m;
            /* If function address is directly stored */
            reg->data.funcVal = arg->val;
            /* If function index is func table is stored */
            reg->data.funcVal = userfuncs_getfunc(arg->val)->address;
            return reg;
        }
        case libfunc_a: {
            const char* name = libfuncs_getused(arg->val);
            reg->type = libfunc_m;
            reg->data.libfuncVal = NULL;

            for (unsigned i = 0; i < libfunc_index; ++i) {
                if (strcmp(name, libnames[i]) == 0) {
                    reg->data.libfuncVal = libfuncs[i];  // assign function pointer
                    break;
                }
            }

            if (!reg->data.libfuncVal) {
                fprintf(stderr, "[ERROR] Unknown libfunc: %s\n", name);
            }

            return reg;
        }
        default: assert(0);
    }
}

void memclear_string(avm_memcell* m){
    assert(m->type == string_m);
    if (!m->data.strVal) {
        fprintf(stderr, "[SAFEGUARD] Attempt to free NULL string — skipping.\n");
        return;
    }

    //printf("[memclear_string] About to free string at %p\n", (void*)m->data.strVal);
    free(m->data.strVal);
    m->data.strVal = NULL;
}

void memclear_table(avm_memcell* m){
    assert(m->data.tableVal);
    avm_tabledecrefcounter(m->data.tableVal);
}

void avm_memcellclear(avm_memcell* m){
    if (m->type == string_m && m->data.strVal) {
        //printf("[CLEAR] about to clear string: %s at %p\n", m->data.strVal, m->data.strVal);
    }
    if(m->type != undef_m){
        memclear_func_t f = memclearFuncs[m->type];
        if(f)
            (*f) (m);
        m->type = undef_m;
    }
}

void avm_tableincreafcounter(avm_table* t){
    ++t->refCounter;
}

void avm_tabledecrefcounter(avm_table* t){
    assert(t->refCounter > 0);
    if(!--t->refCounter){
        avm_tabledestroy(t);
    }
}

void avm_tablebucketsinit(avm_table_bucket** p){
    for(unsigned i=0;i<AVM_TABLE_HASHSIZE;++i){
        p[i] = (avm_table_bucket*) 0;
    }
}

avm_table* avm_tablenew(void){
    avm_table* t = (avm_table*) malloc(sizeof(avm_table));
    AVM_WIPEOUT(*t);

    t->refCounter = t->total = 0;
    avm_tablebucketsinit(t->numIndexed);
    avm_tablebucketsinit(t->strIndexed);

    return t;
}

void avm_tablebucketsdestroy(avm_table_bucket** p){
    for(unsigned i=0;i<AVM_TABLE_HASHSIZE;++i,++p){
        for(avm_table_bucket* b = *p; b;){
            avm_table_bucket* del = b;
            b = b->next;
            avm_memcellclear(&del->key);
            avm_memcellclear(&del->value);
            free(del);
        }
        p[i] = (avm_table_bucket*) 0;
    }
}

void avm_tabledestroy(avm_table* t){
    avm_tablebucketsdestroy(t->strIndexed);
    avm_tablebucketsdestroy(t->numIndexed);
    free(t);
}

/* 
    Helper functions to produce common args for
    generated instructions, like 1, 0, "true", "false"
    and function return values.
*/
void make_numberoperand(vmarg* arg, double val){
    arg->val = const_newnumber(val);
    arg->type = number_a;
}

void make_booloperand(vmarg* arg, unsigned val){
    arg->val = val;
    arg->type = bool_a;
}

void make_retvaloperand(vmarg* arg){
    arg->type = retval_a;
}

void avm_assign(avm_memcell* lv, const avm_memcell* rv) {
    if (lv == rv) return;

    if (rv->type == undef_m) {
        fprintf(stderr, "[ASSIGN ERROR] Cannot assign to '%s' from 'undef' at PC = %d\n",
                avm_tostring(lv), pc);
    }

    // Clear lv safely
    avm_memcellclear(lv);

    lv->type = rv->type;

    switch (rv->type) {
        case number_m:
            lv->data.numVal = rv->data.numVal;
            break;
        case string_m:
            lv->data.strVal = strdup(rv->data.strVal);
            break;
        case bool_m:
            lv->data.boolVal = rv->data.boolVal;
            break;
        case nil_m:
            // Nothing to assign
            break;
        case userfunc_m:
            lv->data.funcVal = rv->data.funcVal;
            break;
        case libfunc_m:
            lv->data.libfuncVal = rv->data.libfuncVal;
            break;
        case table_m:
            lv->data.tableVal = rv->data.tableVal;
            avm_tableincreafcounter(rv->data.tableVal);
            break;
        case undef_m:
            fprintf(stderr, "[Warning] Assigning from 'undef' value.\n");
            break;
        default:
            assert(0 && "Unsupported type in avm_assign");
    }
}


userfunc* userfuncs_getfunc(unsigned index) {
    assert(index < totalUserFuncs);
    // convert from userfunc (quad generator) to userfunc (runtime)
    static userfunc tmp;
    tmp.address   = userFuncs[index].address;
    tmp.localSize = userFuncs[index].localSize;
    tmp.id        = userFuncs[index].id;
    return &tmp;
}

avm_memcell* avm_tablegetelem(avm_table* t, avm_memcell* key) {
    assert(t && key);
    avm_table_bucket* b = NULL;

    switch (key->type) {
        case number_m: {
            unsigned idx = hash_number(key->data.numVal);
            b = t->numIndexed[idx];
            while (b) {
                if (b->key.type == number_m && b->key.data.numVal == key->data.numVal)
                    return &b->value;
                b = b->next;
            }
            break;
        }
        case string_m: {
            unsigned idx = hash_string(key->data.strVal);
            b = t->strIndexed[idx];
            while (b) {
                if (b->key.type == string_m && strcmp(b->key.data.strVal, key->data.strVal) == 0)
                    return &b->value;
                b = b->next;
            }
            break;
        }
        default:
            fprintf(stderr, "[Error] Unsupported key type in avm_tablegetelem.\n");
            executionFinished = 1;
            return NULL;
    }

    return NULL;
}

void avm_tablesetelem(avm_table* t, avm_memcell* key, avm_memcell* value) {
    assert(t && key && value);
    avm_table_bucket** bucketList = NULL;
    avm_table_bucket* b = NULL;
    unsigned idx;

    switch (key->type) {
        case number_m:
            idx = hash_number(key->data.numVal);
            bucketList = t->numIndexed;
            break;
        case string_m:
            idx = hash_string(key->data.strVal);
            bucketList = t->strIndexed;
            break;
        default:
            fprintf(stderr, "[Runtime Error] Unsupported key type in table set.\n");
            executionFinished = 1;
            return;
    }

    // Search for existing key
    for (b = bucketList[idx]; b; b = b->next) {
        if ((key->type == number_m && b->key.data.numVal == key->data.numVal) ||
            (key->type == string_m && strcmp(b->key.data.strVal, key->data.strVal) == 0)) {
            avm_assign(&b->value, value);
            return;
        }
    }

    // Insert new
    b = malloc(sizeof(avm_table_bucket));
    AVM_WIPEOUT(*b);
    assert(b);
    avm_assign(&b->key, key);
    avm_assign(&b->value, value);
    b->next = bucketList[idx];
    bucketList[idx] = b;
    t->total++;
}

void execute_JUMP(instruction* instr) {
    pc = instr->result.val;
}

char* avm_tostring(avm_memcell* m) {
    char* s = malloc(64);
    switch (m->type) {
        case number_m: sprintf(s, "%.2f", m->data.numVal); break;
        case string_m: {
            const char* raw = m->data.strVal;
            size_t len = strlen(raw);
            if (len >= 2 && raw[0] == '"' && raw[len - 1] == '"') {
                // Strip surrounding quotes
                char* unquoted = malloc(len - 1);
                strncpy(unquoted, raw + 1, len - 2);
                unquoted[len - 2] = '\0';
                return unquoted;
            } else {
                return strdup(raw);
            }
        }
        case bool_m: return strdup(m->data.boolVal ? "true" : "false");
        case nil_m: return strdup("nil");
        case userfunc_m: sprintf(s, "userfunc_%u", m->data.funcVal); break;
        case libfunc_m: return strdup("libfunc");
        case undef_m: return strdup("undef");
        default: return strdup("unknown");
    }
    return s;
}

void load_binary(const char* filename) {
    //printf("[DEBUG] Loading bytecode from: %s\n", filename);
    FILE* f = fopen(filename, "rb");
    if (!f) {
        perror("load_binary: Failed to open file");
        exit(1);
    }

    unsigned magic;
    fread(&magic, sizeof(unsigned), 1, f);
    if (magic != MAGIC_NUMBER) {
        fprintf(stderr, "[Error] Invalid binary file format.\n");
        fclose(f);
        exit(1);
    }

    // Load stringConsts
    fread(&totalStringConsts, sizeof(unsigned), 1, f);
    stringConsts = malloc(totalStringConsts * sizeof(char*));
    for (unsigned i = 0; i < totalStringConsts; ++i) {
        unsigned len;
        fread(&len, sizeof(unsigned), 1, f);
        stringConsts[i] = malloc(len + 1);
        fread(stringConsts[i], sizeof(char), len, f);
        stringConsts[i][len] = '\0';
    }

    // Load numConsts
    fread(&totalNumConsts, sizeof(unsigned), 1, f);
    cumConsts = malloc(totalNumConsts * sizeof(double));
    fread(cumConsts, sizeof(double), totalNumConsts, f);

    // Load libfuncs
    fread(&runtimeLibfuncCount, sizeof(unsigned), 1, f);
    runtimeLibfuncs = malloc(runtimeLibfuncCount * sizeof(char*));
    namedLibfuncs = runtimeLibfuncs;
    totalNamedLibfuncs = runtimeLibfuncCount;
    for (unsigned i = 0; i < runtimeLibfuncCount; ++i) {
        unsigned len;
        fread(&len, sizeof(unsigned), 1, f);
        runtimeLibfuncs[i] = malloc(len + 1);
        fread(runtimeLibfuncs[i], sizeof(char), len, f);
        runtimeLibfuncs[i][len] = '\0';
    }

    // Load userFuncs
    fread(&totalUserFuncs, sizeof(unsigned), 1, f);
    userFuncs = malloc(totalUserFuncs * sizeof(userfunc));
    for (unsigned i = 0; i < totalUserFuncs; ++i) {
        fread(&userFuncs[i].address, sizeof(unsigned), 1, f);
        fread(&userFuncs[i].localSize, sizeof(unsigned), 1, f);

        unsigned len;
        fread(&len, sizeof(unsigned), 1, f);
        userFuncs[i].id = malloc(len + 1);
        fread(userFuncs[i].id, sizeof(char), len, f);
        userFuncs[i].id[len] = '\0';
    }

    // Load instructions
    fread(&currInstr, sizeof(unsigned), 1, f);
    instructions = malloc(currInstr * sizeof(instruction));
    for (unsigned i = 0; i < currInstr; ++i) {
        instruction* instr = &instructions[i];
        unsigned char type;

        fread(&instr->opcode, sizeof(unsigned), 1, f);

        fread(&type, sizeof(unsigned char), 1, f);
        instr->result.type = (vmarg_t)type;
        fread(&instr->result.val, sizeof(unsigned), 1, f);

        fread(&type, sizeof(unsigned char), 1, f);
        instr->arg1.type = (vmarg_t)type;
        fread(&instr->arg1.val, sizeof(unsigned), 1, f);

        fread(&type, sizeof(unsigned char), 1, f);
        instr->arg2.type = (vmarg_t)type;
        fread(&instr->arg2.val, sizeof(unsigned), 1, f);

        fread(&instr->srcLine, sizeof(unsigned), 1, f);
        /*
        if (i == 11) {
            printf("[ASSERT] INSTR 11: opcode=%u, arg1.type=%u, arg1.val=%u\n",
                instr->opcode, instr->arg1.type, instr->arg1.val);
        }
            */
    }

    code = instructions;
    codeSize = currInstr;
    fclose(f);
}


