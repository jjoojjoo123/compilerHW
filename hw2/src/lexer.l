%option noyywrap 
%{
#include <stdio.h>
#include "header.h" 
int linenumber = 0;
symtab * lookup();
symtab * ptr;
void     insertID();
void 	 printSymTab();
/* You need to define for all tokens in C--, here are some examples */
#define COMMENT 1
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
RWInt {digit}+
RWFloat {digit}+\.|\.{digit}+|{digit}+\.{digit}+
RWString \"([^\\\"]|\\.)*\"

/* You need to define the following RE's 
Float_constant
String_constant
comment
*/

OP_assign        "="
OP_or   "||"

/* Other operators appear here */ 


newline  "\n"

DL_lparen "("
DL_rparen ")"
DL_lbrace "{"
DL_rbrace "}"
DL_comma  ","
DL_semicol ";"
DL_dot  "."

/* Other separators appear here */

error    .

%%

{WS}            {}
{RWInt}         {/* return INT; */ }
{RWFloat}       {/* return FLOAT; */}
{ID}            { 
			ptr = lookup(yytext);
			if (ptr == NULL)
			     insertID(yytext);	
			else 
			     ptr->counter++;
		}

{OP_assign}     /* return OP_ASSIGN; */ 
{OP_or}         /* return OP_OR; */
{DL_lparen}     /* return MK_LPAREN; */
{DL_rparen}     /* return MK_RPAREN; */
{DL_lbrace}     /* return MK_LBRACE; */
{DL_rbrace}     /* return MK_RBRACE; */
{DL_comma}      {}
{DL_semicol}    {}
{newline}       linenumber += 1;
{error}         printf("ERR \n");/* return ERROR; */


%%

main(int argc, char **argv)
{
 argc--; ++argv;
   if (argc > 0)
      yyin = fopen(argv[0], "r");
   else
      yyin = stdin;
   yylex();
   printSymTab();
}

