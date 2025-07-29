#include <stdio.h>
#include "exec_arithm.h"
#include <assert.h>

extern unsigned top;
extern unsigned topsp;
extern avm_memcell stack[AVM_STACKSIZE];
extern avm_memcell retval;
extern unsigned char executionFinished;

extern avm_memcell ax, bx, cx;

void exec_arithm(instruction* instr, double (*op)(double, double)){
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);

    if (!lv || !rv1 || !rv2) {
        fprintf(stderr, "[Error] exec_arithm: NULL operand(s). result=%p arg1=%p arg2=%p\n", lv, rv1, rv2);
        executionFinished = 1;
        return;
    }

    if (rv1->type != number_m || rv2->type != number_m) {
        fprintf(stderr, "[Error] Arithmetic operands must be numbers!\n");
        executionFinished = 1;
        return;
    }

    avm_memcellclear(lv);
    lv->type = number_m;
    lv->data.numVal = op(rv1->data.numVal, rv2->data.numVal);
}

double add_op(double x, double y){return x+y;}
double sub_op(double x, double y){return x-y;}
double mul_op(double x, double y){return x*y;}
double div_op(double x, double y){
    if(y != 0){
        return x/y;
    }else{
        fprintf(stderr, "[Error] Division by zero!\n");
        executionFinished = 1;
        return 0;
    }
}
double mod_op(double x, double y){
    if(y != 0){
        return (int)x % (int)y;
    }else{
        fprintf(stderr, "[Error] Modulo by zero!\n");
        executionFinished = 1;
        return 0;
    }
}

void execute_ADD(instruction* instr){ exec_arithm(instr, add_op); }
void execute_SUB(instruction* instr){ exec_arithm(instr, sub_op); }
void execute_MUL(instruction* instr){ exec_arithm(instr, mul_op); }
void execute_DIV(instruction* instr){ exec_arithm(instr, div_op); }
void execute_MOD(instruction* instr){ exec_arithm(instr, mod_op); }

//void execute_UMINUS(instruction* instr){ assert(0);}

void execute_UMINUS(instruction* instr){
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    avm_memcell* rv = avm_translate_operand(&instr->arg1, &ax);

    if(rv->type != number_m){
        fprintf(stderr, "[Error] Uminus operand must be a number!\n");
        executionFinished = 1;
        return;
    }
    avm_memcellclear(lv);
    lv->type = number_m;
    lv->data.numVal = -rv->data.numVal;
 }

