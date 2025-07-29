#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"
#include <assert.h>

int MAX_SCOPE = 0; 
extern int yylineno;
static unsigned int programVarOffset = 0;
static unsigned int functionLocalOffset = 0;
static unsigned int formalArgOffset = 0;

static scopespace_t currScopeSpace = programvar;

scopespace_t currscopespace(void) {
    return currScopeSpace;
}

unsigned int currscopeoffset(void) {
    switch (currScopeSpace) {
        case programvar: return programVarOffset;
        case functionlocal: return functionLocalOffset;
        case formalarg: return formalArgOffset;
        default: return 0;
    }
}

void inccurrscopeoffset(void) {
    switch (currScopeSpace) {
        case programvar: programVarOffset++; break;
        case functionlocal: functionLocalOffset++; break;
        case formalarg: formalArgOffset++; break;
        default: break;
    }
}

void enterscopespace(void) {
    switch (currScopeSpace) {
        case programvar: currScopeSpace = formalarg; break;
        case formalarg: currScopeSpace = functionlocal; break;
        case functionlocal: currScopeSpace = functionlocal; break;
        default: break;
    }
}

void exitscopespace(void) {
    switch (currScopeSpace) {
        case functionlocal: currScopeSpace = formalarg; break;
        case formalarg: currScopeSpace = programvar; break;
        default: break;
    }
}

void resetformalargsoffset(void) {
    formalArgOffset = 0;
}

void resetfunctionlocalsoffset(void) {
    functionLocalOffset = 0;
}

void restorecurrscopeoffset(unsigned n) {
    switch (currScopeSpace) {
        case programvar: programVarOffset = n; break;
        case functionlocal: functionLocalOffset = n; break;
        case formalarg: formalArgOffset = n; break;
        default: break;
    }
}

Symbol* extractSymbol(symTable* entry) {
    if (!entry) return NULL;
    
    if (entry->syminfo) {
        return entry->syminfo;
    }
    // If syminfo is missing do the following
    Symbol* sym = malloc(sizeof(Symbol));
    if (!sym) {
        fprintf(stderr, "extractSymbol: malloc failed\n");
        exit(1);
    }

    sym->name = strdup(entry->name);
    sym->scope = entry->scope;
    sym->line = entry->line;
    sym->space = currscopespace(); // fallback guess
    sym->offset = currscopeoffset();
    sym->enabled = 1;
    sym->isFunction = 0;
    sym->taddress = 0;
    sym->snext = NULL;
    return sym;
}

void symPrint(symTable *table)
{
    assert(table);
    int i;
    symTable *cur;
    for (int i = 0; i <= MAX_SCOPE; i++)
    {
        printf("\n-------------- Scope %d --------------", i);

        if (table[i].snext == NULL)
        {
            continue;
        } // if empty move to next scope
        cur = table[i].snext;

        while (1)
        {
            // printing name & type
            printf("\n");
            printf(" \"%s\" ", cur->name);
            printf(" [%s] ", cur->type);

            // printing arguments if there are any
            if (cur->isFunction == 1)
            {
                if (cur->args == NULL)
                {
                    printf("Input string is NULL\n");
                    return;
                }

                char *token; // Pointer to hold each token after splitting

                // Create a copy of the input string since strtok modifies the string
                char csv_copy[strlen(cur->args) + 1];
                strcpy(csv_copy, cur->args);

                // Using strtok to split the CSV values
                token = strtok(csv_copy, ",");

                if (token != NULL)
                    printf("[Arguments: ");

                while (token != NULL)
                {
                    printf("%s", token);
                    token = strtok(NULL, ",");
                }
            }

            // printing line & scope
            printf(" (Line %d) ", cur->line);
            printf(" (Scope %d) ", cur->scope);

            // moving to next node if there is one
            if (cur->snext == NULL)
            {
                break;
            }
            cur = cur->snext; /*curr = curr+1*/
        }
    }
    printf("\n---------------------------------------------\n");
}
    
    
//if you dont care about the line just give it a negative number
int symDelete(symTable *table, char* name,char* type, int scope,int line){
	assert(table);
	symTable* cur;
	symTable* prev;
	int first=1;
		cur= table[scope].snext;
		prev=table;
		
	if(line>=0){
		while(1){
			if(cur==NULL)
				return 0;
			if((strcmp(cur->name, name) ==0) && (strcmp(cur->type, type)==0) && (cur->line == line)){
				free(prev->snext);
				prev->snext = cur->snext;
			}
			prev=cur;
			cur=cur->snext;
		}
	}else{
		while(1){
			if(cur==NULL)
				return 0;
			if((strcmp(cur->name, name) ==0) && (strcmp(cur->type, type)==0)){
				free(prev->snext);
				prev->snext = cur->snext;
			}
			prev=cur;
			cur=cur->snext;
		}
	}
    return 0;
}

//looks for a token with given name, scope, type, and returns it if found
symTable* symSearch(symTable *table, char *name, char *type, int scope)
{
	assert(table);
    symTable *cur;

	cur = table[scope].snext; //TODO: is this safe? is the frst node always empty?
    while (1){
	    if(cur==NULL){return 0;} //if it reaches the end, then the token has not been found
	
        if ((strcmp(cur->name, name) == 0) && (strcmp(cur->type, type) == 0))
        {
            return cur; /*found*/
        }
	    cur = cur->snext;
    }
    return NULL;
}

