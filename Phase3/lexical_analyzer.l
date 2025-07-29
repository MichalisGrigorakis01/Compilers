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
    
    #define YY_DECL int alpha_yylex(void* yylval)
    
    typedef struct alpha_token_t{
        unsigned int numline;
        unsigned int numtoken;
        char *content;/*"if","16"*/
        char *type;/*KEYWORD,IDENT*/
        char *name;/*content in CAPS*/
        struct alpha_token_t *next;
    }alpha_token_t;
    
    void insert(int numline, int numtoken, char *content, char *type,char* name, alpha_token_t *yylval);
    int numtoken = 0;
    
    //comment data
    char* comment;

    //variable to keep multiline comment's data
    int comment_start_line=0;
    int comments_expected=0;

    //variable to check strings
    int strings_expected = 0; // Tracks unclosed strings
    int string_start_line = 0; // Stores the line where an unclosed string starts
    int escape_char_expected=0;
    char* curr_str;
    
    //variables to keep the nested comment's data
    int nested_comment_start_line=0;
    int nested_numtoken=0;
    char  nested_info_string[100];
    int nested=0; //1 if there is 0 if not
    
    
    //function to handle escape characters (the quotes are already removed)
    char* parse_string(char* str) {
        int len = strlen(str);
        char escape_chars[] = {'n', 't', '\\', '"','\"'}; // Allowed escape sequences
        int escape_char_count = sizeof(escape_chars) / sizeof(escape_chars[0]);

        // Process escape sequences
        for (int i = 0; str[i] != '\0'; i++) {
            if (str[i] == '\\') {
                if (str[i + 1] == '\0') {
                    return str; // Do NOT print an error here; let main() handle it.
                }
    
                // Check if it's a valid escape character
                int valid_escape = 0;
                for (int j = 0; j < escape_char_count; j++) {
                    if (str[i + 1] == escape_chars[j]) {
                        valid_escape = 1;
                        break;
                    }
                }
    
                if (!valid_escape) {
                    printf("[ERROR]: parse_string: Illegal escape sequence \\%c at line %d.\n", str[i + 1], yylineno);
                    return str;
                }
    
                // Convert escape sequences
                if (str[i + 1] == 'n') str[i] = '\n';
                else if (str[i + 1] == 't') str[i] = '\t';
                else if (str[i + 1] == '\\') str[i] = '\\';
                else if (str[i + 1] == '"') str[i] = '"';
    
                // Shift the rest of the string
                memmove(str + i + 1, str + i + 2, strlen(str + i + 2) + 1);
            }
        }
    
        return str;
    }
    
    // Structure for error storage
    typedef struct error_node {
        int line;
        char message[100];
        struct error_node *next;
    } error_node;
    
    error_node *error_head = NULL;  // Head of the error list
    
    void add_error(int line, const char *message) {
        error_node *new_error = (error_node *)malloc(sizeof(error_node));
        if (!new_error) return;
        
        new_error->line = line;
        strncpy(new_error->message, message, sizeof(new_error->message));
        new_error->message[sizeof(new_error->message) - 1] = '\0';
        new_error->next = NULL;
        
        // Insert into error list
        if (!error_head) {
            error_head = new_error;
        } else {
            error_node *temp = error_head;
            while (temp->next) temp = temp->next;
            temp->next = new_error;
        }
    }

    // Function to print all stored errors
    void print_errors() {
        error_node *temp = error_head;
        while (temp) {
            printf("%s\n", temp->message);
            temp = temp->next;
        }
    }
     
%}
%option outfile = "alpha_yylex.c"
%option noyywrap
%option yylineno

/*Macros*/
identifier    ([a-z|A-Z|0-9|_]+)
integer       (0[xX][0-9A-Fa-f]+)|([0-9]+)
real      [0-9]*\.[0-9](e|E)?[0-9]*
space       [\r \t\v]

%x MLCOMMENT
%x NESTEDCOMMENT
%x PRINTNESTED
%x SLCOMMENT
%x STRING_STATE
%x ESCAPE_CHAR

