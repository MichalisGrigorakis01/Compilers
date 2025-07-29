#ifndef SYMTABLE_H
#define SYMTABLE_H


typedef enum scopespace_t {
    programvar,
    functionlocal,
    formalarg
} scopespace_t;

typedef enum symbol_t {
    var_s,
    programfunc_s,
    libraryfunc_s
} symbol_t;

typedef struct Symbol {
    symbol_t type;
    char* name;
    scopespace_t space;
    unsigned int offset;
    unsigned int scope;
    unsigned int line;
    int isFunction;
    int enabled;
    struct Symbol* snext;
}Symbol;

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

Symbol* extractSymbol(symTable* entry);

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

scopespace_t currscopespace(void);
unsigned int currscopeoffset(void);
void inccurrscopeoffset(void);
void enterscopespace(void);
void exitscopespace(void);
void resetformalargsoffset(void);
void resetfunctionlocalsoffset(void);
void restorecurrscopeoffset(unsigned n);


#endif  // EXAMPLE_H