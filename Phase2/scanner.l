/*
csd4335 Mixalhs Grigorakhs
csd4831 Kwstantinos Garazanakhs
csd4603 Ioannis Vlahodimos 
*/

/*Definitions Segment*/
%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h" /*Will be generated from parser.y*/

//#define YY_DECL int alpha_yylex(void*)

typedef struct alpha_token_t{
    unsigned int numline;
    unsigned int numtoken;
    char *content;/*"if","16"*/
    char *type;/*KEYWORD,IDENT*/
    char *name;/*content in CAPS*/
    struct alpha_token_t *next;
}alpha_token_t;


void print_tokens(int numline, int numtoken, char *content, char *type,char* name);
int numtoken = 0;
%}
%option outfile = "scanner.c"
%option noyywrap
%option yylineno

/*Macros*/
identifier    ([a-z|A-Z|0-9|_]+)
integer       (0[xX][0-9A-Fa-f]+)|([0-9]+)
real      [0-9]*\.[0-9](e|E)?[0-9]*
string       \"[^\n"]*\" 
newline     "\n"
space       [\r \t\v]

%x MLCOMMENT
%x NESTEDCOMMENT
%x PRINTNESTED
%x SLCOMMENT

/*Rule Section*/
/* gia kapoio logo prin klh8ei to print() sthn main,
o lexer ektupwnei blank lines,spaces,tabs kai characters xwris rules tou test.txt
ara pros8etoume ena extra rule gia thn periptwsh auth sthn opoia kanei ignore
(to rule me . to vazw sto telos)
*/

%%

"\n"    {/*ignore*/}
"\t"    {/*ignore*/}
"\s"	{/*ignore*/}
"\v"	{/*ignore*/}
"\r"	{/*ignore*/}
" "	{/*ignore*/}


<<EOF>> {return 0;}

[a-z|A-Z]   {
                print_tokens(yylineno,++numtoken,yytext,"IDENT","IDENTIFIER_NAME");
                yylval.stringValue = strdup(yytext);
                return ID;
            }

"if"    {
            print_tokens(yylineno,++numtoken,yytext,"KEYWORD","IF");
            yylval.stringValue = strdup(yytext);
            return IF;
        }
"else"  {
            print_tokens(yylineno,++numtoken,yytext,"KEYWORD","ELSE");
            yylval.stringValue = strdup(yytext);
            return ELSE;
        }
"while" {
            print_tokens(yylineno,++numtoken,yytext,"KEYWORD","WHILE");
            yylval.stringValue = strdup(yytext);    
            return WHILE;
        }
"for"   {
            print_tokens(yylineno,++numtoken,yytext,"KEYWORD","FOR");
            yylval.stringValue = strdup(yytext);    
            return FOR;
        }
"function"  {
                print_tokens(yylineno,++numtoken,yytext,"KEYWORD","FUNCTION");
                yylval.stringValue = strdup(yytext);    
                return FUNCTION;
            }
"return"    {
                print_tokens(yylineno,++numtoken,yytext,"KEYWORD","RETURN");
                yylval.stringValue = strdup(yytext);
                return RETURN;
            }
"break"     {
                print_tokens(yylineno,++numtoken,yytext,"KEYWORD","BREAK");
                yylval.stringValue = strdup(yytext);
                return BREAK;
            }
"continue"  {
                print_tokens(yylineno,++numtoken,yytext,"KEYWORD","CONTINUE");
                yylval.stringValue = strdup(yytext);
                return CONTINUE;
            }
"and"       {
                print_tokens(yylineno,++numtoken,yytext,"KEYWORD","AND");
                yylval.stringValue = strdup(yytext);
                return AND;
            }
"not"       {
                print_tokens(yylineno,++numtoken,yytext,"KEYWORD","NOT");
                yylval.stringValue = strdup(yytext);
                return NOT;
            }
"or"        {
                print_tokens(yylineno,++numtoken,yytext,"KEYWORD","OR");
                yylval.stringValue = strdup(yytext);
                return OR;
            }
"local"     {
                print_tokens(yylineno,++numtoken,yytext,"KEYWORD","LOCAL");
                yylval.stringValue = strdup(yytext);
                return LOCAL;
            }
"true"      {
                print_tokens(yylineno,++numtoken,yytext,"KEYWORD","TRUE");
                yylval.stringValue = strdup(yytext);
                return TRUE;
            }
"false"     {
                print_tokens(yylineno,++numtoken,yytext,"KEYWORD","FALSE");
                yylval.stringValue = strdup(yytext);
                return FALSE;
            }
