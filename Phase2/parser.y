%{
    #include <stdio.h>
	#include <string.h>
    #include "yacc_helper.h"
	#include "Symtable.h"

    int yylex();
    extern int yylineno;
    extern char* yytext;
    FILE* yyin_o;
	extern FILE* yyin;
	symTable* table;
	int stsize; /*number of "symbols" */
	int scope;
	int is_function_result = 0;
	int inside_function_def=0;
	int inside_function_call =0;
	int suppress_assignment_error = 0;


	extern unsigned int currQuad;

	char curr_func_name[64] = "";  // current function being parsed
	char arglist[200];
	char assign_id[100];  /* extra id kept in case of assignments (redundancy)*/
	char objct_id[200];
	char prev_id[200]; 
	int prev_scope; /* extra scope kept in case a rule changes the current one, !used for variables! (redundancy)*/
	char probably_function_name[200];
	int anon_funx_count;
	int is_member;
	int return_statement;
	int block_opened;
	int is_double_colon;
	int in_anon_funx_def; /* 1 if we are into an anonymous function definition*/
	int dont_do_it; /* horsepiss code instance */

	int isit_anonymous(char* name){ //checking if funx is anonumous
		return (name[0]=='#');
	}

	/**
	 * @arg type 0 funx, 1 var, 2 arg, 3 funx&var (whatever elist calls)
	 * @arg DONOTINSERT in some cases we need to insert the token TODO remove this shit code 
	 * @atg CONCERN if 1, then print error and return, else ignore TODO remove prints alltogether and add error handling in the main code
	 * 
	 * @ret 1 if found declared, 0 if not found
	 */
	int declaration_check(int type, int DONOTINSERT /* some cases we need to insert if we dont find*/, int CONCERN){
		int declared=0;
		char type_str[50];

		// Check for illegal access to outer-scope variable inside nested function
		if (is_outer_function_scope(table, prev_id, scope)) {
			return 0; // Prevent insert, but pretend it's declared to suppress cascading errors
		}

		//TODO This dont make no sense, make sure we can delete it and the mentioned function
		if (isit_anonymous(prev_id)==1){ //if token is anonymous then its surely handled and declared from previous step, its just not in the symtable
			declared= 1;
		}

		if(type==1){
			if(symSearch(table,prev_id,"GLOBAL VARIABLE",0)){
				declared=1;
			}
			if(symSearch(table,prev_id,"LIBRARY FUNCTION",0)){
				declared=1;
			}

			// Search from current scope up to global scope for any local variable or argument
			for (int i = scope; i >= 0; i--) {
				if (symSearch(table, prev_id, "LOCAL VARIABLE", i) || symSearch(table, prev_id, "FORMAL ARGUMENT", i)) {
					declared = 1;
					break;
				}
			}

			for(int i=0; i<=prev_scope; i++){
				if(symSearch(table,prev_id,"FORMAL ARGUMENT",i)){
					if(get_enabled(table, prev_id, "FORMAL ARGUMENT",i)==1){
						declared=1;
					}
				}
			}
		
			strcpy(type_str,"variable");
		}else if(type==0){
			for(int i=0; i<=scope; i++){
				if(symSearch(table,prev_id,"USER FUNCTION",i)){
					declared=1;
				}
			}
			if(symSearch(table,prev_id,"LIBRARY FUNCTION",0)){
				declared=1;
			}
			for(int i=0; i<=prev_scope; i++){
				if(symSearch(table,prev_id,"FORMAL ARGUMENT",i)){
					declared=2;
				}
			}

			strcpy(type_str,"function");
		}else if(type==2){
			if(symSearch(table,prev_id,"GLOBAL VARIABLE",0)){
				declared=1;
			}
			for(int i=1; i<=scope; i++){
				if(symSearch(table,prev_id,"LOCAL VARIABLE",i)){
					declared=1;
				}
			}
			strcpy(type_str,"argument");
		}else if(type==3){
			if(symSearch(table,prev_id,"GLOBAL VARIABLE",0)){
				declared=1;
			}
			if(symSearch(table,prev_id,"LIBRARY FUNCTION",0)){
				declared=1;
			}
			for(int i=0; i<=prev_scope; i++){
				if(symSearch(table,prev_id,"LOCAL VARIABLE",i)){
					declared=1;
				}
			}
			for(int i=0; i<=scope; i++){
				if(symSearch(table,prev_id,"USER FUNCTION",i)){
					declared=1;
				}
			}
			if(symSearch(table,prev_id,"FORMAL ARGUMENT",prev_scope)){
				declared=1;
			}
			strcpy(type_str,"unknown");
		}
		
		if (!declared){
			if (DONOTINSERT!=1){
				//Axhually, if its not found, it has to be inserted as a variable, bc it counts as a definition
				if(prev_scope ==0){
					symInsert(table, prev_id, "GLOBAL VARIABLE", "0", yylineno, scope,0);
				}else{
					symInsert(table, prev_id, "LOCAL VARIABLE", "0", yylineno, scope,0);
				}
			}else{
				if (CONCERN==1){
						printf("[DECLARATION_CHECK] ERROR: Undefined reference (type: %s) to \"%s\" in line %d\n",type_str, prev_id,yylineno);
				}
			}
		}
		
		return declared;
	}
	
	
	void print_file(char * msg,int scope, int lineno, char *content){
		fprintf(yyin_o,"NAME: %s, TYPE:%s, LINENO: %d, SCOPE: #%d\n",content,msg,lineno,scope);
		return ;
	}
	
	int handle_token(int type /*0 funx, 1 var, 2 arg, 3 library funx*/){
		if(type==0){ /*in case of function*/
			/* validity check*/
			for(int i=0; i<=scope; i++){
				if(symSearch(table,prev_id,"USER FUNCTION",i)){
					//thats not an error, its shadowing
					//printf("ERROR: Function redeclaration: \"%s\" line: %d \n ",prev_id, yylineno);
					//return 1;
				}
				if(symSearch(table,prev_id,"LOCAL VARIABLE",i)){
					if (get_enabled(table, prev_id, "USER FUNCTION",i)==1){
						printf("[HANDLE_TOKEN] ERROR: user function already declared as local variable: \"%s\" line %d\n", prev_id, yylineno);
						return 1;			
					}
				}
				
				if(symSearch(table, prev_id, "FORMAL ARGUMENT",i)){
					if(get_enabled(table, prev_id, "FORMAL ARGUMENT", i)==1){
						printf("[HANDLE_TOKEN] ERROR: user function already declared as formal argument: \"%s\" line %d\n", prev_id, yylineno);
						return 1;
					}
				}
				
			}

			if(symSearch(table,prev_id,"LIBRARY FUNCTION",0)){
				printf("[HANDLE_TOKEN] ERROR: user function already declared as library function: \"%s\" line %d\n", prev_id, yylineno);
				return 1;
			}
			if(symSearch(table,prev_id,"GLOBAL VARIABLE",0)){
				printf("[HANDLE_TOKEN] ERROR: user function declared as variable: \"%s\" line %d\n", prev_id, yylineno);
				return 1;
			}
			if (symSearch(table,prev_id,"USER FUNCTION",scope)){
				if (get_enabled(table, prev_id, "USER FUNCTION",scope)==1){
					printf("[HANDLE_TOKEN] ERROR: user function already declared: \"%s\" line: %d \n ",prev_id, yylineno);
					return 1;
				}
			}
			if(symSearch(table,prev_id,"LOCAL VARIABLE",scope)){
				if (get_enabled(table, prev_id, "LOCAL VARIABLE",scope)==1){
					printf("[HANDLE_TOKEN] ERROR: Function declaration conflicts with local variable: \"%s\" line %d\n", prev_id, yylineno);
					return 1;			
				}
			}
			
			if(symSearch(table, prev_id, "FORMAL ARGUMENT",scope)){
				if(get_enabled(table, prev_id, "FORMAL ARGUMENT", scope)==1){
					printf("[HANDLE_TOKEN] ERROR: user function already declared as formal argument: \"%s\" line %d\n", prev_id, yylineno);
					return 1;
				}
			}
			

			/* insertion*/
			print_file("USER FUNCTION",scope,yylineno,prev_id);
			symInsert(table, prev_id, "USER FUNCTION", arglist, yylineno, scope,1); /* !!! here we use the scope and not prev_scope bc this has been altered by the variables inside of the function definition*/
			return 0;
		}
		else if(type==1){ /* in case of variable */
			/*validity check */

			if (symSearch(table, prev_id, "USER FUNCTION", scope)) {
				if (get_enabled(table, prev_id, "USER FUNCTION", scope)) {
					printf("[HANDLE_TOKEN] ERROR: Variable \"%s\" already defined as function at scope %d, line %d\n", prev_id, scope, yylineno);
					return 1;
				}
			}
			/* Library function conflict check */
			if(symSearch(table,prev_id,"LIBRARY FUNCTION",0)){
				printf("[HANDLE_TOKEN] ERROR: Variable declared as library function: \"%s\" line %d\n", prev_id, yylineno);
				return 1;
			}

			if(symSearch(table,prev_id,"GLOBAL VARIABLE",0) && scope == 0){
				return 0;
			}

			for(int i=0; i<=scope; i++){
				if(symSearch(table,prev_id,"LOCAL VARIABLE",i)){
					if(get_enabled(table, prev_id, "LOCAL VARIABLE",i)==0){
						symInsert(table, prev_id, "LOCAL VARIABLE","0", yylineno, scope, 0);
					}
					return 0;
				}
			}
			/* insertion */
			if(scope==0){
				print_file("GLOBAL VARIABLE",0,yylineno,prev_id);
				symInsert(table, prev_id, "GLOBAL VARIABLE", "0", yylineno, 0, 0);
				return 0;
			}else{
				print_file("LOCAL VARIABLE",scope,yylineno,prev_id);
				symInsert(table, prev_id, "LOCAL VARIABLE", "0", yylineno, scope, 0);
				return 0;
			}
		}
		
		else if(type==2){
			print_file("FORMAL ARGUMENT",scope,yylineno,prev_id);
			if(symSearch(table,prev_id,"FORMAL ARGUMENT",scope)){
				if(get_enabled(table, prev_id, "FORMAL ARGUMENT", scope)==1){
					printf("[HANDLE_TOKEN] ERROR: Argument with same name: %s, already given, line: %d\n", prev_id, yylineno);
					return 1;
				}
			}
			if (symSearch(table,prev_id,"LIBRARY FUNCTION",0)){
				printf("[HANDLE_TOKEN] ERROR: Can't use library function: %s as argument (line: %d) \n", prev_id, yylineno);
				return 1;
			}
			symInsert(table, prev_id, "FORMAL ARGUMENT", "0", yylineno, scope,0);
			return 0;	
		}
		else if(type==3){ /*in case of library function*/
			/* no need for validity check*/
			/* insertion*/
			print_file("LIBRARY FUNCTION",scope,yylineno,prev_id);
			symInsert(table, prev_id, "LIBRARY FUNCTION", arglist, 0, scope,1);
			return 0;
		}
		
		return 0;
	}
	

%}

