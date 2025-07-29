#ifndef EXEC_BOOLEAN_H
#define EXEC_BOOLEAN_H

#include "avm.h"

unsigned char trans_to_bool(avm_memcell* m);

void execute_AND(instruction* instr);
void execute_OR(instruction* instr);
void execute_NOT(instruction* instr);

#endif