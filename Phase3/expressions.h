#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H

#include "symtable.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef enum expr_t {
    var_e,
    tableitem_e,

    programfunc_e,
    libraryfunc_e,

    arithexpr_e,
    boolexpr_e,
    assignexpr_e,
    newtable_e,

    costnum_e,
    constbool_e,
    conststring_e,
    nil_e,
    function_address_e
}expr_t;

typedef struct ListNode {
    int quad_no;
    struct ListNode* next;
}ListNode;

typedef struct expr {
    expr_t type;
    Symbol* sym;
    struct expr* index;
    double numConst;
    char* strConst;
    unsigned char boolConst;
    struct expr* next;
	
    ListNode* truelist;
    ListNode* falselist;

    unsigned true_jump_quad;
    unsigned false_jump_quad;
    unsigned step_quad;
    unsigned jump_to_step;
    int not_flag;
}expr;

void print_list(ListNode* list);
expr* lvalue_expr(Symbol* sym);
expr* member_item (expr* lv, char* name/*, unsigned int scope, unsigned int line*/);
expr* newexpr(expr_t t);
expr* newexpr_conststring(char* s);
expr* newexpr_constnum(double i);
expr* newexpr_constbool (unsigned int b);
void check_arith(expr* e, const char* context);
expr* make_bool_if_needed(expr* e);
void check_if_bool(expr* e, const char* context);
unsigned int istempname (char* s);
unsigned int istempexpr (expr* e);
expr* emit_tablecreate(void);
void emit_tablesetelem(expr* t, expr* key, expr* value);
int get_list_head_quad(ListNode* l);
int get_list_element(ListNode* list, int index);
expr* make_member_access(expr* t, expr* i);
#endif