/*%output "parser.c" POSIX YACC DOES NOT SUPPORT THIS*/
/*%defines POSIX YACC DOES NOT SUPPORT THIS*/

%union{
    	char* stringValue;
   	int intValue;
    	double realValue;
}


%start program
%expect 16
/*Symbols*/
%token<stringValue> ID
%token<stringValue> STRING
%token<intValue>   INT
%token<realValue>  DOUBLE

/*Statements*/
%token IF     
%token ELSE     
%token WHILE    
%token FOR      
//%token BLOCK    
%token FUNCTION 
%token<boolVal> TRUE     
%token<boolVal> FALSE    
%token LOCAL    
%token NIL      

/*Operators*/
%token PLUS     
%token MINUS    
%token MULT     
%token DIV      
%token MOD     
%token GT      
%token GE       
%token LT       
%token LE       
%token EQ       
%token NE      
%token AND      
%token OR       
%token NOT      
%token ASSIGN   
%token MINUS_MINUS 
%token PLUS_PLUS 

/*Other*/
%token SEMICOLON
%token LEFT_BRACKET 
%token RIGHT_BRACKET 
%token L_CURLY_BR  
%token R_CURLY_BR  
%token L_BRACKET   
%token R_BRACKET  	
%token COMMA     
%token COLON      
%token DOUBLE_COLON 
%token DOT          
%token DOUBLE_DOT   
%token EXCLAMATION  
%token COMMENT


