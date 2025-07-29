#ifndef QUADS_H
#define QUADS_H

#include "expressions.h"

#define EXPAND_SIZE 1024
#define CURR_SIZE (total*sizeof(quad))
#define NEW_SIZE (EXPAND_SIZE*sizeof(quad)+CURR_SIZE)

typedef enum iopcode {
    assign, add, sub,
    mul,    div_, mod_,
    uminus, and_, or_,
    not_,    if_eq, if_noteq,
    if_lesseq, if_greatereq, if_less,
    if_greater, call_, param,
    ret, getretval, funcstart,
    funcend, tablecreate, tablegetelem,
    tablesetelem, jump
}iopcode;

typedef struct quad {
    iopcode op;
    expr* result;
    expr* arg1;
    expr* arg2;
    unsigned label;
    unsigned line;
} quad;



char* newtempname();
void resettemp();
void expand(void);
void emit(enum iopcode op, expr* arg1, expr* arg2, expr* result, unsigned label, unsigned line);
expr* emit_iftableitem(expr* e);
expr* make_call(expr* lv, expr* reversed_elist);

unsigned nextquad(void);
void patchlabel(unsigned quadNo, unsigned label);

typedef struct stmt_t {
    ListNode* breakList;
    ListNode* contList;
    unsigned nextlist;
    unsigned quad_start;
}stmt_t;

stmt_t* new_stmt();

void make_stmt(stmt_t* s);
ListNode* new_list(int quad);
ListNode* mergelist(ListNode* l1, ListNode* l2);
void patchlist(ListNode* list,int label);
Symbol* newtemp(void);
void emit_relop(enum iopcode op, expr* e1, expr* e2, expr* result, unsigned lineno);
void print_quads(void);
expr* handle_inc(expr* e);
expr* handle_dec(expr* e);
expr* emit_and(expr* e1, expr* e2, int* really_useful_index_ptr);
expr* emit_or(expr* e1, expr* e2, int* really_useful_index_ptr);
expr* emit_not(expr* e);
expr* get_result(void);
void patch_all_jumps();

#endif