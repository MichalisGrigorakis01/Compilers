#include <stdio.h>
#include "exec_libfunc.h"
#include <stdint.h>

extern unsigned char executionFinished;
library_func_t libfuncs[256] = { 0 };
char* libnames[256] = { 0 };
int libfunc_index = 0;
extern unsigned totalActuals;

extern unsigned top, topsp;
extern avm_memcell retval;

unsigned avm_totalactuals(void) {
    if (topsp < 4) {
        fprintf(stderr, "[FATAL] topsp too low in avm_totalactuals (topsp=%u)\n", topsp);
        exit(1);
    }
    return stack[topsp + 3].data.numVal;
}

avm_memcell* avm_getactual(unsigned i) {
    unsigned total = avm_totalactuals();
    assert(topsp >= total);
    assert(topsp - total + i < AVM_STACKSIZE);
    assert(i < total);
    assert(stack[topsp - total + i].type != undef_m);
    return &stack[topsp - total + i];
}

//char* avm_tostring(avm_memcell* m); // previously defined

void avm_registerlibfunc(const char* id, library_func_t func) {
    if (!id || libfunc_index >= 256) {
        fprintf(stderr, "[ERROR] Invalid libfunc registration: '%s'\n", id ? id : "NULL");
        return;
    }

    libnames[libfunc_index] = strdup(id); // Ensure we own the string memory
    libfuncs[libfunc_index] = func;

    //printf("[REGISTER] libfuncs[%d] = %p (id = %s)\n", libfunc_index, (void*)func, libnames[libfunc_index]);
    libfunc_index++;
}


void avm_calllibfunc(char* id) {
    assert(libfunc_index < 256);

    for (int i = 0; i < libfunc_index; ++i) {
        if (!libfuncs[i]) {
            fprintf(stderr, "[FATAL] libfunc '%s' has NULL pointer!\n", id);
            executionFinished = 1;
            return;
        }
        
        if (strcmp(id, libnames[i]) == 0) {
            if (!libfuncs[i]) {
                fprintf(stderr, "[FATAL] libfunc '%s' has NULL pointer!\n", id);
                executionFinished = 1;
                return;
            }

            printf("[DEBUG] Calling %s at %p\n", id, (void*)libfuncs[i]);
            libfuncs[i]();
            return;
        }
    }

    fprintf(stderr, "[FATAL] libfunc '%s' not found in registry\n", id);
    executionFinished = 1;
}

// print
void libfunc_print(void) {

    unsigned n = avm_totalactuals();
    for (unsigned i = 0; i < n; ++i) {
        avm_memcell* arg = avm_getactual(i);
        if (arg->type == undef_m) {
            printf("undef");
            continue;
        }
        char* s = avm_tostring(arg);

        if (arg->type == string_m) {
            // Handle special strings
            if (strcmp(s, "\\n") == 0 || strcmp(s, "\n") == 0) {
                putchar('\n');
            } else if (strcmp(s, " ") == 0) {
                putchar(' ');
            } else {
                fputs(s, stdout);
            }
        } else {
            fputs(s, stdout);
        }

        if (i + 1 < n) {
            avm_memcell* next = avm_getactual(i + 1);
            if (!(next->type == string_m && (strcmp(next->data.strVal, "\n") == 0 || strcmp(next->data.strVal, "\\n") == 0))) {
                putchar(' ');
            }
        }
        //Remove this if you add new lines in tests
        //printf("\n"); 
        free(s);
    }

    avm_memcellclear(&retval);
    retval.type = nil_m;

    // Clear actuals from the stack
    for (unsigned i = 0; i < totalActuals; ++i) {
        avm_memcellclear(&stack[top - totalActuals + i]);
    }
    //top -= totalActuals;
    if (top >= totalActuals)
        top -= totalActuals;
    else {
        fprintf(stderr, "[BUG] libfunc_print: attempt to underflow stack\n");
        executionFinished = 1;
        return;
    }

    totalActuals = 0;
}

// typeof
void libfunc_typeof(void) {
    if (avm_totalactuals() != 1) {
        fprintf(stderr, "[Error] typeof() takes 1 argument\n");
        executionFinished = 1;
        return;
    }

    avm_memcell* arg = avm_getactual(0);
    avm_memcellclear(&retval);
    retval.type = string_m;

    const char* type_str = NULL;

    switch (arg->type) {
        case number_m:    type_str = "number"; break;
        case string_m:    type_str = "string"; break;
        case bool_m:      type_str = "boolean"; break;
        case table_m:     type_str = "table"; break;
        case userfunc_m:  type_str = "userfunction"; break;
        case libfunc_m:   type_str = "libraryfunction"; break;
        case nil_m:       type_str = "nil"; break;
        case undef_m:     type_str = "undefined"; break;
        default:          type_str = "unknown"; break;
    }

    retval.data.strVal = strdup(type_str);
}