/* Library functions or whatever these are*/
%token PRINT 
%token INPUT 
%token OBJECTMEMBERKEYS 
%token OBJECTTOTALMEMBERS 
%token OBJECTCOPY 
%token TOTALARGUMENTS 
%token ARGUMENT 
%token TYPEOF 
%token STRTONUM 
%token SQRT 
%token COS 
%token SIN 

/*Priorities*/
%right  ASSIGN
%left   OR
%left   PLUS
%left   MULT
%left   AND
%right  NOT
%right  MINUS
%left   DIV
%left   L_BRACKET
%left   R_BRACKET
%left   MOD
%right  MINUS_MINUS
%right  PLUS_PLUS
%left   LEFT_BRACKET
%left   RIGHT_BRACKET
%left   DOUBLE_DOT
%left   DOT
%nonassoc EQ
%nonassoc NE
%nonassoc GE
%nonassoc LE
%nonassoc GT
%nonassoc LT


/*non terminals */
%type<statement> program
%type<statement> stmt
%type<statement> ifstmt
%type<statement> whilestmt
%type<statement> forstmt
%type<statement> semicolon
%type<statement> returnstmt
%token<statement> RETURN      
%token<statement> CONTINUE
%token<statenent> BREAK
%type<statement> block
%type<statement> stmt_list