%%

"\n"    {/*ignore*/}
"\t"    {/*ignore*/}

"&&"|"||"   {
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "[ERROR]: Not accepted logical operator -> '%s' in line %d.", yytext, yylineno);
        add_error(yylineno, buffer);
    }

"_"[a-zA-Z0-9_]* {
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "[ERROR]: Illegal identifier -> '%s' in line %d.", yytext, yylineno);
        add_error(yylineno, buffer);
    }

[a-z|A-Z]   {insert(yylineno,++numtoken,yytext,"IDENT","IDENTIFIER",yylval);return 1;}

"if"    {insert(yylineno,++numtoken,yytext,"KEYWORD","IF",yylval);return 1;}
"else"  {insert(yylineno,++numtoken,yytext,"KEYWORD","ELSE",yylval);return 1;}
"while" {insert(yylineno,++numtoken,yytext,"KEYWORD","WHILE",yylval);return 1;}
"for"   {insert(yylineno,++numtoken,yytext,"KEYWORD","FOR",yylval);return 1;}
"function"  {insert(yylineno,++numtoken,yytext,"KEYWORD","FUNCTION",yylval);return 1;}
"return"    {insert(yylineno,++numtoken,yytext,"KEYWORD","RETURN",yylval);return 1;}
"break"     {insert(yylineno,++numtoken,yytext,"KEYWORD","BREAK",yylval);return 1;}
"continue"  {insert(yylineno,++numtoken,yytext,"KEYWORD","CONTINUE",yylval);return 1;}
"and"       {insert(yylineno,++numtoken,yytext,"KEYWORD","AND",yylval);return 1;}
"not"       {insert(yylineno,++numtoken,yytext,"KEYWORD","NOT",yylval);return 1;}
"or"        {insert(yylineno,++numtoken,yytext,"KEYWORD","OR",yylval);return 1;}
"local"     {insert(yylineno,++numtoken,yytext,"KEYWORD","LOCAL",yylval);return 1;}
"true"      {insert(yylineno,++numtoken,yytext,"KEYWORD","TRUE",yylval);return 1;}
"false"     {insert(yylineno,++numtoken,yytext,"KEYWORD","FALSE",yylval);return 1;}
"nil"       {insert(yylineno,++numtoken,yytext,"KEYWORD","NIL",yylval);return 1;}
"="    {insert(yylineno,++numtoken,yytext,"OPERATOR","ASSIGN",yylval);return 1;}
"+"    {insert(yylineno,++numtoken,yytext,"OPERATOR","PLUS",yylval);return 1;}
"-"    {insert(yylineno,++numtoken,yytext,"OPERATOR","MINUS",yylval);return 1;}
"*"    {insert(yylineno,++numtoken,yytext,"OPERATOR","MULTIPLY",yylval);return 1;}
"/"    {insert(yylineno,++numtoken,yytext,"OPERATOR","DIVISION",yylval);return 1;}
"%"    {insert(yylineno,++numtoken,yytext,"OPERATOR","MOD",yylval);return 1;}
"=="    {insert(yylineno,++numtoken,yytext,"OPERATOR","EQUALS",yylval);return 1;}
"!="    {insert(yylineno,++numtoken,yytext,"OPERATOR","NOT_EQUAL",yylval);return 1;}
"++"    {insert(yylineno,++numtoken,yytext,"OPERATOR","INCREMENT",yylval); return 1;}
"--"    {insert(yylineno,++numtoken,yytext,"OPERATOR","DECREMENT",yylval); return 1; }
">"    {insert(yylineno,++numtoken,yytext,"OPERATOR","GREATERTHAN",yylval); return 1; }
"<"    {insert(yylineno,++numtoken,yytext,"OPERATOR","LESSTHAN",yylval);return 1;}
">="    {insert(yylineno,++numtoken,yytext,"OPERATOR","GREATEREQUAL",yylval);return 1;}
"<="    {insert(yylineno,++numtoken,yytext,"OPERATOR","LESSEQUAL",yylval);return 1;}
{integer}       {insert(yylineno,++numtoken,yytext,"VALUE","INTEGER",yylval);return 1;}
{real}     {insert(yylineno,++numtoken,yytext,"VALUE","FLOAT",yylval);return 1;}
{identifier}      {insert(yylineno,++numtoken,yytext,"IDENT","IDENTIFIER",yylval);return 1;}
"{"     {insert(yylineno,++numtoken,yytext,"PUNCTUATION"," L_CURLY_BR",yylval);return 1;}
"}"     {insert(yylineno,++numtoken,yytext,"PUNCTUATION"," R_CURLY_BR",yylval);return 1;}
"["     {insert(yylineno,++numtoken,yytext,"PUNCTUATION"," L_SQUARE_BR",yylval);return 1;}
"]"     {insert(yylineno,++numtoken,yytext,"PUNCTUATION"," R_SQUARE_BR",yylval);return 1;}
"("     {insert(yylineno,++numtoken,yytext,"PUNCTUATION"," L_BRACKET",yylval);return 1;}
")"     {insert(yylineno,++numtoken,yytext,"PUNCTUATION"," R_BRACKET",yylval);return 1;}
";"     {insert(yylineno,++numtoken,yytext,"PUNCTUATION","SEMICOLON",yylval);return 1;}
","     {insert(yylineno,++numtoken,yytext,"PUNCTUATION","COMMA",yylval);return 1;}
":"     {insert(yylineno,++numtoken,yytext,"PUNCTUATION","COLON",yylval);return 1;}
"::"    {insert(yylineno,++numtoken,yytext,"PUNCTUATION","DOUBLE_COLON",yylval);return 1;}
"."     {insert(yylineno,++numtoken,yytext,"PUNCTUATION","DOT",yylval);return 1;}
".."    {insert(yylineno,++numtoken,yytext,"PUNCTUATION","DOUBLE_DOT",yylval);return 1;}


