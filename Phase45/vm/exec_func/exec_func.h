#ifndef EXEC_FUNC_H 
#define EXEC_FUNC_H 

#include "exec_libfunc.h"

typedef enum env_offset{
     SAVED_TOP_OFFSET = 1,
    SAVED_TOPSP_OFFSET = 2,
    SAVED_PC_OFFSET = 3,
    SAVED_TOTALACTUALS_OFFSET = 4
}env_offset;

void avm_callsaveenvironment(void);

void execute_CALL(instruction* instr);
void execute_FUNCSTART(instruction* instr);
void execute_FUNCEND(instruction* instr);
void execute_PARAM(instruction* instr);
void execute_GETRETVAL(instruction* instr);
void execute_RETURN(instruction* instr);

#endif