%type<expression> expr
%type<expression> op
%type<expression> term
%type<expression> assignexpr
%type<expression> primary
%type<expression> lvalue
%type<expression> member
%type<expression> call
%type<expression> elist
%type<expression> objectdef
%type<expression> indexed
%type<expression> indexedelem
%type<expression> const
%type<expression> arithmetic






%%	


program: stmt 	{}
	| stmt program	{}	
	;


stmt:	expr semicolon			{}
	| ifstmt			{}
	| whilestmt			{}
	| forstmt			{}	
	| returnstmt		{
							if(block_opened==0){
								printf("[STMT] ERROR: return statement ouside of function (line: %d)\n", yylineno);
							}
						}
	| BREAK semicolon	{
							if(block_opened==0){
								printf("[STMT] ERROR: break statement ouside of loop (line: %d)\n", yylineno);
							}
						}
	| CONTINUE semicolon	{
								if(block_opened==0){
									printf("[STMT] ERROR: continue statement ouside of loop (line: %d)\n", yylineno);
								}
							}
	| block				{}
	| funcdef			{}
	| anon_funcdef			{}
	| anon_funcdef_assign		{}
	| semicolon			{}
	;


expr:	assignexpr		{}	
	| expr op expr		{}	
	| term			{}
	| arithmetic		{}
	;


arithmetic: expr PLUS expr  {}
        | expr MINUS expr   {}
        | expr MULT expr    {}
        | expr DIV expr     {}
        | expr MOD expr     {}
        ;

op:	 GT	{}	
	| GE	{}	
	| LT	{}	
	| LE	{}	
	| EQ	{}	
	| NE	{}	
	| AND	{}	
	| OR	{}	
	;


term:	L_BRACKET expr  R_BRACKET {}
	| MINUS expr	{}
	| NOT expr		{}
	| PLUS_PLUS lvalue {declaration_check(1,0,1); if(declaration_check(0,0,0)==1){printf("[TERM] ERROR: Cannot use operators on function: %s, line: %d\n", prev_id, yylineno);}}
	| lvalue  PLUS_PLUS { declaration_check(1,0,1); if(declaration_check(0,0,0)==1){printf("[TERM] ERROR: Cannot use use operators on function: %s, line %d\n", prev_id, yylineno);}}
	| MINUS_MINUS lvalue {declaration_check(1,0,1);  if(declaration_check(0,0,0)==1){printf("[TERM] ERROR: Cannot use use operators on function: %s, line %d\n", prev_id, yylineno);}}
	| lvalue  MINUS_MINUS {declaration_check(1,0,1); if(declaration_check(0,0,0)==1){printf("[TERM] ERROR: Cannot use use operators on function: %s, line %d\n", prev_id, yylineno);}}
	| primary {}
	;

