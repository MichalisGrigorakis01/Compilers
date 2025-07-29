%{
    #include <stdio.h>
	#include <string.h>
	#include "symtable.h"
    #include "yacc_helper.h"
	#include "expressions.h"
	#include "quads.h"

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
	expr* current_return_expr = NULL;
	int inside_loop = 0;
	int function_nesting_level = 0;
	int break_continue_invalid_context = 0;

	int not_flag = 0;
	FILE* quad_output = NULL;

	extern unsigned int currQuad;

	char curr_func_name[64] = "";  // current function being parsed
	char arglist[200];
	char assign_id[100][100];  /* extra id kept in case of assignments, TODO should be dynamic but fucking hell already*/
	int assign_id_index=-1;
	char objct_id[200];
	char prev_id[200]; 
	int prev_scope; /* extra scope kept in case a rule changes the current one, !used for variables! (redundancy)*/

	char probably_function_name[200];
	char recent_function_name_from_funcdef[200]; // this is actually useful only in anonymous defs
	char funcdef_names[100][100]; //storage of function names recently declared

	int anon_funx_count;
	int is_member;
	int return_statement;
	int block_opened;
	int is_double_colon;
	int in_anon_funx_def; /* 1 if we are into an anonymous function definition*/
	int dont_do_it; /* horsepiss code instance */
	extern expr* last_result;

	int funcstart_jump_quad_index[100]; // this stores the quad index of the jump quad that is emitted at funcstart, to be patchlabeled
	int return_quad_indices [100][100]; //the indices of return quads that need to point ti the funcend
    int return_quads[100]; //the return quads awaiting patch
    int function_index = 0;// how many nested functions there are

	extern int short_circuit_mode;

	unsigned loop_body_label;
	unsigned loop_post_label;
	unsigned loop_cond_check;
	unsigned loop_body_jump_back;
	unsigned loop_exit_jump;

	int and_or_indicator = 0;
	int really_useful_index = 0; //for bool perations


	int isit_anonymous(char* name){ //checking if funx is anonumous
		return (name[0]=='#');
	}


	expr* get_expression(char* token_id){
		symTable* entry = NULL;
		entry = symSearch(table, token_id, "FORMAL ARGUMENT", scope);
		if (!entry) entry = symSearch(table, token_id, "GLOBAL VARIABLE", 0);
		if (!entry) entry = symSearch(table, token_id, "LIBRARY FUNCTION", 0);
		for(int i =0; i<=scope; i++){
			if (!entry) entry = symSearch(table, token_id, "LOCAL VARIABLE", i);
			if (!entry) entry = symSearch(table, token_id, "USER FUNCTION", i);
		}

		expr* e;
		if(entry){
			Symbol* sym = extractSymbol(entry);
			e = lvalue_expr(sym);
	
		}else{
			printf("[get_expression] ERROR: Failed. No such symbol (%s) found in Symtable\n", token_id);
			e=NULL;
		}
		return e;
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
		unsigned int uns;
		expr* expression;
		struct stmt_t* statement;
}


%start program
//%expect 16
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
//%token UMINUS

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
%right UMINUS
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
%type<statement> elsestmt
%type<statement> whilestmt
%type<statement> forstmt
%type<statement> semicolon
%type<statement> returnstmt
%token<statement> RETURN      
%token<statement> CONTINUE
%token<statement> BREAK
%type<statement> block
%type<statement> stmt_list
%type<statement> lcurly_stmt_list

%type<uns> whilestart
%type<expression> whilecond

%type<expression> forprefix
%type<uns> elsestart
%type<uns> N
%type<uns> M

%type<expression> ifprefix

%type<expression> expr
//%type<expression> op
%type<expression> term
%type<expression> assignexpr
%type<expression> primary
%type<expression> lvalue
%type<expression> member
%type<expression> call
%type<expression> elist
%type<expression> objectdef
%type<expression> indexed
%type<expression> idlist
%type<expression> indexedelem
%type<expression> funcdef
%type<expression> anon_funcdef
%type<expression> const
%type<expression> callsuffix
%type<expression> funxcall
%type<expression> normcall


//%type<expression> arithmetic
//%type<expression> relational
//%type<expression> other






%%	


program: stmt 	{ }
	| stmt program	{ }	
	;


stmt:	expr semicolon	{$$ = new_stmt(); resettemp();}
	| ifstmt			{$$ = $1; resettemp();}
	| elsestmt            {$$ = $1; resettemp();}
	| whilestmt			{$$ = $1; resettemp();}
	| forstmt			{$$ = $1; resettemp();}	
	| returnstmt		{
							$$ = new_stmt();
							if(block_opened==0){
								printf("[STMT] ERROR: return statement ouside of function (line: %d)\n", yylineno);
								exit(1);
							}
							resettemp();
						}
	| BREAK semicolon	{
							if (inside_loop == 0 || break_continue_invalid_context) {
								fprintf(stderr, "[ERROR] Invalid 'break' inside function at line %d\n", yylineno);
								exit(1);
							}
							$$ = new_stmt();
							if (inside_loop == 0) {
								fprintf(stderr, "[ERROR] 'break' used outside of a loop at line %d\n", yylineno);
								exit(1);
							}
							emit(jump, NULL, NULL, NULL, 0, yylineno);
							$$->breakList = new_list(nextquad() - 1);
							if(block_opened==0){
								printf("[STMT] ERROR: break statement ouside of loop (line: %d)\n", yylineno);
							}
							resettemp();
						}
	| CONTINUE semicolon	{
								if (inside_loop == 0 || break_continue_invalid_context) {
									fprintf(stderr, "[ERROR] Invalid 'continue' inside function at line %d\n", yylineno);
									exit(1);
								}
								$$ = new_stmt();
								if (inside_loop == 0) {
									fprintf(stderr, "[ERROR] 'continue' used outside of a loop at line %d\n", yylineno);
									exit(1);
								}
								emit(jump, NULL, NULL, NULL, 0, yylineno);
								$$->contList = new_list(nextquad() - 1);
								if(block_opened==0){
									printf("[STMT] ERROR: continue statement ouside of loop (line: %d)\n", yylineno);
								}
								resettemp();
							}
	| block				{$$ = $1; resettemp();}
	| funcdef			{$$ = new_stmt(); resettemp();}
	| anon_funcdef			{$$ = new_stmt(); resettemp();}
	| anon_funcdef_assign		{$$ = new_stmt(); resettemp();}
	| semicolon			{$$ = new_stmt(); resettemp();}
	;


expr:
		assignexpr                          { $$ = $1; and_or_indicator =0; }

		| expr AND expr {
			$$ = emit_and($1, $3, &really_useful_index);
			and_or_indicator =1;
			// emit_and already handles patching and sets truelist/falselist properly
		}

		| expr OR expr {
			 $$ = emit_or($1, $3, &really_useful_index);
			 and_or_indicator =-1;
			 // emit_or already handles patching and sets truelist/falselist properly
		}

		| expr PLUS expr     { and_or_indicator =0; $$ = newexpr(arithexpr_e); $$->sym = newtemp(); emit(add, $1, $3, $$, 0, yylineno); }
		| expr MINUS expr    { and_or_indicator =0; $$ = newexpr(arithexpr_e); $$->sym = newtemp(); emit(sub, $1, $3, $$, 0, yylineno); }
		| expr MULT expr     { and_or_indicator =0; $$ = newexpr(arithexpr_e); $$->sym = newtemp(); emit(mul, $1, $3, $$, 0, yylineno); }
		| expr DIV expr      { and_or_indicator =0; $$ = newexpr(arithexpr_e); $$->sym = newtemp(); emit(div_, $1, $3, $$, 0, yylineno); }
		| expr MOD expr      { and_or_indicator =0; $$ = newexpr(arithexpr_e); $$->sym = newtemp(); emit(mod_, $1, $3, $$, 0, yylineno); }

		| expr GT expr       { and_or_indicator =0; $$ = newexpr(boolexpr_e); $$->sym = newtemp(); emit_relop(if_greater, $1, $3, $$, yylineno); }
		| expr GE expr       { and_or_indicator =0; $$ = newexpr(boolexpr_e); $$->sym = newtemp(); emit_relop(if_greatereq, $1, $3, $$, yylineno); }
		| expr LT expr       { and_or_indicator =0; $$ = newexpr(boolexpr_e); $$->sym = newtemp(); emit_relop(if_less, $1, $3, $$, yylineno); }
		| expr LE expr       { and_or_indicator =0; $$ = newexpr(boolexpr_e); $$->sym = newtemp(); emit_relop(if_lesseq, $1, $3, $$, yylineno); }
		| expr EQ expr       { and_or_indicator =0; $$ = newexpr(boolexpr_e); $$->sym = newtemp(); emit_relop(if_eq, $1, $3, $$, yylineno); }
		| expr NE expr       { and_or_indicator =0; $$ = newexpr(boolexpr_e); $$->sym = newtemp(); emit_relop(if_noteq, $1, $3, $$, yylineno); }

		| term               {
				if($1->type == boolexpr_e) {
					and_or_indicator = 0;
				}
			 	$$ = $1;
			 }
		
		;

term:
			MINUS expr %prec UMINUS {
				expr* evaluated = emit_iftableitem($2);
				check_arith(evaluated, "unary minus");
				$$ = newexpr(arithexpr_e);
				$$->sym = evaluated->sym; /* IMPORTANT */
				emit(uminus, evaluated, NULL, $$, 0, yylineno);
			}

			| NOT expr {
				if($2->type != boolexpr_e){ //in case of constant or variable, before its turned into an expression, ve gotta emit
					$2->truelist = new_list(nextquad());
					emit(if_eq, $2, newexpr_constbool(1), NULL, 0, yylineno);
					$2->falselist = new_list(nextquad());
					emit(jump, NULL, NULL, NULL, 0, yylineno);
				}
				$$ = newexpr(boolexpr_e);
				$$->sym = $2->sym; //possibly reduntant, $$ is no longer a constant but its an expression, so no symbol
				$$->truelist = $2->falselist;
				$$->falselist = $2->truelist;
				$$->not_flag = ($2->not_flag + 1) % 2;
			}

			| PLUS_PLUS lvalue {
				declaration_check(1, 0, 1);
				$$ = handle_inc($2);
			}

			| lvalue PLUS_PLUS {
				declaration_check(1, 0, 1);
				$$ = handle_inc($1);
			}

			| MINUS_MINUS lvalue {
				declaration_check(1, 0, 1);
				$$ = handle_dec($2);
			}

			| lvalue MINUS_MINUS {
				declaration_check(1, 0, 1);
				$$ = handle_dec($1);
			}

			| L_BRACKET expr R_BRACKET { $$ = $2; }

			| primary { $$ = $1; }
			;

assignexpr:	lvalue ASSIGN {
								assign_id_index +=1;
								strcpy(assign_id[assign_id_index], prev_id); 
								//if( declaration_check(0,1,0) && is_member==0){printf("ERROR: Cannot assign value to function: %s, line %d\n", prev_id, yylineno);}
								if (is_double_colon == 0 && inside_function_call == 0 && strlen(assign_id[assign_id_index]) > 0 && suppress_assignment_error == 0) {
									int is_local_var = symSearch(table, prev_id, "LOCAL VARIABLE", scope) != NULL;
									if (!is_local_var) {
										for (int i = scope; i >= 0; i--) {
											if (symSearch(table, prev_id, "USER FUNCTION", i)) {
												printf("[ASSIGNEXPR] ERROR: Cannot assign value to function name: \"%s\" at line %d\n", prev_id, yylineno);
												//break; // FASH 3 ERROR
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
								//Phase3
								//Extracting the symbol of the $1 element
								//This has to be done only here bc the symbol is handled just above this rule
								expr* res_e = get_expression(assign_id[assign_id_index]);

								expr* temp = newexpr(assignexpr_e);
								temp->sym = newtemp();
								expr* temp_result  = get_result(); // to get the last saved _t

								if ($4->type == boolexpr_e) {
									// If it is boolean with truelist/falselist, patch to a bool temp
									expr* bool_temp = newexpr(var_e);
									bool_temp->sym = newtemp();

									unsigned trueLabel = nextquad();
									emit(assign, newexpr_constbool(1), NULL, bool_temp, 0, yylineno);

									unsigned jumpToEnd = nextquad();
									emit(jump, NULL, NULL, NULL, 0, yylineno);

									unsigned falseLabel = nextquad();
									emit(assign, newexpr_constbool(0), NULL, bool_temp, 0, yylineno);

									unsigned after = nextquad();
									if($4->truelist){
										patchlist($4->truelist, trueLabel);
									}
									if($4->falselist){
										patchlist($4->falselist, falseLabel);
									}
									patchlabel(jumpToEnd, after);

									emit(assign, bool_temp, NULL, res_e, 0, yylineno); // Final assignment
								}else {
									emit(assign, $4, NULL, res_e, 0, yylineno);
								}
								emit(assign, res_e,NULL, temp, 0 ,yylineno);
								$$ = temp;

								//Phase2
								if(strcmp(prev_id,assign_id[assign_id_index])!=0){ //checking if prev_id has changed, this means that the other end of the assignment has a variable (else its a constant)
									int result = declaration_check(1,1,0); //checking if the variable exists
									int result2 = declaration_check(0,1,0); //checking if the function exists
									if(result==0 && result2 ==0){
										printf("[ASSIGNEXPR] ERROR: Undefined reference to \"%s\" in line %d\n", prev_id,yylineno);
									}
								}
								assign_id_index -=1;
								//sanity check redundant
								if(assign_id_index<-1){
									printf("[assignexpr]: ERROR: Messed up assign_id_index\n");
									assign_id_index =-1;
								}
	}
		;

primary:	lvalue 				{  $$ = get_expression(prev_id); is_function_result = 0;}
		| call				{ is_function_result = 1;}
		| objectdef			{ strcpy(prev_id, objct_id); is_function_result = 0;}
		| L_BRACKET funcdef R_BRACKET	{$$ = $2; is_function_result = 1;}
		| L_BRACKET anon_funcdef R_BRACKET {$$ = $2; is_function_result = 1;}	
		| const				{$$ = $1; is_function_result = 0;}	
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

					printf("defs %d, scope: %d\n", inside_function_def, scope);
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

				//phase3
				/* IMPORTANT */
				
				symTable* entry = NULL;
				entry = symSearch(table, yytext, "FORMAL ARGUMENT", scope);
				if (!entry) entry = symSearch(table, yytext, "LOCAL VARIABLE", scope);
				if (!entry) entry = symSearch(table, yytext, "GLOBAL VARIABLE", 0);
				if (!entry) entry = symSearch(table, yytext, "USER FUNCTION", scope);

				if (entry) {
					Symbol* sym = extractSymbol(entry);
					$$ = lvalue_expr(sym);
				} else {
					$$ = NULL;
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
							
							expr* t = newexpr(tableitem_e);
							t->sym = newtemp();
							t->index = newexpr_conststring($3);
							emit(tablegetelem, $1, t->index, t, 0, yylineno);
							$$ = t;
							$$ = member_item($1, $3);
							is_function_result = 0;
						}
	| lvalue DOUBLE_DOT ID	{
								expr* obj = $1;
								expr* method = newexpr(tableitem_e);
								method->sym = newtemp();
								method->index = newexpr_conststring($3);
								emit(tablegetelem, obj, method->index, method, 0, yylineno);
								$$ = method;
								
								if (!is_function_result) {
									prev_scope=scope; 
									declaration_check(1,0,1);
								}
								is_function_result = 0;
							}
	| lvalue {} LEFT_BRACKET expr RIGHT_BRACKET{/* could be decl or access so handle */
												declaration_check(1,1,0);
												expr* table = emit_iftableitem($1);
												expr* key = $4;
												$$ = make_member_access(table, key);
											}
	| call DOT ID   {	
						expr* t = newexpr(tableitem_e);
						t->sym = newtemp();
						t->index = newexpr_conststring($3);
						t->type = arithexpr_e;
						emit(tablegetelem, $1, t->index, t, 0, yylineno);
						$$ = t;
					}
	| call LEFT_BRACKET expr RIGHT_BRACKET {
			expr* table = emit_iftableitem($1);
			expr* key = $3;
			$$ = make_member_access(table, key);
		}
	;

call:	call L_BRACKET elist R_BRACKET	{
			expr* func = $1;
			expr* args = $3;
			while(args) {
				emit(param, args, NULL, NULL, 0, yylineno);
				args = args->next;
			}
			expr* result = newexpr(var_e);
			result->sym = newtemp();
			emit(call_, func, NULL, NULL, 0, yylineno);
			emit(getretval, NULL, NULL, result, 0, yylineno);
			$$ = result;
			is_function_result = 1;
		}
	| call L_BRACKET R_BRACKET	{
			expr* func = $1;
			expr* result = newexpr(var_e);
			result->sym = newtemp();
			emit(call_, func, NULL, NULL, 0, yylineno);
			emit(getretval, NULL, NULL, result, 0, yylineno);
			$$ = result;
			is_function_result = 1;
		}
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
					//phase3
					//expr* func = $1;
    				//expr* args = $3;
					expr* func_e = get_expression(prev_id);
					if (!func_e) {
						printf("[CALL] ERROR: Function '%s' not found in symbol table (line: %d)\n", prev_id, yylineno);
						func_e = newexpr(programfunc_e);
						func_e->sym = newtemp();
					}
					/*
					expr* arg = args;
					while (arg) {
						emit(param, arg, NULL, NULL, 0, yylineno);
						arg = arg->next;
					}

					emit(call_, func, NULL, NULL, 0, yylineno);
					expr* result = newexpr(var_e);
					result->sym = newtemp();
					emit(getretval, NULL, NULL, result, 0, yylineno);
					$$ = result;
					*/
					
					$$ = make_call(func_e, $3);
				}
				is_member=0;

				strcpy(assign_id[assign_id_index], ""); // Clear assign_id so assignexpr knows this is a call
				
				//$$ = make_call(func_e, $3);
			}						
							
	| L_BRACKET funcdef R_BRACKET L_BRACKET {inside_function_call=1;} elist R_BRACKET {is_function_result = 1;}

	| L_BRACKET anon_funcdef R_BRACKET L_BRACKET elist R_BRACKET {
																	expr* args = $5;
																	expr* func = $2;
																	while(args){
																		emit(param, args, NULL, NULL, 0, yylineno);
																		args = args->next;
																	}
																	expr* result = newexpr(var_e);
																	result->sym = newtemp();

																	//emit(call_, func->sym ? func : newexpr(programfunc_e), NULL, NULL, 0, yylineno);
																	emit(call_, func, NULL, NULL, 0, yylineno);
																	emit(getretval, NULL, NULL, result, 0, yylineno);

																	$$ = result;

																	is_function_result = 1;
																}

	;

callsuffix:	funxcall		{ $$ = $1; }
		| method_funx_call 	{ $$ = NULL; }
		;

funxcall: 	normcall		{ $$ = $1; }
		| normcall_empty	{ $$ = NULL; }
		;

method_funx_call: methodcall		{}
		| methodcall_empty	{}
		;

normcall:	L_BRACKET elist R_BRACKET		{ $$ = $2; }
		| L_BRACKET elist R_BRACKET normcall	{ $$ = $2; }
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

objectdef:	LEFT_BRACKET {strcpy(objct_id, prev_id);} elist RIGHT_BRACKET	{
								expr* t = emit_tablecreate();
								expr* cur = $3;
								int i = 0;
								while (cur) {
									emit_tablesetelem(t, newexpr_constnum(i++), cur);
									cur = cur->next;
								}
								$$ = t;
							}
		| LEFT_BRACKET {strcpy(objct_id, prev_id);} indexed RIGHT_BRACKET	{
																				expr* t = emit_tablecreate();
																				expr* cur = $3;
																				while (cur && cur->next) {
																					emit_tablesetelem(t, cur->index, cur->next);  // result = t, arg1 = key, arg2 = value
																					cur = cur->next->next;
																				}
																				$$ = t;
																			}
		| LEFT_BRACKET {strcpy(objct_id, prev_id);} RIGHT_BRACKET	{$$ = emit_tablecreate();}
		;

elist:	 expr {declaration_check(3,0,1); $$ = $1; }
	| expr {declaration_check(3,0,1);} COMMA elist 	{
		$1->next = $4;
    	$$ = $1;
	}
	;

indexed:	indexedelem			{$$ = $1;}
		| indexedelem COMMA indexed	{
										$1->next->next = $3;
									 	$$ = $1;
									}
		;

indexedelem:	L_CURLY_BR expr COLON expr R_CURLY_BR		{
																expr* key = $2;
																expr* value = $4;
																
																 if (!value->sym && value->type == conststring_e) {
																	value->sym = newtemp();
																	value->sym->name = value->strConst;
																}
																
																expr* pair = newexpr(tableitem_e);
																pair->index = key;
																pair->next  = value;
																//value->next = NULL; //might remove
																$$ = pair;
															}
		;

block:		 lcurly_stmt_list R_CURLY_BR	{block_opened -= 1; $$ = $1;}
		;

lcurly_stmt_list:	L_CURLY_BR { scope += 1; block_opened += 1; } stmt_list { change_enabled_atscope(table, scope, 0); scope -= 1; $$ = $3; }
		;

stmt_list:	stmt	{ $$ = $1; }
		| stmt_list stmt {
			$$ = new_stmt();
			$$->breakList = mergelist($1->breakList, $2->breakList);
			$$->contList = mergelist($1->contList, $2->contList);
		}
		;

block_empty:	L_CURLY_BR R_CURLY_BR
		;
		
code_block:	block
		| block_empty
		;

funcdef:    FUNCTION ID {
							function_nesting_level++;
							if (inside_loop > 0) {
								break_continue_invalid_context = 1;
							}

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
								
								//phase3
								funcstart_jump_quad_index[inside_function_def] = nextquad();
								emit(jump, NULL, NULL, NULL, 0, yylineno); // leaving this with label: 0, but will change when the funcdef ends using funcstart_jump_quad_index
								expr* func_e = get_expression($2);
								if (!func_e) {
									printf("[ERROR] Function '%s' not declared (line: %d)\n", $2, yylineno);
									func_e = newexpr(programfunc_e);
									func_e->sym = newtemp();// Prevent segfault
								}
								emit(funcstart, NULL, NULL, func_e, 0, yylineno);
							
							}else{
								dont_do_it=0;
							}
							strcpy(arglist, "");
							strcpy(curr_func_name, $2);

						} R_BRACKET code_block {
							function_nesting_level--;
							if (function_nesting_level == 0){
								break_continue_invalid_context = 0;
							}

							in_anon_funx_def = 0;
							$$ = newexpr(programfunc_e);
							for(int i=0; i<=scope; i++){
								Symbol* sym  = extractSymbol(symSearch(table, prev_id, "USER FUNCTION", i));
								if(sym!=NULL){
									$$->sym = sym;
								}
							}
							//$$->sym = extractSymbol(symSearch(table, prev_id, "USER FUNCTION", scope));
							//phase3
							expr* func_e = get_expression($2);
							if (!func_e) {
								printf("[ERROR] Function '%s' not declared (line: %d)\n", $2, yylineno);
								func_e = newexpr(programfunc_e);
								func_e->sym = newtemp();// Prevent segfault
							}
							int funcend_quad_index = nextquad();
							emit(funcend, NULL, NULL, func_e, 0, yylineno);

							for(int i=0; i<inside_function_def; i++){
								patchlabel(funcstart_jump_quad_index[inside_function_def], funcend_quad_index+1);
								funcstart_jump_quad_index[inside_function_def]=0;
							}
							//patching return quads labels
							for(int i=0; i<return_quads[inside_function_def]; i++){
									patchlabel(return_quad_indices[i][inside_function_def], funcend_quad_index);
									return_quad_indices[i][inside_function_def]=0;
							}
							return_quads[inside_function_def]=0;
							inside_function_def-=1;
							/*	
							patchlabel(funcstart_jump_quad_index, nextquad());
							funcstart_jump_quad_index=0; //resetting the index
							*/
			;}
		;
	
anon_funcdef:	 FUNCTION L_BRACKET {scope+=1;} idlist {
									function_nesting_level++;

									if (inside_loop > 0) {
										break_continue_invalid_context = 1;
									}

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
									strcpy(recent_function_name_from_funcdef, prev_id);

									strcpy(funcdef_names[inside_function_def], recent_function_name_from_funcdef); //saving this to set its funcend quad properly

									handle_token(0);
									strcpy(arglist,"");

									//phase3
									funcstart_jump_quad_index[inside_function_def] = nextquad();
									emit(jump, NULL, NULL, NULL, 0, yylineno); // leaving this with label: 0, but will change when the funcdef ends using funcstart_jump_quad_index
									expr* func_e = get_expression(prev_id);
									emit(funcstart, NULL, NULL, func_e, 0, yylineno);
									
								}R_BRACKET code_block {
									function_nesting_level--;

									if (function_nesting_level == 0){
										break_continue_invalid_context = 0;
									}

									//phase3
									expr* func_e = get_expression(funcdef_names[inside_function_def]);
									int funcend_quad_index = nextquad();
									emit(funcend, NULL, NULL, func_e, 0, yylineno);

									for(int i=0; i<inside_function_def; i++){
										patchlabel(funcstart_jump_quad_index[inside_function_def], nextquad());
										funcstart_jump_quad_index[inside_function_def]=0;
									}
									//patching return quads labels
									for(int i=0; i<return_quads[inside_function_def]; i++){
										patchlabel(return_quad_indices[i][inside_function_def], funcend_quad_index);
										return_quad_indices[i][inside_function_def]=0;
									}
									return_quads[inside_function_def]=0;
									/*
									patchlabel(funcstart_jump_quad_index, nextquad());
									funcstart_jump_quad_index=0;
									*/
									//phase2
									in_anon_funx_def=0; 
									inside_function_def-=1;

									$$ = newexpr(programfunc_e);
									for(int i=0; i<=scope; i++){
											Symbol* sym  = extractSymbol(symSearch(table, prev_id, "USER FUNCTION", i));
											if(sym!=NULL){
													$$->sym = sym;
											}
									}
								}
		;

anon_funcdef_assign:	lvalue  ASSIGN {/*TODO should this be inserted?*/} anon_funcdef		{/*N/A*/}	
			;



const:		INT	{$$ = newexpr_constnum($1);}
		| DOUBLE	{$$ = newexpr_constnum($1);}
		| STRING	{$$ = newexpr_conststring($1);}
		| NIL	{$$ = newexpr(nil_e);}
		| TRUE	{$$ = newexpr_constbool(1);}
		| FALSE	{$$ = newexpr_constbool(0);}
		;

idlist	:	ID					{is_member=0; strcpy(prev_id,$1);  handle_token(2); strcat(arglist,"("); strcat(arglist, $1); strcat(arglist,") "); }
		| idlist COMMA ID			{is_member=0; strcpy(prev_id, $3);  handle_token(2); strcat(arglist,"("); strcat(arglist, $3); strcat(arglist,") ");}
		| {}				
		;


ifprefix:
    IF L_BRACKET { scope++; } expr R_BRACKET {
        scope--;
        expr* cond = make_bool_if_needed($4);

        // Patch true list immediately to nextquad (start of THEN)
        patchlist(cond->truelist, nextquad());
        $$ = cond;
    };

ifstmt:
    ifprefix stmt elsestmt {
        $$ = new_stmt();

        if ($3) {
            // Patch the jump that skips the else block (emitted in elsestart)
            patchlabel($3->quad_start - 1, nextquad());

            // Patch the falselist (if false) -> start of else
            patchlist($1->falselist, $3->quad_start);
        } else {
            patchlist($1->falselist, nextquad());
        }
    }
;

elsestmt:    ELSE elsestart stmt    {$$ = new_stmt(); $$->quad_start = $2;} // afto einai to 5o jummp
        |        {$$ = NULL;}
        ;

elsestart:    {emit(jump,NULL, NULL, NULL, 0, yylineno); $$=nextquad();} ;


whilestart: { $$ = nextquad(); } ;

whilecond:
    L_BRACKET expr R_BRACKET {
        expr* cond = make_bool_if_needed($2);

        if (cond->truelist && cond->falselist) {
            $$ = cond;
        } else {
            expr* temp_bool = newexpr(var_e);
            temp_bool->sym = newtemp();

            unsigned trueLabel = nextquad();
            emit(assign, newexpr_constbool(1), NULL, temp_bool, 0, yylineno);

            unsigned jumpToFalse = nextquad();
            emit(jump, NULL, NULL, NULL, 0, yylineno);

            unsigned falseLabel = nextquad();
            emit(assign, newexpr_constbool(0), NULL, temp_bool, 0, yylineno);

            unsigned after = nextquad();
            patchlist(cond->truelist, trueLabel);
            patchlist(cond->falselist, falseLabel);
            patchlabel(jumpToFalse, after);

            expr* wrapper = newexpr(boolexpr_e);
            wrapper->sym = newtemp();
            emit(if_eq, temp_bool, newexpr_constbool(1), NULL, nextquad() + 2, yylineno);
            emit(jump, NULL, NULL, NULL, 0, yylineno);

            wrapper->truelist = new_list(nextquad() - 2);
            wrapper->falselist = new_list(nextquad() - 1);
            $$ = wrapper;
        }
    }


;

whilestmt:
    WHILE { inside_loop++; }whilestart whilecond stmt {
        emit(jump, NULL, NULL, NULL, $3, yylineno); // loop back to start
        patchlist($4->falselist, nextquad()); // exit loop
		patchlist($5->breakList, nextquad()); // BREAK
		patchlist($5->contList, $3); //CONTINUE
		inside_loop--;
        $$ = new_stmt();
    }

;

forprefix:
    FOR { inside_loop++; }L_BRACKET elist semicolon N expr semicolon N elist R_BRACKET M{ // true part of for where the loop happens
        expr* cond = make_bool_if_needed($7);
        patchlist(cond->truelist, nextquad());
        
        cond->true_jump_quad = $6;   // start of condition
        cond->false_jump_quad = nextquad(); // label after the loop
        cond->step_quad = $9;        // start of step
        cond->jump_to_step = $12;    // jump after loop body -> step
        $$ = cond;
    }
;

forstmt:
    forprefix stmt {
        $$ = new_stmt();
        
        expr* cond = $1;
        patchlist($1->falselist, nextquad()+1); // condition false -> after loop
        patchlabel(cond->jump_to_step - 1, cond->true_jump_quad);
        emit(jump, NULL, NULL, NULL, cond->step_quad, yylineno); // to teleftaio jump back sto vima
		inside_loop--;
};

M: { emit(jump, NULL, NULL, NULL, 0, yylineno); $$ = nextquad();  };// from end of stmt to step}; to 10o jump to messaio
N: { $$ = nextquad();
};


/* unhinged */

//for_stuff: 	L_BRACKET elist semicolon expr semicolon elist R_BRACKET {scope = scope - 1;}stmt
//		| L_BRACKET semicolon expr semicolon elist R_BRACKET {scope = scope - 1;}stmt
//		| L_BRACKET elist semicolon semicolon elist R_BRACKET {scope = scope - 1;}stmt
//		| L_BRACKET elist semicolon expr semicolon R_BRACKET {scope = scope - 1;}stmt
//		| L_BRACKET semicolon expr semicolon R_BRACKET {scope = scope - 1;}stmt
//		| L_BRACKET elist semicolon  semicolon  R_BRACKET {scope = scope - 1;}stmt
//		| L_BRACKET semicolon  semicolon elist R_BRACKET {scope = scope - 1;}stmt
//		| L_BRACKET semicolon semicolon R_BRACKET {scope = scope - 1;}stmt
//		;
		

semicolon: SEMICOLON{
	really_useful_index = 0;
	inside_function_call=0; //after a semicolon a function call is surely over
}
;
returnstmt:	RETURN semicolon {
		if (function_nesting_level == 0) {
			fprintf(stderr, "[ERROR] 'return' used outside of any function at line %d\n", yylineno);
			exit(1);
		}
		current_return_expr = NULL;
	}
		|  RETURN expr semicolon {
				if (function_nesting_level == 0) {
					fprintf(stderr, "[ERROR] 'return' with value used outside of any function at line %d\n", yylineno);
					exit(1);
				}
				if (istempexpr($2)) {
					current_return_expr = $2;
				} else {
					current_return_expr = newexpr(var_e);
					current_return_expr->sym = newtemp();
					emit(ret, $2, NULL, NULL, 0, yylineno);

					return_quad_indices[return_quads[inside_function_def]] [inside_function_def] = nextquad();
					return_quads[inside_function_def]+=1;
					emit(jump, NULL, NULL, NULL, 0, yylineno);
				}
			}
		;

%%

int main(int argc,char** argv){

    table = (symTable *)malloc(sizeof(symTable)*10);
    stsize = 0;
    scope = 0;
    anon_funx_count=0;
    is_member=0;
	int arg = 0;
	emit(jump, NULL, NULL, NULL, 0, 0); // emits dummy quad at #0

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
	quad_output = fopen("quads.txt", "w");
	if (!quad_output) {
		fprintf(stderr, "Cannot open quads.txt for writing.\n");
		return 1;
	}
	patch_all_jumps();
	print_quads();
    fclose(yyin_o);
	fclose(quad_output);


    return 0;
}