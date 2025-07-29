#ifndef STACK_H
#define STACK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"
#include <assert.h>
#include "quads.h"

#define MAX_STACK_SIZE 1000
#define MAX_STRING_LENGTH 100


typedef struct {
    char str[MAX_STRING_LENGTH];
    symTable* sym_address;
} Item;

typedef struct {
    Item items[MAX_STACK_SIZE];
    int top;
} Stack;

void init_stack(Stack *s);

int isEmpty(Stack *s);

int isFull(Stack *s);

int create_and_push(Stack *s, const char *str, symTable* addr);
int push(Stack *s, Item *item);

int pop(Stack *s, Item *poppedItem);

int peek(Stack *s, Item *topItem);

void print_stack(Stack *s);

void extract_global_variables(Stack* s,symTable *table);

/* Updates the stack based on the given quad, returns 0 if all OK.
    Used to build the stack, alongside the final code, step by step - quad by quad
*/
int update_stack(Stack *s, quad *q);


#endif