assignexpr:	lvalue ASSIGN {
								strcpy(assign_id, prev_id); 
								//if( declaration_check(0,1,0) && is_member==0){printf("ERROR: Cannot assign value to function: %s, line %d\n", prev_id, yylineno);}
								if (is_double_colon == 0 && inside_function_call == 0 && strlen(assign_id) > 0 && suppress_assignment_error == 0) {
									int is_local_var = symSearch(table, prev_id, "LOCAL VARIABLE", scope) != NULL;
									if (!is_local_var) {
										for (int i = scope; i >= 0; i--) {
											if (symSearch(table, prev_id, "USER FUNCTION", i)) {
												printf("[ASSIGNEXPR] ERROR: Cannot assign value to function name: \"%s\" at line %d\n", prev_id, yylineno);
												break;
											}
										}
									}									
									if (!symSearch(table, prev_id, "USER FUNCTION", scope)) {
										handle_token(1);
									}
								} else {
									is_double_colon = 0;
								}
								suppress_assignment_error = 0;
								is_function_result = 0;
	}  expr	{
								if(strcmp(prev_id,assign_id)!=0){ //checking if prev_id has changed, this means that the other end of the assignment has a variable (else its a constant)
									int result = declaration_check(1,1,0); //checking if the variable exists
									int result2 = declaration_check(0,1,0); //checking if the function exists
									if(result==0 && result2 ==0){
										printf("[ASSIGNEXPR] ERROR: Undefined reference to \"%s\" in line %d\n", prev_id,yylineno);
									}
								}
	}
		;

primary:	lvalue 				{is_function_result = 0;}
		| call				{is_function_result = 1;}		
		| objectdef			{ strcpy(prev_id, objct_id); is_function_result = 0;}	
		| L_BRACKET funcdef R_BRACKET	{is_function_result = 1;}
		| L_BRACKET anon_funcdef R_BRACKET {is_function_result = 1;}	
		| const				{is_function_result = 0;}	
		;

/* this rule from now on, is responsible for keeping the information of each ID, this information will be handled in higher level rules*/
lvalue:	ID			
			{	
				is_member = 0;
				strcpy(prev_id, yytext);
				prev_scope = scope;
				int illegal = is_outer_function_scope(table, yytext, scope);
				suppress_assignment_error = 0;

				int given_as_argument=0;
				int given_as_argument_at_scope=0;
				if (illegal) {
					//PROBLEEEM TODO 
					printf("defs %d, scope: %d\n", inside_function_def, scope);
					/*
					if (inside_function_def>0){
						for(int i=0; i<scope; i++){
							if(symSearch(table,yytext,"FORMAL ARGUMENT",i)){
								given_as_argument=1;
								break;
							}
						}
						if(symSearch(table,yytext,"FORMAL ARGUMENT",scope)){
							given_as_argument_at_scope=1;
							break;
						}

					}
					*/
					if (inside_function_def>0){
						if(symSearch(table,yytext, "FORMAL ARGUMENT",inside_function_def)){
							given_as_argument=1;
						}
					}

					if(given_as_argument ==0){
						suppress_assignment_error = 1;
						if (in_anon_funx_def==1){
							printf("[LVALUE] ERROR : Cannot access '%s' inside anonymous function (line: %d)\n", yytext, yylineno);
						}else{
							printf("[LVALUE] ERROR : Cannot access '%s' inside function '%s' (line: %d)\n", yytext, curr_func_name, yylineno);
						}
					}
				}
			}
 
	| LOCAL ID					{
								is_member=0;
								strcpy(prev_id, $2);
								prev_scope=scope;
								handle_token(1);
							}
	| DOUBLE_COLON ID	{
							symTable* globalVar = symSearch(table,$2,"GLOBAL VARIABLE",0);
							symTable* globalFun = symSearch(table,$2,"USER FUNCTION",0);
							
							if(!globalVar && !globalFun){
								printf("[LVALUE] ERROR: No global variable or function '::%s' exists (line: %d)\n", $2, yylineno);
							} else {
								is_member = 0;
								is_double_colon = 1;
								strcpy(prev_id, $2);
								prev_scope = 0;
								declaration_check(3, 1, 1);
							}
						}
	| member					{  is_member=1;}
	;

member:	lvalue DOT ID	{ 
							if (!is_function_result) {
								prev_scope = scope;
								declaration_check(1,0,1);
							}
							is_function_result = 0;
						}
	| lvalue DOUBLE_DOT ID	{ 
								if (!is_function_result) {
									prev_scope=scope; 
									declaration_check(1,0,1);
								}
								is_function_result = 0;
							}
	| lvalue {} LEFT_BRACKET expr RIGHT_BRACKET{/* could be decl or access so handle */
												declaration_check(1,1,0);
											}
	| call DOT ID {/* N/A */}
	| call LEFT_BRACKET expr RIGHT_BRACKET {/* N/A */}
	;

