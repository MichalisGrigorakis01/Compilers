#ifndef EXEC_LIBFUNC_H
#define EXEC_LIBFUNC_H

#include "../avm.h"

typedef void (*library_func_t)(void);

extern library_func_t libfuncs[256];
extern char* libnames[256];
extern int libfunc_index;

void avm_registerlibfunc(const char* id, library_func_t func);
void avm_calllibfunc(char* id);

void libfunc_print(void);
void libfunc_typeof(void);
void libfunc_totalarguments(void);
void libfunc_argument(void);

static void libfunc_input(void);
static void libfunc_objectmemberkeys(void);
static void libfunc_objecttotalmembers(void);
static void libfunc_objectcopy(void);
static void libfunc_strtonum(void);
static void libfunc_sqrt(void);
static void libfunc_cos(void);
static void libfunc_sin(void);
void avm_register_default_libfuncs(void);

#endif