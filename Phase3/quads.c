#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quads.h"


extern FILE* quad_output;
extern int yylineno;
static unsigned tempVarCounter = 0;
quad* quads = NULL;
unsigned total = 0;
unsigned currQuad = 0;
expr* last_result;
extern int not_flag;
extern int and_or_indicator;

int short_circuit_mode = 0;

char* newtempname() {
    char* name = (char*)malloc(10);
    sprintf(name, "_t%d", tempVarCounter++);
    return name;
}

void resettemp() {
    tempVarCounter = 0;
}

Symbol* newtemp(void) {
    char name[64];
    snprintf(name, sizeof(name), "_t%u", tempVarCounter++);

    Symbol* sym = (Symbol*)malloc(sizeof(Symbol));
    if (!sym) {
        fprintf(stderr, "newtemp: memory allocation failed\n");
        exit(1);
    }

    sym->type = var_s;
    sym->name = strdup(name);
    sym->space = currscopespace();        
    sym->offset = currscopeoffset();      
    sym->scope = scope;                   
    sym->line = yylineno;
    sym->isFunction = 0;
    sym->enabled = 1;
    sym->snext = NULL;

    inccurrscopeoffset(); 
    return sym;
}


const char* iopcode_to_str(enum iopcode op) {
    switch (op) {
        case assign: return "assign";
        case add: return "add";
        case sub: return "sub";
        case mul: return "mul";
        case div_: return "div";
        case mod_: return "mod";
        case uminus: return "uminus";
        case and_: return "and";
        case or_: return "or";
        case not_: return "not";
        case if_eq: return "if_eq";
        case if_noteq: return "if_noteq";
        case if_lesseq: return "if_lesseq";
        case if_greatereq: return "if_greatereq";
        case if_less: return "if_less";
        case if_greater: return "if_greater";
        case call_: return "call";
        case param: return "param";
        case ret: return "ret";
        case getretval: return "getretval";
        case funcstart: return "funcstart";
        case funcend: return "funcend";
        case tablecreate: return "tablecreate";
        case tablegetelem: return "tablegetelem";
        case tablesetelem: return "tablesetelem";
        case jump: return "jump";
        default: return "unknown_op";
    }
}

void print_quads(void) {
    printf("\n%-6s %-15s %-12s %-12s %-12s %-6s\n", "quad#", "opcode", "result", "arg1", "arg2", "label");
    fprintf(quad_output,"\n%-6s %-15s %-12s %-12s %-12s %-6s\n", "quad#", "opcode", "result", "arg1", "arg2", "label");
    printf("---------------------------------------------------------------\n");
    fprintf(quad_output,"---------------------------------------------------------------\n");

    for (unsigned i = 1; i < currQuad; ++i) {
        quad q = quads[i];

        char result[32] = "-", arg1[32] = "-", arg2[32] = "-";

        if (q.result) {
            if (q.result->sym && q.result->sym->name)
                strcpy(result, q.result->sym->name);
            else if (q.result->type == constbool_e)
                strcpy(result, q.result->boolConst ? "true" : "false");
            else if (q.result->type == conststring_e && q.result->strConst)
                strcpy(result, q.result->strConst);
            else if (q.result->type == costnum_e)
                sprintf(result, "%g", q.result->numConst);
        }

        if (q.arg1) {
            if (q.arg1->sym && q.arg1->sym->name)
                strcpy(arg1, q.arg1->sym->name);
            else if (q.arg1->type == constbool_e)
                strcpy(arg1, q.arg1->boolConst ? "true" : "false");
            else if (q.arg1->type == conststring_e)
                strcpy(arg1, q.arg1->strConst);
            else if (q.arg1->type == costnum_e)
                sprintf(arg1, "%g", q.arg1->numConst);
        }

        if (q.arg2) {
            if (q.arg2->sym && q.arg2->sym->name)
                strcpy(arg2, q.arg2->sym->name);
            else if (q.arg2->type == constbool_e)
                strcpy(arg2, q.arg2->boolConst ? "true" : "false");
            else if (q.arg2->type == conststring_e)
                strcpy(arg2, q.arg2->strConst);
            else if (q.arg2->type == costnum_e)
                sprintf(arg2, "%g", q.arg2->numConst);
        }

                printf("%-6u %-15s %-12s %-12s %-12s %-6u\n",
               i, iopcode_to_str(q.op), result, arg1, arg2, q.label);
        
        fprintf(quad_output,"%-6u %-15s %-12s %-12s %-12s %-6u\n",
               i, iopcode_to_str(q.op), result, arg1, arg2, q.label);
    }

    printf("---------------------------------------------------------------\n\n");
    fprintf(quad_output,"---------------------------------------------------------------\n\n");
}



