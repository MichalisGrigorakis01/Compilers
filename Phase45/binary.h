#ifndef BINARY_H
#define BINARY_H

#include <stdio.h>

#define MAGIC_NUMBER 173985693

void write_magic_number(FILE* f);
void write_string_array(FILE* f);
void write_number_array(FILE* f);
void write_libfuncs(FILE* f);
void write_userfuncs(FILE* f);
void write_instructions(FILE* f);
void generate_binary(const char* filename);

#endif