//turns on/off the enable field 
void change_enabled_atscope(symTable *table, int scope, int enable){
     assert(table);
     symTable *cur;
     
     if (scope<=0){
        return;
     }
     if(table[scope].snext==NULL){
        return;
     }
     cur = table[scope].snext;
     int i=0;
     while (1){
        
        if(cur->scope == scope){
            cur->enabled = enable;
            
        }
        if(cur->snext == NULL)break;
        cur = cur->snext; /*curr = curr+1*/
    }
}  

int get_enabled(symTable *table, char *name, char *type, int scope){
    assert(table);
    symTable *cur;

    cur = table[scope].snext; //TODO: is this safe? is the frst node always empty?
    while(1){
        if(cur==NULL){return -1;} //token not found
        if((strcmp(cur->name, name) ==0) && (strcmp(cur->type, type) ==0)){
            return (cur->enabled); //token is enabled
        }
        cur = cur->snext;
    }
    return -1; //token not found
}


//looks for entry with given name and same or smaller scope, if found returns the entry
symTable* temp_var_search(symTable *table,char *name,unsigned int scope){
    assert(table);
    printf("temp_var_srch\n");
    int lookup_scope = scope;
    int exists=0;
    
    symTable* cur;
    
    while(lookup_scope >= 0){  
    	cur = symSearch(table,name, "TEMP_VAR", lookup_scope);
        if(cur!=NULL){
            return cur;
        }
        lookup_scope = lookup_scope - 1;
    }
    return NULL;
}


/* TO ARGS EINAI PANTA 0 (GT DEN PROLAVAME NA YLOPOIHSOUME, HTAN GIA TIS SUNARTHSEIS)*/
symTable *symInsert(symTable *table, char *name, char *type, char *args, int line, int scope, int isFunction)
{
	//quick checks
	assert(table);
	assert(name);
	assert(type);

    //Scopelist *s_entry = new Scopelist();
        Scopelist *s_entry = (struct Scopelist *)malloc(sizeof(struct Scopelist));

    	//constructin the new node
        symTable *node, *cur;
        node = (symTable *)malloc(sizeof(symTable));
        node->name = malloc(strlen(name) + 1);
        node->type = malloc(strlen(type) + 1);
        node->args = malloc(strlen(args) + 1);
        strcpy(node->name, name);
        strcpy(node->type, type);
        strcpy(node->args, args);
        node->scope = scope;
        node->line = line;
        node->isFunction = isFunction;
        node->visible=1;
        node->enabled=1;
        node->snext = NULL;
        stsize++;


        if(MAX_SCOPE < scope){
            MAX_SCOPE = scope;
        }

	//if table is empty, put curr node at start
        if (table[scope].snext == NULL)
        {
            table[scope].snext = node;
            return table;
        }
        //else find the end of the table and put node at the end
        cur = table[scope].snext;
        //node = table[scope].snext;
        while (cur->snext != NULL)
        {
            if(cur->snext == NULL)break;
            cur = cur->snext; /*curr = curr+1*/
        }
        cur->snext = node;

        // Phase 4
        if (strcmp(type, "FORMAL ARGUMENT") == 0) {
            Symbol* s = malloc(sizeof(Symbol));
            s->type = var_s;
            s->space = formalarg;
            s->offset = currscopeoffset();
            s->scope = scope;
            s->line = line;
            s->enabled = 1;
            s->isFunction = 0;
            s->name = strdup(name);
            inccurrscopeoffset();
            node->syminfo = s;
        } else if (strcmp(type, "LOCAL VARIABLE") == 0) {
            Symbol* s = malloc(sizeof(Symbol));
            s->type = var_s;
            s->space = functionlocal;
            s->offset = currscopeoffset();
            s->scope = scope;
            s->line = line;
            s->enabled = 1;
            s->isFunction = 0;
            s->name = strdup(name);
            inccurrscopeoffset();
            node->syminfo = s;
        } else if (strcmp(type, "GLOBAL VARIABLE") == 0) {
            Symbol* s = malloc(sizeof(Symbol));
            s->type = var_s;
            s->space = programvar;
            s->offset = currscopeoffset();
            s->scope = scope;
            s->line = line;
            s->enabled = 1;
            s->isFunction = 0;
            s->name = strdup(name);
            inccurrscopeoffset();
            node->syminfo = s;
        }

        return table;
}

/* Walks up the scopes and detects if a variable is in an outer function scope */
int is_outer_function_scope(symTable *table, char *name, int current_scope) {
    symTable *entry = NULL;
    int found_scope = -1;

    // Search for the variable in outer scopes
    for (int s = current_scope - 1; s >= 0; s--) {
        entry = symSearch(table, name, "LOCAL VARIABLE", s);
        if (!entry)
            entry = symSearch(table, name, "FORMAL ARGUMENT", s);

        if (entry != NULL) {
            found_scope = s;

            // Check if a function is declared between found_scope and current_scope
            for (int i = found_scope; i < current_scope; i++) {
                symTable *cur = table[i].snext;
                while (cur != NULL) {
                    if (cur->isFunction == 1 && strcmp(cur->type, "USER FUNCTION") == 0) {
                        return 1;
                    }
                    cur = cur->snext;
                }
            }            

            break;
        }
    }

    return 0;
}