"nil"       {
                print_tokens(yylineno,++numtoken,yytext,"KEYWORD","NIL");
                yylval.stringValue = strdup(yytext);
                return NIL;
            }
"="         {
                print_tokens(yylineno,++numtoken,yytext,"OPERATOR","ASSIGN");
                yylval.stringValue = strdup(yytext);
                return ASSIGN;
            }
"+"         {
                print_tokens(yylineno,++numtoken,yytext,"OPERATOR","PLUS");
                yylval.stringValue = strdup(yytext);
                return PLUS;
            }
"-"         {
                print_tokens(yylineno,++numtoken,yytext,"OPERATOR","MINUS");
                yylval.stringValue = strdup(yytext);
                return MINUS;
            }
"*"         {
                print_tokens(yylineno,++numtoken,yytext,"OPERATOR","MULTIPLY");
                yylval.stringValue = strdup(yytext);
                return MULT;
            }
"/"         {
                print_tokens(yylineno,++numtoken,yytext,"OPERATOR","DIVISION");
                yylval.stringValue = strdup(yytext);
                return DIV;
            }
"%"         {
                print_tokens(yylineno,++numtoken,yytext,"OPERATOR","MOD");
                yylval.stringValue = strdup(yytext);
                return MOD;
            }
"=="        {
                print_tokens(yylineno,++numtoken,yytext,"OPERATOR","EQUALS");
                yylval.stringValue = strdup(yytext);
                return EQ;
            }
"!="        {
                print_tokens(yylineno,++numtoken,yytext,"OPERATOR","NOT_EQUAL");
                yylval.stringValue = strdup(yytext);
                return NE;
            }
"++"        {
                print_tokens(yylineno,++numtoken,yytext,"OPERATOR","INCREMENT");
                yylval.stringValue = strdup(yytext);
                return PLUS_PLUS;
            }
"--"        {
                print_tokens(yylineno,++numtoken,yytext,"OPERATOR","DECREMENT");
                yylval.stringValue = strdup(yytext);
                return MINUS_MINUS;
            }
">"         {
                print_tokens(yylineno,++numtoken,yytext,"OPERATOR","GREATERTHAN");
                yylval.stringValue = strdup(yytext);
                return GT;
            }
"<"         {
                print_tokens(yylineno,++numtoken,yytext,"OPERATOR","LESSTHAN");
                yylval.stringValue = strdup(yytext);
                return LT;
            }
">="        {
                print_tokens(yylineno,++numtoken,yytext,"OPERATOR","GREATEREQUAL");
                yylval.stringValue = strdup(yytext);
                return GE;
            }
"<="        {
                print_tokens(yylineno,++numtoken,yytext,"OPERATOR","LESSEQUAL");
                yylval.stringValue = strdup(yytext);
                return LE;
            }
{integer}       {
                print_tokens(yylineno,++numtoken,yytext,"IDENT","INTEGER");
                yylval.intValue = atoi(yytext);/*Converting string to integer*/
                return INT;
            }
{real}     {
                print_tokens(yylineno,++numtoken,yytext,"IDENT","FLOAT");
                yylval.realValue = atof(yytext);/*Converting string to real*/
                return DOUBLE;
            }
{string}	{
			print_tokens(yylineno,++numtoken,yytext,"IDENT","STRING");
			yylval.stringValue = strdup(yytext);
			return STRING;
		}

{identifier}      {
                print_tokens(yylineno,++numtoken,yytext,"IDENT","IDENTIFIER_NAME");
                yylval.stringValue = strdup(yytext);
                return ID;
            }
"{"     {
                print_tokens(yylineno,++numtoken,yytext,"PUNCTUATION","L_CURLY_BR");
                yylval.stringValue = strdup(yytext);
                return L_CURLY_BR;
            }
"}"     {
                print_tokens(yylineno,++numtoken,yytext,"PUNCTUATION","R_CURLY_BR");
                yylval.stringValue = strdup(yytext);
                return R_CURLY_BR;
            }
"["     {
                print_tokens(yylineno,++numtoken,yytext,"PUNCTUATION","L_SQUARE_BR");
                yylval.stringValue = strdup(yytext);
                return LEFT_BRACKET;
            }
"]"     {
                print_tokens(yylineno,++numtoken,yytext,"PUNCTUATION","R_SQUARE_BR");
                yylval.stringValue = strdup(yytext);
                return RIGHT_BRACKET;
            }
