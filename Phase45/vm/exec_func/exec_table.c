#include "exec_table.h"
#include <stdio.h>
#include <assert.h>

extern avm_memcell stack[AVM_STACKSIZE];
extern unsigned char executionFinished;

extern avm_memcell ax, bx, cx;

void execute_NEWTABLE(instruction* instr){
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    assert(lv);

    avm_memcellclear(lv);
    lv->type = table_m;
    lv->data.tableVal = avm_tablenew();
    avm_tableincreafcounter(lv->data.tableVal);
}

void execute_TABLEGETELEM(instruction* instr){
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    avm_memcell* t  = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* i  = avm_translate_operand(&instr->arg2, &bx);

    assert(lv && t && i);
    avm_memcellclear(lv);

    if (t->type != table_m) {
        fprintf(stderr, "[Error] Attempt to index non-table value.\n");
        executionFinished = 1;
        return;
    }
    
    avm_memcell* content = avm_tablegetelem(t->data.tableVal, i);
    if (content)
        avm_assign(lv, content);
    else
        lv->type = nil_m;
}

void execute_TABLESETELEM(instruction* instr){
    avm_memcell* t = avm_translate_operand(&instr->result, &ax);
    avm_memcell* i = avm_translate_operand(&instr->arg1, &bx);
    avm_memcell* c = avm_translate_operand(&instr->arg2, &cx);

    assert(t && i && c);

    if (t->type != table_m) {
        fprintf(stderr, "[Error] Attempt to set element on non-table value.\n");
        executionFinished = 1;
        return;
    }

    avm_tablesetelem(t->data.tableVal, i, c);
}