void expand(void){
    assert(total==currQuad);
    quad* p = (quad*)malloc(NEW_SIZE);
    if(quads){
        memcpy(p,quads,CURR_SIZE);
        free(quads);
    }
    quads = p;
    total += EXPAND_SIZE;
}

void emit(enum iopcode op, expr* arg1, expr* arg2, expr* result, unsigned label, unsigned line){
    if(currQuad == total){
        expand();
    }
    quad* p = quads+currQuad++;
    p->op = op;
    p->arg1 = arg1;
    p->arg2 = arg2;
    p->result = result;
    p->label = label;
    p->line = line;
}

expr* emit_iftableitem(expr* e) {
    if(e->type != tableitem_e){
        return e;
    }else{
        expr* result =  newexpr(var_e);
        result->sym = newtemp();
        emit(
            tablegetelem,
            e,
            e->index,
            result,
            0,
            yylineno
        );
        return result;
    }
}

expr* make_call(expr* lv, expr* reversed_elist){
    //assert(reversed_elist != NULL);
    expr* func = emit_iftableitem(lv);
    
    while (reversed_elist) {
        emit(param, reversed_elist, NULL, NULL,0,yylineno);
        reversed_elist = reversed_elist->next;
    }
    emit(call_, func,NULL,NULL ,0,yylineno);
    
    expr* result;
    if (lv && lv->type == tableitem_e && lv->sym != NULL) {
        result = lv;
    } else {
        result = newexpr(var_e);
        result->sym = newtemp();
    }
    emit(getretval, NULL, NULL, result,0,yylineno);
    return result;
}

unsigned nextquad(void){
    return currQuad;
}

void patchlabel(unsigned quadNo, unsigned label){
    assert(quadNo < currQuad);
    quads[quadNo].label = label;
}

void make_stmt(stmt_t* s){
    s = (stmt_t*)malloc(sizeof(stmt_t));
    s->breakList = 0;
    s->contList = 0;
}

ListNode* new_list(int quad) {
    ListNode* node = malloc(sizeof(ListNode));
    node->quad_no = quad;
    node->next = NULL;
    return node;
}

ListNode* mergelist(ListNode* l1, ListNode* l2){
    if (!l1) return l2;
    if (!l2) return l1;
    ListNode* head = l1;
    while (l1->next) l1 = l1->next;
    l1->next = l2;
    return head;
}

int get_list_element(ListNode* list, int index){
    int value=0;
    for( int i=0; i<index; i++){
        if(list){
            value = list->quad_no;
        }else{
            printf("[get_list_element]: ERROR OVERFLOW\n");
            return -1;
        }
        list = list->next;
    }
    return value;
}

void patchlist(ListNode* list, int label){
    while (list) {
        quads[list->quad_no].label = label;
        list = list->next;
    }
}

void print_list(ListNode* list){
    printf("list: ");
    while(list){
        printf("%d - ",list->quad_no);
        list =  list->next;
    }   
    printf("\n");
}