"//"		{
            insert(yylineno,++numtoken,yytext,"COMMENT","SINGLE LINE",yylval);
            BEGIN(SLCOMMENT);
            return 1;
        }
<SLCOMMENT>.*	{
            //ignore?
        }
<SLCOMMENT>\n	{
            BEGIN(INITIAL);
        }


"/*"        {  
        comment_start_line=yylineno; 
        comments_expected+=1;
        
                BEGIN(MLCOMMENT);
            }
        
<MLCOMMENT>.|\n      {
                /*ignore*/
            }
<MLCOMMENT>"*/"    {
            char result[50];
            snprintf(result, sizeof(result),"%s %d %s %d","MULTI LINE", comment_start_line, "-", yylineno);
                        insert(comment_start_line,++numtoken,result,"COMMENT","MULTI LINE",yylval);
                        comments_expected-=1;
                        
                        BEGIN(INITIAL);
                        return 1;
                    }

<MLCOMMENT>"/*"		{
                nested_comment_start_line=yylineno;
                nested_numtoken=numtoken;
                comments_expected+=1;
                
                BEGIN(NESTEDCOMMENT);	
            }    

<NESTEDCOMMENT>"/*" {
                    comments_expected+=1;
                    
                }


<NESTEDCOMMENT>"*/"	{
                if(comments_expected>2){//if there are three or more nested
                    //TODO: insert these properly
                    //char result[50];
                    //snprintf(result, sizeof(result),"%s %d %s %d","MULTI LINE", nested_comment_start_line, "-", yylineno);
                    //insert(nested_comment_start_line,++numtoken,result,"NESTED COMMENT","MULTI LINE",yylval);
                    comments_expected-=1;
                }else{
                    char result[50];
                    snprintf(result, sizeof(result),"%s %d %s %d","MULTI LINE", nested_comment_start_line, "-", yylineno);
                    insert(nested_comment_start_line,++numtoken,result,"NESTED COMMENT","MULTI LINE",yylval);
                    BEGIN(MLCOMMENT);
                    comments_expected-=1;
                    
                    strcpy(nested_info_string,result);
                    return 1;
                }
            }      

