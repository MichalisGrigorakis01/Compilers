#ifndef YYERROR_DEFINED
#define YYERROR_DEFINED
#include <stdio.h>
int yylex();
extern int yylineno;
extern char* yytext;
extern FILE* yyin_o;




int yyerror (char* yaccProvidedMessage){
    fprintf(stderr,"%s: at line %d, before token: %s\n", yaccProvidedMessage,yylineno,yytext);
    fprintf(stderr,"INPUT NOT VALID\n");
}
	
#endif