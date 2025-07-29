#include "exec_func.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define AVM_STACKENV_SIZE 4

extern unsigned pc;
extern unsigned topsp;
extern unsigned top;
extern avm_memcell stack[AVM_STACKSIZE];
extern avm_memcell retval;
extern unsigned char executionFinished;
extern unsigned codeSize;
extern instruction* code;
extern unsigned totalActuals;
int insideLibfunc = 0;
extern avm_memcell ax;

void avm_callsaveenvironment(void) {
    stack[top + SAVED_TOP_OFFSET - 1].type = number_m;
    stack[top + SAVED_TOP_OFFSET - 1].data.numVal = top;

    stack[top + SAVED_TOPSP_OFFSET - 1].type = number_m;
    stack[top + SAVED_TOPSP_OFFSET - 1].data.numVal = topsp;

    stack[top + SAVED_PC_OFFSET - 1].type = number_m;
    stack[top + SAVED_PC_OFFSET - 1].data.numVal = pc + 1;

    stack[top + SAVED_TOTALACTUALS_OFFSET - 1].type = number_m;
    stack[top + SAVED_TOTALACTUALS_OFFSET - 1].data.numVal = totalActuals;

    topsp = top; // environment starts here
    top += AVM_STACKENV_SIZE; // reserve space
}

void avm_call_functor(avm_table* t) {
    fprintf(stderr, "[Error] Table as functor not supported (yet).\n");
    executionFinished = 1;
}

void execute_FUNCSTART(instruction* instr) {
    userfunc* funcInfo = &userFuncs[instr->result.val];
    assert(funcInfo);

    retval.type = nil_m;
    
    for (unsigned i = 0; i < funcInfo->localSize; ++i) {
        printf("we in\n");
        AVM_WIPEOUT(stack[top + i]);
        stack[top + i].type = undef_m;
    }

    top += funcInfo->localSize;
    //printf("[DEBUG] FUNCSTART: ENTERED function at pc=%u (locals=%u)\n", pc, funcInfo->localSize);
    pc++;
}

void execute_FUNCEND(instruction* instr) {
    unsigned restored_top = stack[topsp + SAVED_TOP_OFFSET - 1].data.numVal;
    unsigned restored_topsp = stack[topsp + SAVED_TOPSP_OFFSET - 1].data.numVal;
    unsigned return_pc = stack[topsp + SAVED_PC_OFFSET - 1].data.numVal;

    top = restored_top;
    topsp = restored_topsp;
    pc = return_pc;

    // Prevent re-entering the function
    if (pc < codeSize && code[pc].opcode == call_v) {
        ++pc;
    }

    if (topsp == 0) {
        // In case of infinite loop :)
        printf("[HALT] Returning from top-level. Halting.\n");
        executionFinished = 1;
    }
}

void execute_PARAM(instruction* instr) {
    
    if (top >= AVM_STACKSIZE) {
        fprintf(stderr, "[FATAL] Stack overflow at top=%u\n", top);
        executionFinished = 1;
        return;
    }
    avm_memcell temp;
    AVM_WIPEOUT(temp);

    avm_memcell* arg = avm_translate_operand(&instr->arg1, &temp);

    if (arg->type == string_m && arg->data.strVal != NULL) {
        //printf("[PARAM] pushing string: \"%s\" at top=%u\n", arg->data.strVal, top);
    }

    avm_assign(&stack[top], arg);
    ++top;
    ++totalActuals;
}

void execute_CALL(instruction* instr) {
    avm_memcell* func = avm_translate_operand(&instr->arg1, &ax);
    assert(func);

    avm_memcellclear(&retval);

    unsigned old_topsp = topsp; // save only for userfuncs
    avm_callsaveenvironment();

    switch (func->type) {
        case userfunc_m:
            pc = func->data.funcVal;
            assert(pc < codeSize);
            assert(code[pc].opcode == funcenter_v);
            top = topsp + AVM_STACKENV_SIZE; // locals start here
            break;

        case libfunc_m:
            insideLibfunc = 1;
            func->data.libfuncVal();
            insideLibfunc = 0;
            // Restore PC, top, topsp
            top = topsp;
            pc = stack[topsp + SAVED_PC_OFFSET - 1].data.numVal;
            topsp = stack[topsp + SAVED_TOPSP_OFFSET - 1].data.numVal;
            return;

        case string_m:
            avm_calllibfunc(func->data.strVal);
            top = topsp;
            pc = stack[topsp + SAVED_PC_OFFSET - 1].data.numVal;
            topsp = stack[topsp + SAVED_TOPSP_OFFSET - 1].data.numVal;
            return;

        default:
            fprintf(stderr, "[Error] call: cannot bind type %d to function\n", func->type);
            executionFinished = 1;
            return;
    }

    totalActuals = 0;
}

void execute_RETURN(instruction* instr) {
    printf("return\n");
    avm_memcell* retVal = avm_translate_operand(&instr->arg1, &ax);
    avm_assign(&retval, retVal);
    pc = stack[topsp + SAVED_PC_OFFSET - 1].data.numVal;
}

void execute_GETRETVAL(instruction* instr) {
    //printf("getretval\n");
    if (retval.type == undef_m) {
        retval.type = nil_m; // Default to nil
    }

    if (instr->result.type == nil_a) return;

    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    if (!lv) {
        fprintf(stderr, "[ERROR] GETRETVAL: NULL result memcell.\n");
        executionFinished = 1;
        return;
    }

    avm_memcellclear(lv);
    avm_assign(lv, &retval);
}