void emit_relop(iopcode op, expr* e1, expr* e2, expr* result, unsigned lineno) {
    unsigned if_quad = nextquad();
    emit(op, e1, e2, NULL, nextquad() + 2, lineno); // if true → skip next jump

    unsigned jump_quad = nextquad();
    emit(jump, NULL, NULL, NULL, nextquad() + 2, lineno); // jump unconditionally

    result->truelist = new_list(if_quad);
    result->falselist = new_list(jump_quad);
}

expr* handle_inc(expr* e) {
    expr* result = newexpr(arithexpr_e);
    result->sym = newtemp();

    emit(add, e, newexpr_constnum(1), result, 0, yylineno);
    emit(assign, result, NULL, e, 0, yylineno);             

    return result;
}

expr* handle_dec(expr* e) {
    expr* result = newexpr(arithexpr_e);
    result->sym = newtemp();

    emit(sub, e, newexpr_constnum(1), result, 0, yylineno);
    emit(assign, result, NULL, e, 0, yylineno);             

    return result;
}


expr* emit_or(expr* e1, expr* e2, int* really_useful_index_ptr) {
    expr* result = newexpr(boolexpr_e);
    int if_eq_index_e1 = 0, jump_index_e1 = 0;
    int if_eq_index_e2 = 0, jump_index_e2 = 0;
    if (e1->type == boolexpr_e && e2->type == boolexpr_e) {
        // Both are boolean expressions: just patch e1 falselist to start of e2
        int e2_start = nextquad();
        patchlist(e1->falselist, e2_start);

        result->truelist = mergelist(e1->truelist, e2->truelist);
        result->falselist = e2->falselist;

    } else if(e1->type == boolexpr_e){
        // Patch e1's falselist to point to e2's evaluation
        int e2_start = nextquad();

        if_eq_index_e2 = nextquad();
        emit(if_eq, e2, newexpr_constbool(1), NULL, nextquad() + 2, yylineno); // if e2

        jump_index_e2 = nextquad();
        emit(jump, NULL, NULL, NULL, nextquad() + 3, yylineno); // if e2 is false

        patchlist(e1->falselist, e2_start);

        result->truelist = mergelist(e1->truelist, new_list(if_eq_index_e2));
        result->falselist = new_list(jump_index_e2);

        *really_useful_index_ptr = if_eq_index_e1;
    }
    else if (e2->type == boolexpr_e) {
        // Emit for e1
        if_eq_index_e1 = nextquad();
        emit(if_eq, e1, newexpr_constbool(1), NULL, 0, yylineno); // target filled later
        
        jump_index_e1 = nextquad();
        emit(jump, NULL, NULL, NULL, 0, yylineno); // target filled later
        
        patchlist(e2->falselist, if_eq_index_e1);
        
        // Result truelist and falselist
        result->truelist = mergelist(new_list(if_eq_index_e1) , e2->truelist);
        result->falselist = new_list(jump_index_e1);
        
        // Track
        e1->truelist = new_list(if_eq_index_e1);
        e1->falselist = new_list(jump_index_e1);
        *really_useful_index_ptr = if_eq_index_e1;
    }
    else {
        // e1 is a constant/variable, emit its condition manually
        if_eq_index_e1 = nextquad();
        emit(if_eq, e1, newexpr_constbool(1), NULL, nextquad() + 2, yylineno); // if e1
        
        jump_index_e1 = nextquad();
        emit(jump, NULL, NULL, NULL, nextquad() + 1, yylineno); // if e1 false
        
        if_eq_index_e2 = nextquad();
        emit(if_eq, e2, newexpr_constbool(1), NULL, nextquad() + 2, yylineno); // if e2
        
        jump_index_e2 = nextquad();
        emit(jump, NULL, NULL, NULL, nextquad() + 3, yylineno); // if e2 false
        
        result->truelist = mergelist(new_list(if_eq_index_e1), new_list(if_eq_index_e2));
        result->falselist = new_list(jump_index_e2);
        
        *really_useful_index_ptr = if_eq_index_e1;
    }
    last_result = result;
    return result;
}