call:	call L_BRACKET elist R_BRACKET	{is_function_result = 1;}
	| call L_BRACKET R_BRACKET	{is_function_result = 1;}
	| lvalue 
		{
			if(!is_member)	
				strcpy(probably_function_name, prev_id); /*the prev_id will have stored the latest id, this could be argument, so store the functions name at once to use it later at insertion*/	
		} 
			callsuffix				
			{
				inside_function_call=1; //we are in a function call, which will terminate at a semicolon
				is_function_result =1; 
				if(!is_member){
					strcpy(prev_id, probably_function_name);
					int check_result1 = declaration_check(0,1,0); // if this is 1, then its already declared as a function	
					int check_result2 = declaration_check(3,1,0);  // if this is 1 AND NOT check_result1, then declared as a POINTER to a function, by an assignexpr
					if (check_result1==0){
						if (check_result2==0){
							printf("[CALL] ERROR: Undefined reference (type: function) to \"%s\" in line %d\n", prev_id, yylineno);
						}else{
							printf("[CALL] WARNING: Detected reference to a function possibly by a pointer: %s in line: %d\t(How should i handle this?)\n",prev_id,yylineno);
						}
					}
				}
				is_member=0;

				strcpy(assign_id, ""); // Clear assign_id so assignexpr knows this is a call
			}						
							
	| L_BRACKET funcdef R_BRACKET L_BRACKET {inside_function_call=1;} elist R_BRACKET {is_function_result = 1;}
	;

callsuffix:	funxcall		{}
		| method_funx_call 	{}
		;

funxcall: 	normcall		{}
		| normcall_empty	{}
		;

method_funx_call: methodcall		{}
		| methodcall_empty	{}
		;

normcall:	L_BRACKET elist R_BRACKET		{}
		| L_BRACKET elist R_BRACKET normcall	{}
		;

normcall_empty: L_BRACKET R_BRACKET 			{}
		| L_BRACKET R_BRACKET normcall_empty	{}
		;

methodcall:	lvalue DOT ID L_BRACKET lvalue COMMA elist R_BRACKET	{}
		| lvalue DOT ID L_BRACKET elist R_BRACKET		{}
		;

methodcall_empty: lvalue DOT ID L_BRACKET lvalue COMMA R_BRACKET	{}
		| lvalue DOT ID L_BRACKET R_BRACKET			{}
		;

elist:	 expr {declaration_check(3,0,1);}
	| expr {declaration_check(3,0,1);} COMMA elist 	{}
	;

objectdef:	LEFT_BRACKET {strcpy(objct_id, prev_id);} elist RIGHT_BRACKET	{

							}
		| LEFT_BRACKET {strcpy(objct_id, prev_id);} indexed RIGHT_BRACKET	{

							}
		| LEFT_BRACKET {strcpy(objct_id, prev_id);} RIGHT_BRACKET	{}
		;

indexed:	indexedelem			{}
		| indexedelem COMMA indexed	{}
		;

indexedelem:	L_CURLY_BR expr COLON expr R_CURLY_BR		{}
		;

block:		 L_CURLY_BR {scope+=1; block_opened+=1;} stmt_list {change_enabled_atscope(table,scope,0); scope-=1; } R_CURLY_BR	{block_opened -= 1;}		
		;

stmt_list:	stmt
		| stmt_list stmt
		;

block_empty:	L_CURLY_BR R_CURLY_BR
		;
		
code_block:	block
		| block_empty
		;

funcdef:    FUNCTION ID {
							inside_function_def+=1;
							for(int i=0; i<=scope; i++){
								if(get_enabled(table, $2, "FORMAL ARGUMENT", i)>0){
									
									printf("[FUNCDEF] ERROR: Function: %s (line: %d), exists as argument\n", $2 ,yylineno);
									dont_do_it =1;
								} //check if it has been declared before as an argument
							}
						} L_BRACKET {scope += 1; }idlist {
		//prev_scope = scope; //dangerous
		scope -= 1;
		//change_enabled_atscope(table,scope,0);
		//printf("scope: %d disabled %s\n", scope, $2);
		strcpy(prev_id,$2);
		if(dont_do_it==0){
			handle_token(0);
		}else{
			dont_do_it=0;
		}
		strcpy(arglist, "");
		strcpy(curr_func_name, $2);
		}
		R_BRACKET code_block {inside_function_def-=1;}
		;
	
