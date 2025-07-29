#ifndef EXEC_ARITHM_H
#define EXEC_ARITHM_H
#include "avm.h"

void exec_arithm(instruction* instr, double (*op)(double, double));

double add_op(double x, double y);
double sub_op(double x, double y);
double mul_op(double x, double y);
double div_op(double x, double y);
double mod_op(double x, double y);

void execute_ADD(instruction*);
void execute_SUB(instruction*);
void execute_MUL(instruction*);
void execute_DIV(instruction*);
void execute_MOD(instruction*);
void execute_UMINUS(instruction*);

void execute_ADD(instruction* instr);
void execute_SUB(instruction* instr);
void execute_MUL(instruction* instr);
void execute_DIV(instruction* instr);
void execute_MOD(instruction* instr);
void execute_UMINUS(instruction* instr);

#endif