<NESTEDCOMMENT>.|\n	{
                /*ignore*/
            }


\" {
    if (strings_expected == 0) {  
        string_start_line = yylineno; // Store the first unclosed string's line
    }
    strings_expected++;
    curr_str = malloc(1);  // Allocate memory
    *curr_str = '\0';          // Initialize empty string
    BEGIN(STRING_STATE);
}

<STRING_STATE>\\ {
    //expecting an escape character, so we will need to append the current character AND the next
    size_t new_len = strlen(curr_str) + strlen(yytext);
    curr_str = realloc(curr_str, new_len);
    strcat(curr_str, yytext);
    BEGIN(ESCAPE_CHAR);
}
<ESCAPE_CHAR>.|\n {
    //appending escape character whatever it may be
    size_t new_len = strlen(curr_str) + strlen(yytext);
    curr_str = realloc(curr_str, new_len);
    strcat(curr_str, yytext);
    //continuing with string
    BEGIN(STRING_STATE);
}

<STRING_STATE>[^"] {
    //for each symbol except " found, we append it to the curr_string
    size_t new_len = strlen(curr_str) + strlen(yytext);
    curr_str = realloc(curr_str, new_len); 
    strcat(curr_str, yytext);
    
}

<STRING_STATE>\" {
    strings_expected-=1;
    
    if (strings_expected == 0) {
        string_start_line = 0; // Reset tracking if all strings are closed
    }
    insert(yylineno, ++numtoken, parse_string(curr_str), "VALUE", "STRING", yylval);
    
    BEGIN(INITIAL);
    return 1;
}

<STRING_STATE><<EOF>> {
    printf("[ERROR]: main: %d string(s) not properly closed, starting from Line: %d.\n", strings_expected, string_start_line);
    BEGIN(INITIAL);
}


[^ \t\n]   {
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "[ERROR]: Not accepted character -> '%s' in line %d.", yytext, yylineno);
        add_error(yylineno, buffer);
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

void insert(int numline, int numtoken, char *content, char *type,char *name, alpha_token_t *yylval){
    yylval->numline = numline;
    yylval->numtoken = numtoken;
    yylval->content = (char*)malloc((strlen(content) + 1)*sizeof(char));
    strcpy(yylval->content,content);
    yylval->content[strlen(content)] = '\0';
    yylval->type = type;
    yylval->name = name;
    yylval->next = NULL;
}

/*Prints the struct token list*/
void print(){
    alpha_token_t *head = tokens.head;

    printf("Printing:\n");
    //printf("%u\n",head->numline);
    printf("%-6s %-10s %-100s %-20s %-10s\n", "Line", "Token", "Content", "Type", "Category");
    printf("------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    while(head != NULL){
        printf("%u:\t #%-10u %-100s %-20s %-10s\n",
            head->numline, head->numtoken, head->content, head->type, head->name);
        head = head->next;
    }
}

int main(int argc, char **argv){
    alpha_token_t *curr = NULL;
    
    if(argc > 1){
        if(!(yyin = fopen(argv[1],"r"))){
            fprintf(stderr,"Cannot read file: %s\n", argv[1]);
            return 1;
        }
    }else{
        yyin = stdin;
    }

    while(1){
        curr = (alpha_token_t *)malloc(sizeof(alpha_token_t));
        if(alpha_yylex(curr) == 0) break;
        else insert_token(curr);
    }
    print();
    print_errors();

    if (comments_expected!=0){
        printf("[ERROR]: main: %d comment(s) not properly closed, starting from Line: %d.\n", comments_expected,comment_start_line );
    }
    if (strings_expected > 0) {
        printf("[ERROR]: main: %d string(s) not properly closed, starting from Line: %d.\n", 
                strings_expected, string_start_line);
    }

    return 0;
}
