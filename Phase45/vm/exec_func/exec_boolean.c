#include "exec_boolean.h"
#include <stdio.h>
#include <assert.h>

extern unsigned char executionFinished;

extern avm_memcell ax, bx;

unsigned char trans_to_bool(avm_memcell* m){
    switch(m->type){
        case undef_m: {
            fprintf(stderr, "[Error] 'undef' used in boolean expression!\n");
            executionFinished = 1;
            return 0;
        }
        case nil_m: return 0;
        case bool_m: return m->data.boolVal;
        case number_m: return m->data.numVal != 0;
        case string_m: return m->data.strVal[0] != '\0';
        case table_m:
        case userfunc_m:
        case libfunc_m: return 1;
        default: return 0;
    }
}

void execute_AND(instruction* instr){
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);

    avm_memcellclear(lv);
    lv->type = bool_m;
    lv->data.boolVal = trans_to_bool(rv1) && trans_to_bool(rv2);
}

void execute_OR(instruction* instr){
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);

    avm_memcellclear(lv);
    lv->type = bool_m;
    lv->data.boolVal = trans_to_bool(rv1) || trans_to_bool(rv2);
}

void execute_NOT(instruction* instr){
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    avm_memcell* rv = avm_translate_operand(&instr->arg1, &ax);

    avm_memcellclear(lv);
    lv->type = bool_m;
    lv->data.boolVal = !trans_to_bool(rv);
}