// totalarguments
void libfunc_totalarguments(void) {
    unsigned old_topsp = stack[topsp - 2].data.numVal;
    if (old_topsp == 0) {
        avm_memcellclear(&retval);
        retval.type = nil_m;
        return;
    }

    avm_memcellclear(&retval);
    retval.type = number_m;
    retval.data.numVal = stack[old_topsp - 4].data.numVal;
}

// argument(i)
void libfunc_argument(void) {
    if (avm_totalactuals() != 1) {
        fprintf(stderr, "[Error] argument(i) takes exactly 1 argument\n");
        executionFinished = 1;
        return;
    }

    avm_memcell* arg = avm_getactual(0);
    if (arg->type != number_m) {
        fprintf(stderr, "[Error] argument(i): expected number\n");
        executionFinished = 1;
        return;
    }

    unsigned old_topsp = stack[topsp - 2].data.numVal;
    if (old_topsp == 0) {
        avm_memcellclear(&retval);
        retval.type = nil_m;
        return;
    }

    unsigned index = (unsigned)arg->data.numVal;
    avm_memcellclear(&retval);
    avm_assign(&retval, &stack[old_topsp + AVM_STACKENV_SIZE + 1 + index]);
}

static void libfunc_input(void) {
    char buffer[1024];
    avm_memcellclear(&retval);

    if (!fgets(buffer, sizeof(buffer), stdin)) {
        retval.type = string_m;
        retval.data.strVal = strdup("");
        return;
    }

    buffer[strcspn(buffer, "\n")] = 0;

    if (strcmp(buffer, "true") == 0) {
        retval.type = bool_m;
        retval.data.boolVal = 1;
    } else if (strcmp(buffer, "false") == 0) {
        retval.type = bool_m;
        retval.data.boolVal = 0;
    } else if (strcmp(buffer, "nil") == 0) {
        retval.type = nil_m;
    } else {
        // try to parse as number
        char* end = NULL;
        double num = strtod(buffer, &end);
        if (end && *end == '\0') {
            retval.type = number_m;
            retval.data.numVal = num;
        } else {
            retval.type = string_m;
            retval.data.strVal = strdup(buffer);
        }
    }
}

static void libfunc_objectmemberkeys(void) {
    if (avm_totalactuals() != 1) {
        fprintf(stderr, "[ERROR] objectmemberkeys(obj) expects 1 argument\n");
        executionFinished = 1;
        return;
    }

    avm_memcell* arg = avm_getactual(0);
    if (arg->type != table_m) {
        fprintf(stderr, "[ERROR] objectmemberkeys() expects a table\n");
        executionFinished = 1;
        return;
    }

    avm_table* t = arg->data.tableVal;
    avm_table* arr = avm_tablenew();
    unsigned count = 0;

    for (unsigned i = 0; i < AVM_TABLE_HASHSIZE; ++i) {
        for (avm_table_bucket* b = t->strIndexed[i]; b; b = b->next) {
            avm_memcell key, val;
            key.type = number_m;
            key.data.numVal = count++;

            val.type = string_m;
            val.data.strVal = strdup(b->key.data.strVal);

            avm_tablesetelem(arr, &key, &val);
        }
        for (avm_table_bucket* b = t->numIndexed[i]; b; b = b->next) {
            avm_memcell key, val;
            key.type = number_m;
            key.data.numVal = count++;

            val.type = number_m;
            val.data.numVal = b->key.data.numVal;

            avm_tablesetelem(arr, &key, &val);
        }
    }

    avm_memcellclear(&retval);
    retval.type = table_m;
    retval.data.tableVal = arr;
    avm_tableincreafcounter(arr);
}

static void libfunc_objecttotalmembers(void) {
    if (avm_totalactuals() != 1) {
        fprintf(stderr, "[ERROR] objecttotalmembers(obj) expects 1 argument\n");
        executionFinished = 1;
        return;
    }

    avm_memcell* arg = avm_getactual(0);
    if (arg->type != table_m) {
        fprintf(stderr, "[ERROR] objecttotalmembers() expects a table\n");
        executionFinished = 1;
        return;
    }

    avm_memcellclear(&retval);
    retval.type = number_m;
    retval.data.numVal = arg->data.tableVal->total;
}