anon_funcdef:	 FUNCTION L_BRACKET {scope+=1;} idlist {
									inside_function_def+=1;
									in_anon_funx_def =1;
									//change_enabled_atscope(table,scope,0);
									scope -=1;
									anon_funx_count+=1; 
									char str [100];
									char count [10];
									snprintf(count, sizeof(count), "%d", anon_funx_count);
									strcpy(str, "#ANON_FUNX_");
									strcat(str,count); 
									strcpy(prev_id, str);
									handle_token(0);
									strcpy(arglist,"");
									}
									 R_BRACKET code_block	{in_anon_funx_def=0; inside_function_def-=1;}
		;

anon_funcdef_assign:	lvalue  ASSIGN {/*TODO should this be inserted?*/} anon_funcdef		{/*N/A*/}	
			;


const:		INT	{}
		| DOUBLE	{}
		| STRING	{}
		| NIL	{}
		| TRUE	{}
		| FALSE	{}
		;

idlist	:	ID					{is_member=0; strcpy(prev_id,$1);  handle_token(2); strcat(arglist,"("); strcat(arglist, $1); strcat(arglist,") "); }
		| idlist COMMA ID			{is_member=0; strcpy(prev_id, $3);  handle_token(2); strcat(arglist,"("); strcat(arglist, $3); strcat(arglist,") ");}
		| {}				
		;


ifstmt:        IF {} L_BRACKET {scope = scope + 1;}expr R_BRACKET {scope = scope - 1;} stmt elsestmt {}
		;

elsestmt:	ELSE stmt	{}
		|		
		;

whilestmt:    WHILE {} L_BRACKET {scope = scope + 1;} expr {} R_BRACKET {scope = scope - 1;} stmt {}
		;


forstmt:    FOR {scope = scope + 1;} for_stuff {}
		;
		
/* unhinged */
for_stuff: 	L_BRACKET elist semicolon expr semicolon elist R_BRACKET {scope = scope - 1;}stmt
		| L_BRACKET semicolon expr semicolon elist R_BRACKET {scope = scope - 1;}stmt
		| L_BRACKET elist semicolon semicolon elist R_BRACKET {scope = scope - 1;}stmt
		| L_BRACKET elist semicolon expr semicolon R_BRACKET {scope = scope - 1;}stmt
		| L_BRACKET semicolon expr semicolon R_BRACKET {scope = scope - 1;}stmt
		| L_BRACKET elist semicolon  semicolon  R_BRACKET {scope = scope - 1;}stmt
		| L_BRACKET semicolon  semicolon elist R_BRACKET {scope = scope - 1;}stmt
		| L_BRACKET semicolon semicolon R_BRACKET {scope = scope - 1;}stmt
		;
		

semicolon: SEMICOLON{
	inside_function_call=0; //after a semicolon a function call is surely over
}

returnstmt:	RETURN {} semicolon {}
		|  RETURN {} expr semicolon {}
		;
		
%%

int main(int argc,char** argv){

    table = (symTable *)malloc(sizeof(symTable)*10);
    stsize = 0;
    scope = 0;
    anon_funx_count=0;
    is_member=0;
	int arg = 0;

    strcpy(arglist,"");
    
    if(table==NULL){
        printf("shit\n");
    }

    if(!(yyin_o = fopen("output_yacc.txt","w"))){
        printf("cant open yacc output file\n");
        return 1;
    }
    
    
    	/* Adding library functions */
    	char funx_names[12][40] = {"print", "input", "objectmemberkeys", "objecttotalmembers", "objectcopy", "totalarguments", "argument", "typeof", "strtonum", "sqrt", "cos", "sin"};
	
	for(int i=0; i<12; i++){
		strcpy(prev_id, (char*)(funx_names+i));
		handle_token(3);
	}
    
    if(argc > 1){
		(strcmp(argv[1], "-output") == 0) ? (arg = 1) : (arg = 0);
        if(!(yyin = fopen(argv[1],"r"))){
            fprintf(stderr,"Cannot read file: %s\n", argv[1]);
            fclose(yyin_o);
            return 1;
        }else{printf("Reading code from: %s\n", argv[1]);}
    }else{
    printf("Expecting input from terminal:\n\n");
        yyin = stdin;
    }
    if(yyparse() !=0){printf("\nSomething is wrong with the parser. Maybe a syntax error....\n");}
    symPrint(table);
    fclose(yyin_o);

    return 0;
}