expr* emit_and(expr* e1, expr* e2, int* really_useful_index_ptr) {

    expr* result = newexpr(boolexpr_e);
    int if_eq_index_e2 = 0, jump_index_e2 = 0;
    int if_eq_index_e1 = 0, jump_index_e1 = 0;
    if (e1->type == boolexpr_e && e2->type == boolexpr_e) {
        
        patchlist(e1->truelist, *really_useful_index_ptr);
        print_list(e1->truelist);
        printf("%d\n", *really_useful_index_ptr);

        result->truelist = e2->truelist;
        result->falselist = mergelist(e1->falselist, e2->falselist);

    } else if (e2->type == boolexpr_e) {
        // e1 is NOT boolexpr, e2 IS boolexpr
        
        // Emit for e1
        if_eq_index_e1 = nextquad();
        emit(if_eq, e1, newexpr_constbool(1), NULL, nextquad() + 2, yylineno); // if e1 true

        jump_index_e1 = nextquad();
        emit(jump, NULL, NULL, NULL, nextquad() + 3, yylineno); // if e1 false

        patchlist(e2->truelist, if_eq_index_e1);

        result->truelist = new_list(if_eq_index_e1);
        result->falselist = mergelist(new_list(jump_index_e1), e2->falselist);

        *really_useful_index_ptr = if_eq_index_e1;

    } else if (e1->type == boolexpr_e) { // Case 1: e1 is already a boolean expression (patch its true list to evaluate e2)
        // Emit condition for e2
        if_eq_index_e2 = nextquad();
        emit(if_eq, e2, newexpr_constbool(1), NULL, nextquad() + 2, yylineno); // if e2
        
        jump_index_e2 = nextquad();
        emit(jump, NULL, NULL, NULL, nextquad() + 3, yylineno); // e2 false case
        // Patch e1's truelist to go to e2's evaluation
        patchlist(e1->truelist, if_eq_index_e2);
        // Final truelist is from e2's success
        result->truelist = new_list(if_eq_index_e2);
        // Final falselist is a merge of e1's falses and e2's
        result->falselist = mergelist(e1->falselist, new_list(jump_index_e2));
        *really_useful_index_ptr = if_eq_index_e1;
    } else { // Case 2: e1 is NOT a boolexpr (literal/variable), so we emit fresh condition for it
        int if_eq_index_e1 = 0, jump_index_e1 = 0;
        if_eq_index_e1 = nextquad();
        emit(if_eq, e1, newexpr_constbool(1), NULL, nextquad() + 2, yylineno); // if e1
        jump_index_e1 = nextquad();
        emit(jump, NULL, NULL, NULL, nextquad() + 3, yylineno); // e1 false
        if_eq_index_e2 = nextquad();
        emit(if_eq, e2, newexpr_constbool(1), NULL, nextquad() + 2, yylineno); // if e2
        jump_index_e2 = nextquad();
        emit(jump, NULL, NULL, NULL, nextquad() + 3, yylineno); // e2 false
        // Build result lists
        result->truelist = new_list(if_eq_index_e2);
        result->falselist = mergelist(new_list(jump_index_e1), new_list(jump_index_e2));
        *really_useful_index_ptr = if_eq_index_e1;
    }

    last_result = result;
    return result;
}



expr* get_result(void){
    return last_result;
}

stmt_t* new_stmt() {
	stmt_t* s = (stmt_t*) malloc(sizeof(stmt_t));
    s->quad_start = nextquad();
	s->nextlist = 0;
	return s;
}

void patch_all_jumps(){
    for(int i=0; i<nextquad(); i++){
 
        quad q = quads[i];
        if(q.op == jump){
            //if it points to a jump that is below it
            if(quads[q.label].op == jump && quads[q.label].label > q.label){
                q.label = quads[q.label].label;    //then just skip to the second one's label
            }
        }
    }
}