static void libfunc_objectcopy(void) {
    if (avm_totalactuals() != 1) {
        fprintf(stderr, "[ERROR] objectcopy(obj) expects 1 argument\n");
        executionFinished = 1;
        return;
    }

    avm_memcell* arg = avm_getactual(0);
    if (arg->type != table_m) {
        fprintf(stderr, "[ERROR] objectcopy() expects a table\n");
        executionFinished = 1;
        return;
    }

    avm_table* orig = arg->data.tableVal;
    avm_table* copy = avm_tablenew();

    for (unsigned i = 0; i < AVM_TABLE_HASHSIZE; ++i) {
        for (avm_table_bucket* b = orig->strIndexed[i]; b; b = b->next) {
            avm_memcell k, v;
            k.type = string_m;
            k.data.strVal = strdup(b->key.data.strVal);
            avm_assign(&v, &b->value);
            avm_tablesetelem(copy, &k, &v);
        }
        for (avm_table_bucket* b = orig->numIndexed[i]; b; b = b->next) {
            avm_memcell k, v;
            k.type = number_m;
            k.data.numVal = b->key.data.numVal;
            avm_assign(&v, &b->value);
            avm_tablesetelem(copy, &k, &v);
        }
    }

    avm_memcellclear(&retval);
    retval.type = table_m;
    retval.data.tableVal = copy;
    avm_tableincreafcounter(copy);
}

static void libfunc_strtonum(void) {
    if (avm_totalactuals() != 1) {
        fprintf(stderr, "[ERROR] strtonum(s) expects 1 argument\n");
        executionFinished = 1;
        return;
    }

    avm_memcell* arg = avm_getactual(0);
    avm_memcellclear(&retval);

    if (arg->type != string_m) {
        retval.type = nil_m;
        return;
    }

    char* end;
    double val = strtod(arg->data.strVal, &end);
    if (end == arg->data.strVal || *end != '\0') {
        retval.type = nil_m;
    } else {
        retval.type = number_m;
        retval.data.numVal = val;
    }
}

static void libfunc_sqrt(void) {
    if (avm_totalactuals() != 1) {
        fprintf(stderr, "[ERROR] sqrt(n) expects 1 argument\n");
        executionFinished = 1;
        return;
    }

    avm_memcell* arg = avm_getactual(0);
    avm_memcellclear(&retval);

    if (arg->type != number_m || arg->data.numVal < 0) {
        retval.type = nil_m;
        return;
    }

    retval.type = number_m;
    retval.data.numVal = sqrt(arg->data.numVal);
}

static void libfunc_cos(void) {
    if (avm_totalactuals() != 1) {
        fprintf(stderr, "[ERROR] cos(n) expects 1 argument\n");
        executionFinished = 1;
        return;
    }

    avm_memcell* arg = avm_getactual(0);
    avm_memcellclear(&retval);

    if (arg->type != number_m) {
        retval.type = nil_m;
        return;
    }

    retval.type = number_m;
    retval.data.numVal = cos(arg->data.numVal * M_PI / 180.0);
}

static void libfunc_sin(void) {
    if (avm_totalactuals() != 1) {
        fprintf(stderr, "[ERROR] sin(n) expects 1 argument\n");
        executionFinished = 1;
        return;
    }

    avm_memcell* arg = avm_getactual(0);
    avm_memcellclear(&retval);

    if (arg->type != number_m) {
        retval.type = nil_m;
        return;
    }

    retval.type = number_m;
    retval.data.numVal = sin(arg->data.numVal * M_PI / 180.0);
}

void avm_register_default_libfuncs(void) {
    avm_registerlibfunc("print", libfunc_print);
    avm_registerlibfunc("input", libfunc_input);
    avm_registerlibfunc("objectmemberkeys", libfunc_objectmemberkeys);
    avm_registerlibfunc("objecttotalmembers", libfunc_objecttotalmembers);
    avm_registerlibfunc("objectcopy", libfunc_objectcopy);
    avm_registerlibfunc("totalarguments", libfunc_totalarguments);
    avm_registerlibfunc("argument", libfunc_argument);
    avm_registerlibfunc("typeof", libfunc_typeof);
    avm_registerlibfunc("strtonum", libfunc_strtonum);
    avm_registerlibfunc("sqrt", libfunc_sqrt);
    avm_registerlibfunc("cos", libfunc_cos);
    avm_registerlibfunc("sin", libfunc_sin);
}