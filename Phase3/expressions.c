#include <stdio.h>
#include "symtable.h"
#include "expressions.h"
#include "quads.h"

extern int yylineno;
extern quad* quads;
extern expr* last_result;

expr* lvalue_expr(Symbol* sym){
    assert(sym);
    expr* e = (expr*)malloc(sizeof(expr));
    memset(e,0,sizeof(expr));

    e->next = (expr*)0;
    e->sym = sym;

    switch(sym->type){
        case var_s : e->type = var_e; break;
        case programfunc_s : e->type = programfunc_e; break;
        case libraryfunc_s : e->type = libraryfunc_e; break;
        default: assert(0);
    }
    return e;
}

expr* member_item (expr* lv, char* name/*, unsigned int scope, unsigned int line*/) {
    if (!lv) {
        fprintf(stderr, "[member_item] ERROR: Left-hand expression is NULL when accessing member '%s' (line %d)\n", name, yylineno);
        expr* temp = newexpr(tableitem_e);
        temp->sym = newtemp();
        temp->index = newexpr_conststring(name);
        return temp;
    }
    lv = emit_iftableitem(lv/*,scope,line*/); // Emit code if r-value use of table item
    expr* ti = newexpr(tableitem_e); // Make a new expression
    ti->sym = lv->sym;
    ti->index = newexpr_conststring(name); // Const string index
    return ti;
}

expr* newexpr(expr_t t){
    expr* e = (expr*)malloc(sizeof(expr));
    memset(e,0,sizeof(expr));
    e->type = t;
    e->next = (expr*)0; // i added that
    return e;
}

expr* newexpr_conststring(char* s){
    expr* e = newexpr(conststring_e);
    e->strConst = strdup(s);
    return e;
}

expr* newexpr_constnum(double i){
    expr* e = newexpr(costnum_e);
    e->numConst = i;
    return e;
}

expr* newexpr_constbool (unsigned int b) {
    expr* e = newexpr(constbool_e);
    e->boolConst = !!b; //idk wtf is this. Savvidis had it
    return e;
}


void check_arith(expr* e, const char* context){
    if ( e->type == constbool_e ||
        e->type == conststring_e ||
        e->type == nil_e ||
        e->type == newtable_e ||
        e->type == programfunc_e ||
        e->type == libraryfunc_e ||
        e->type == boolexpr_e ){
            fprintf(stderr,"Invalid use of expression in %s in line %d!\n\n",context, yylineno);
            exit(0);
        }     
}

int get_list_head_quad(ListNode* l) {
    return l ? l->quad_no : -1; // -1 -> invalid list
}


expr* make_bool_if_needed(expr* e) {
    if (!e) return NULL;

    if (e->type == boolexpr_e && e->truelist && e->falselist)
        return e;

    if (e->type == var_e || e->type == constbool_e) {
        expr* result = newexpr(boolexpr_e);

        emit(if_eq, e, newexpr_constbool(1), NULL, nextquad() + 2, yylineno);
        emit(jump, NULL, NULL, NULL, nextquad() + 3, yylineno);

        result->truelist = new_list(nextquad() - 2);
        result->falselist = new_list(nextquad() - 1);
        last_result = result;
        return result;
    }

    expr* result = newexpr(boolexpr_e);

    emit(if_eq, e, newexpr_constbool(1), NULL, nextquad() + 2, yylineno);
    emit(jump, NULL, NULL, NULL, nextquad() + 3, yylineno);

    result->truelist = new_list(nextquad() - 2);
    result->falselist = new_list(nextquad() - 1);
    last_result = result;
    return result;
}

expr* make_member_access(expr* table, expr* i) {
    table = emit_iftableitem(table);
    expr* t = newexpr(tableitem_e);
    t->sym = table->sym;
    t->index = i;
    return t;
}

void check_if_bool(expr* e, const char* context) {
    if (
        e->type == conststring_e ||
        e->type == nil_e ||
        e->type == var_e ||
        e->type == arithexpr_e ||
        e->type == tableitem_e ||
        e->type == newtable_e
    ) {
        fprintf(stderr, "Invalid use of non-boolean expression in %s at line %d!\n", context, yylineno);
        exit(1);
    }
}

unsigned int istempname (char* s) {
    return *s == '_';
}

unsigned int istempexpr (expr* e) {
    return e->sym && istempname(e->sym->name);
}

expr* emit_tablecreate(void) {
    expr* t = newexpr(newtable_e);
    t->sym = newtemp();
    emit(tablecreate, NULL, NULL, t, 0, yylineno);
    return t;
}

void emit_tablesetelem(expr* t, expr* key, expr* value) {
    assert(t && key && value);
    emit(tablesetelem, key, value, t, 0, yylineno);
    
}


