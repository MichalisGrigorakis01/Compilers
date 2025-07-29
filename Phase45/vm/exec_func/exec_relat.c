#include <stdio.h>
#include "exec_relat.h"
#include <string.h>
#include <assert.h>

extern unsigned pc;
extern unsigned currLine;
extern unsigned char executionFinished;

extern avm_memcell ax, bx;

double avm_tonumber(avm_memcell* m) {
    switch (m->type) {
        case number_m: return m->data.numVal;
        case bool_m:   return m->data.boolVal ? 1.0 : 0.0;
        case nil_m:    return 0.0;
        case string_m: return atof(m->data.strVal);
        default:
            fprintf(stderr, "[Runtime Error] Cannot convert type %d to number at line %u\n", m->type, currLine);
            executionFinished = 1;
            return 0;
    }
}

void exec_relat(instruction* instr, int (*cmp)(double, double)) {
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);

    if (rv1->type == undef_m || rv2->type == undef_m) {
        fprintf(stderr, "[Error] Relational operand is 'undef'.\n");
        executionFinished = 1;
        return;
    }

    int result = 0;

    if (rv1->type == table_m && rv2->type == table_m) {
        result = (rv1->data.tableVal == rv2->data.tableVal);
    } else if (rv1->type == number_m || rv2->type == number_m ||
            rv1->type == bool_m   || rv2->type == bool_m   ||
            rv1->type == string_m || rv2->type == string_m ||
            rv1->type == nil_m    || rv2->type == nil_m) {
        double v1 = avm_tonumber(rv1);
        double v2 = avm_tonumber(rv2);
        result = cmp(v1, v2);
    } else {
        fprintf(stderr, "[Runtime Error] Unsupported types (%d, %d) for relational operation at line %u\n",
                rv1->type, rv2->type, currLine);
        executionFinished = 1;
        return;
    }

    if (instr->result.type == label_a) {
        if (result) pc = instr->result.val;
    } else {
        avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
        if (!lv) {
            fprintf(stderr, "[Error] Cannot assign result of relational expression. Line %u\n", currLine);
            executionFinished = 1;
            return;
        }
        avm_memcellclear(lv);
        lv->type = bool_m;
        lv->data.boolVal = result;
    }
}

int eq_op(double x, double y){ return x==y; }
int neq_op(double x, double y){ return x!=y; }
int lt_op(double x, double y){ return x < y; }
int gt_op(double x, double y){ return x > y; }
int le_op(double x, double y){ return x <= y; }
int ge_op(double x, double y){ return x >= y; }

void execute_IF_EQ(instruction* instr){ exec_relat(instr, eq_op); }
void execute_IF_NOTEQ(instruction* instr){ exec_relat(instr, neq_op); }
void execute_IF_LESSEQ(instruction* instr){ exec_relat(instr, le_op); }
void execute_IF_GREATEREQ(instruction* instr){ exec_relat(instr, ge_op); }
void execute_IF_LESS(instruction* instr){ exec_relat(instr, lt_op); }
void execute_IF_GREATER(instruction* instr){ exec_relat(instr, gt_op); }

/* Ignore those for some reason*/
/*
void execute_AND(instruction* instr){assert(0);}
void execute_OR(instruction* instr){assert(0);}
void execute_NOT(instruction* instr){assert(0);}
*/

