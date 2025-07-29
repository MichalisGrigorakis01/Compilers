#ifndef EXEC_RELAT_H
#define EXEC_RELAT_H
#include "avm.h"

double avm_tonumber(avm_memcell* m);
void exec_relat(instruction* instr, int (*cmp)(double, double));

int eq_op(double x, double y);
int neq_op(double x, double y);
int lt_op(double x, double y);
int gt_op(double x, double y);
int le_op(double x, double y);
int ge_op(double x, double y);

void execute_IF_EQ(instruction* instr);
void execute_IF_NOTEQ(instruction* instr);
void execute_IF_LESSEQ(instruction* instr);
void execute_IF_GREATEREQ(instruction* instr);
void execute_IF_LESS(instruction* instr);
void execute_IF_GREATER(instruction* instr);

/* Ignore those for some reason*/
//void execute_AND(instruction* instr);
//void execute_OR(instruction* instr);
//void execute_NOT(instruction* instr);

#endif