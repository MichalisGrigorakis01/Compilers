#include "stack.h"

void init_stack(Stack *s) {
    s->top = -1;
}

int isEmpty(Stack *s) {
    return s->top == -1;
}

int isFull(Stack *s) {
    return s->top == MAX_STACK_SIZE - 1;
}

int create_and_push(Stack *s, const char *str, symTable* addr) {
    if (isFull(s)) {
        printf("Stack overflow!\n");
        return 0;
    }
    s->top++;
    strncpy(s->items[s->top].str, str, MAX_STRING_LENGTH - 1);
    s->items[s->top].str[MAX_STRING_LENGTH - 1] = '\0'; // Null-terminate
    s->items[s->top].sym_address = addr;
    return 1;
}


int push(Stack *s, Item *item) {
    if (isFull(s)) {
        printf("Stack overflow!\n");
        return 0;
    }
    s->top++;
    s->items[s->top] = *item; 
    return 1;
}


int pop(Stack *s, Item *poppedItem) {
    if (isEmpty(s)) {
        printf("Stack underflow!\n");
        return 0;
    }
    *poppedItem = s->items[s->top];
    s->top--;
    return 1;
}

int peek(Stack *s, Item *topItem) {
    if (isEmpty(s)) {
        return 0;
    }
    *topItem = s->items[s->top];
    return 1;
}

void print_stack(Stack *s) {
    printf("==== Stack contents (top to bottom) ====\n");
    for (int i = s->top; i >= 0; i--) {
        printf("  [%d] name: %s, sym address: %p\n", i, s->items[i].str, (void *)s->items[i].sym_address);
    }
}

void extract_global_variables(Stack *s, symTable *table){
    assert(table);
    int scope=0;
    symTable *cur;


    cur = table[scope].snext;
    
    while(cur!=NULL){
        if(!strcmp(cur->type, "GLOBAL VARIABLE")){
            create_and_push(s,cur->name, cur);
            cur->syminfo->offset = s->top;
        }
        cur = cur->snext; 
    }
}

int update_stack(Stack *s, quad *q){
    
}