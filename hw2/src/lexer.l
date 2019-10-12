%option noyywrap 
%{
#include <stdio.h>
#include "header.h" 
int linenumber = 0;
symtab * lookup();
symtab * ptr;
void     insertID();
void 	 printSymTab();
void 	 printFreq();
/* You need to define for all tokens in C--, here are some examples */
#define IDENTIFIER	7

#define INT		10
#define FLOAT 	11
#define STRING	12

#define MK_LPAREN 	20
#define MK_RPAREN 	21
#define MK_LBRACE	22
#define MK_RBRACE	23
#define MK_LBRACKET	24
#define MK_RBRACKET	25

#define OP_ASSIGN 	30

#define OP_PLUS		40 
#define OP_MINUS	41
#define OP_MULTI	42
#define OP_DIVIDE	43

#define OP_LESS		50
#define OP_GREATER	51
#define OP_LEQ		52
#define OP_GEQ		53
#define OP_NEQ		54
#define	OP_EQ		55

#define OP_OR	60
#define OP_AND	61
#define OP_NOT	62

#define RW_RETURN	70
#define RW_TYPEDEF	71
#define RW_IF		72
#define RW_ELSE		73
#define RW_INT		74
#define RW_FLOAT	75
#define RW_FOR		76
#define RW_VOID		77
#define RW_WHILE	78

#define ERROR 		100 

%}

letter   [A-Za-z]
digit    [0-9]
ID	 {letter}({letter}|{digit}|"_")*
WS	 [ \t]+
Int_constant {digit}+
Float_constant ({Int_constant}\.|\.{Int_constant}|{Int_constant}\.{Int_constant})([Ee][\+\-]?{Int_constant})?
String_constant \"([^\\\"]|\\.)*\"

Comment "/*"([^*]|\*+[^/\*])*"*"*"/"

/* You need to define the following RE's 
Float_constant
String_constant
comment
*/

OP_assign        "="

OP_plus   "+"
OP_minus  "-"
OP_multi  "*"
OP_divide "/"

OP_less    "<"
OP_greater ">"
OP_leq     "<="
OP_geq     ">="
OP_neq     "!="
OP_eq      "=="

OP_and  "&&"
OP_or   "||"
OP_not  "!"

/* Other operators appear here */ 


newline  "\n"

DL_lparen "("
DL_rparen ")"
DL_lbrace "{"
DL_rbrace "}"
DL_lbracket "["
DL_rbracket "]"
DL_comma  ","
DL_semicol ";"
DL_dot  "."

/* Other separators appear here */

error    .

%%

{WS}            {}
{Int_constant}         {/* return INT; */ }
{Float_constant}       {/* return FLOAT; */ }
{String_constant}		{/* return STRING; */ }
{Comment}		{
				char* find = strchr(yytext, '\n');
				while(find != NULL){
					linenumber += 1;
					find = strchr(find + 1, '\n');
				}
				printf("%s\n", yytext);
				}
{ID}            { 
			if(strcmp(yytext, "return") == 0){
				/* return RW_RETURN; */
			}else if(strcmp(yytext, "typedef") == 0){
				/* return RW_TYPEDEF; */
			}else if(strcmp(yytext, "if") == 0){
				/* return RW_IF; */
			}else if(strcmp(yytext, "else") == 0){
				/* return RW_ELSE; */
			}else if(strcmp(yytext, "int") == 0){
				/* return RW_INT; */
			}else if(strcmp(yytext, "float") == 0){
				/* return RW_FLOAT; */
			}else if(strcmp(yytext, "for") == 0){
				/* return RW_FOR; */
			}else if(strcmp(yytext, "void") == 0){
				/* return RW_VOID; */
			}else if(strcmp(yytext, "while") == 0){
				/* return RW_WHILE; */
			}else{
				ptr = lookup(yytext);
				if (ptr == NULL)
				     insertID(yytext);	
				else 
				     ptr->counter++;
			}
		}

{OP_assign}     /* return OP_ASSIGN; */ 
{OP_plus}       /* return OP_PLUS; */
{OP_minus}      /* return OP_MINUS; */
{OP_multi}      /* return OP_MULTI; */
{OP_divide}     /* return OP_DIVIDE; */
{OP_less}       /* return OP_LESS; */
{OP_greater}    /* return OP_GREATER; */
{OP_leq}        /* return OP_LEQ; */
{OP_geq}        /* return OP_GEQ; */
{OP_neq}        /* return OP_NEQ; */
{OP_eq}         /* return OP_EQ; */
{OP_and}        /* return OP_AND; */
{OP_or}         /* return OP_OR; */
{OP_not}        /* return OP_NOT; */
{DL_lparen}     /* return MK_LPAREN; */
{DL_rparen}     /* return MK_RPAREN; */
{DL_lbrace}     /* return MK_LBRACE; */
{DL_rbrace}     /* return MK_RBRACE; */
{DL_lbracket}   /* return MK_LBRACKET; */
{DL_rbracket}   /* return MK_RBRACKET; */
{DL_comma}      {}
{DL_semicol}    {}
{newline}       linenumber += 1;
{error}         {
				printf("ERR in line %d: %s\n", linenumber, yytext);
				return ERROR;
				}


%%

main(int argc, char **argv)
{
 argc--; ++argv;
   if (argc > 0)
      yyin = fopen(argv[0], "r");
   else
      yyin = stdin;
   yylex();
   /* printSymTab(); */
   printf("\n");
   printFreq();
}