"("     {
                print_tokens(yylineno,++numtoken,yytext,"PUNCTUATION","L_BRACKET");
                yylval.stringValue = strdup(yytext);
                return L_BRACKET;
            }
")"     {
                print_tokens(yylineno,++numtoken,yytext,"PUNCTUATION","R_BRACKET");
                yylval.stringValue = strdup(yytext);
                return R_BRACKET;
            }
";"     {
                print_tokens(yylineno,++numtoken,yytext,"PUNCTUATION","SEMICOLON");
                yylval.stringValue = strdup(yytext);
                return SEMICOLON;
            }
","     {
                print_tokens(yylineno,++numtoken,yytext,"PUNCTUATION","COMMA");
                yylval.stringValue = strdup(yytext);
                return COMMA;
            }
":"     {
                print_tokens(yylineno,++numtoken,yytext,"PUNCTUATION","COLON");
                yylval.stringValue = strdup(yytext);
                return COLON;
            }
"::"    {
                print_tokens(yylineno,++numtoken,yytext,"PUNCTUATION","DOUBLE_COLON");
                yylval.stringValue = strdup(yytext);
                return DOUBLE_COLON;
            }
"."     {
                print_tokens(yylineno,++numtoken,yytext,"PUNCTUATION","DOT");
                yylval.stringValue = strdup(yytext);
                return DOT;
            }
".."    {
                print_tokens(yylineno,++numtoken,yytext,"PUNCTUATION","DOUBLE_DOT");
                yylval.stringValue = strdup(yytext);
                return DOUBLE_DOT;
            }
"!"    {
                print_tokens(yylineno,++numtoken,yytext,"PUNCTUATION","EXCLAMATION");
                yylval.stringValue = strdup(yytext);
                return EXCLAMATION;
            }


"//"|"#"	{
			print_tokens(yylineno,++numtoken,yytext,"COMMENT","SINGLE LINE");
			yylval.stringValue = strdup(yytext);
			BEGIN(SLCOMMENT);
			
			//return COMMENT; // Removed bc parser is not made to handle comments
		}
<SLCOMMENT>.*	{
			/*ignore*/
		}
<SLCOMMENT>.*\n	{
			BEGIN(INITIAL);
		}


"/*"        {   
                BEGIN(MLCOMMENT);
            }
            
<MLCOMMENT>.|\n	{
			/*ignore*/
		}
<MLCOMMENT>"*/"	{
                print_tokens(yylineno,++numtoken,yytext,"COMMENT","MULTILINE COMMENT");
			BEGIN(INITIAL);
			//return COMMENT; //Removed bc parser is not made to handle comments
		}
<MLCOMMENT>"/*"		{
				BEGIN(NESTEDCOMMENT);	
			}	
			
<NESTEDCOMMENT>.|\n	{
				/*ignore*/
			}
			
<NESTEDCOMMENT>"*/"	{
				print_tokens(yylineno,++numtoken,yytext,"COMMENT","MULTILINE COMMENT");
				BEGIN(MLCOMMENT);
				//return COMMENT; //Removed bc parser is not made to handle comments
			}							
        


%%

struct token_list{
    alpha_token_t *head;
    alpha_token_t *tail;
} tokens;




/*Inserting a token in the token list*/
void insert_token(alpha_token_t *new_token){
    if(tokens.head == NULL){
        tokens.head = new_token;
    }else if(tokens.tail == NULL){
        tokens.tail = new_token;
        tokens.head->next = new_token;
    }else{
        tokens.tail->next = new_token;
        tokens.tail = new_token;
    }
}

void print_tokens(int numline, int numtoken, char *content, char *type,char *name){
    FILE *fp;
    fp = fopen("output_lex.txt","a+");
    if((strcmp(type,"STRING") == 0)){
        fprintf(fp,"%u: \t #%u \t '%s' \t\t\t %s \t\t %s \t\n",numline, numtoken, content, type, name);
    }
    fclose(fp);

/*
->numline = numline;
->numtoken = numtoken;
->content = (char*)malloc((strlen(content) + 1)*sizeof(char));
    strcpy->content,content);
->content[strlen(content)] = '\0';
->type = type;
->name = name;
->next = NULL;
*/
}

/*Prints the struct token list*/
void print(){
    alpha_token_t *head = tokens.head;

    printf("Printing:\n");
    //printf("%u\n",head->numline);
    while(head != NULL){
        printf("%u: \t #%u \t '%s' \t\t\t %s \t\t %s \t\n",head->numline, head->numtoken, head->content, head->type, head->name);
        head = head->next;
    }
}


