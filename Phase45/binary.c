#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "generate.h"
#include "binary.h"

void write_magic(FILE* f) {
    unsigned magic = MAGIC_NUMBER;
    fwrite(&magic, sizeof(unsigned), 1, f);
}

void write_strings(FILE* f) {
    fwrite(&totalStringConsts, sizeof(unsigned), 1, f);
    for (unsigned i = 0; i < totalStringConsts; ++i) {
        //printf("stringConsts[%u] = %s\n", i, stringConsts[i]);
        unsigned len = strlen(stringConsts[i]);
        fwrite(&len, sizeof(unsigned), 1, f);
        fwrite(stringConsts[i], sizeof(char), len, f);
    }
}

void write_numbers(FILE* f) {
    fwrite(&totalNumConsts, sizeof(unsigned), 1, f);
    for (unsigned i = 0; i < totalNumConsts; ++i) {
        fwrite(&cumConsts[i], sizeof(double), 1, f);
    }
}

void write_libfuncs(FILE* f) {
    fwrite(&totalNamedLibfuncs, sizeof(unsigned), 1, f);
    for (unsigned i = 0; i < totalNamedLibfuncs; ++i) {
        unsigned len = strlen(namedLibfuncs[i]);
        fwrite(&len, sizeof(unsigned), 1, f);
        fwrite(namedLibfuncs[i], sizeof(char), len, f);
    }
}

void write_userfuncs(FILE* f) {
    fwrite(&totalUserFuncs, sizeof(unsigned), 1, f);
    for (unsigned i = 0; i < totalUserFuncs; ++i) {
        fwrite(&userFuncs[i].address, sizeof(unsigned), 1, f);
        fwrite(&userFuncs[i].localSize, sizeof(unsigned), 1, f);

        unsigned len = strlen(userFuncs[i].id);
        fwrite(&len, sizeof(unsigned), 1, f);
        fwrite(userFuncs[i].id, sizeof(char), len, f);
    }
}

void write_instrs(FILE* f) {
    fwrite(&currInstr, sizeof(unsigned), 1, f);
    for (unsigned i = 0; i < currInstr; ++i) {
        instruction* instr = &instructions[i];

        fwrite(&instr->opcode, sizeof(unsigned), 1, f);

        unsigned char type;

        type = (unsigned char)instr->result.type;
        fwrite(&type, sizeof(unsigned char), 1, f);
        fwrite(&instr->result.val, sizeof(unsigned), 1, f);

        type = (unsigned char)instr->arg1.type;
        fwrite(&type, sizeof(unsigned char), 1, f);
        fwrite(&instr->arg1.val, sizeof(unsigned), 1, f);

        type = (unsigned char)instr->arg2.type;
        fwrite(&type, sizeof(unsigned char), 1, f);
        fwrite(&instr->arg2.val, sizeof(unsigned), 1, f);

        fwrite(&instr->srcLine, sizeof(unsigned), 1, f);
    }
}

void generate_binary(const char* filename) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        perror("Failed to open binary output file");
        exit(1);
    }
    
    for (unsigned i = 0; i < currInstr; ++i) {
        //printf("INSTR %u: opcode = %d\n", i, instructions[i].opcode);
    }
    
    write_magic(f);
    write_strings(f);
    write_numbers(f);
    write_libfuncs(f);
    write_userfuncs(f);
    write_instrs(f);

    fclose(f);
    printf("\n\nBinary file written to: %s\n", filename);
}