#ifndef EXAMPLE_H
#define EXAMPLE_H

typedef struct symTable// List for the Hash Table
{
    int isFunction;
    char *name;
    char *type;
    char *args;
    int scope;
    int line;
    int visible;
    int enabled;
    struct symTable *snext;
} symTable;

extern int stsize;
extern int scope;
extern symTable* table;


void symPrint(symTable *table);

symTable* symSearch(symTable *table, char *name, char *type, int scope);

int get_enabled(symTable *table, char *name, char *type, int scope);

int symDelete(symTable *table, char* name,char* type, int scope,int line);

void change_enabled_atscope(symTable *table, int scope,int enable);

int is_outer_function_scope(symTable *table, char *name, int current_scope);

symTable *symInsert(symTable *table, char *name, char *type, char *args, int line, int scope, int isFunction);

/*looks for a temp variable, returns it if found */
symTable* temp_var_search(symTable *table,char *name,unsigned int scope);

symTable *scope_lookup(symTable *table,char *name,unsigned int scope);

typedef struct Scopelist{
    symTable *entry;
    struct Scopelist *next;
}Scopelist;

#endif  // EXAMPLE_H
