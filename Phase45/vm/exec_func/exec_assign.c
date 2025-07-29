#include <stdio.h>
#include "exec_assign.h"
#include <assert.h>

extern avm_memcell ax, bx, cx;

void execute_ASSIGN(instruction* instr) {
    if (instr->result.type == nil_a) {
        fprintf(stderr, "[ASSIGN ERROR] Cannot assign to 'nil' at PC = %u\n", pc);
        executionFinished = 1;
        return;
    }
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    avm_memcell temp;
    AVM_WIPEOUT(temp);

    avm_memcell* rv = avm_translate_operand(&instr->arg1, &temp);
    assert(lv && rv);

    avm_assign(lv, rv);
}

