#ifndef EXEC_TABLE_H
#define EXEC_TABLE_H

#include "avm.h"

void execute_NEWTABLE(instruction* instr);
void execute_TABLEGETELEM(instruction* instr);
void execute_TABLESETELEM(instruction* instr);

#endif