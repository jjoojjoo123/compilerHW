#ifndef __GENCODE_H__
#define __GENCODE_H__

#include "header.h"
#include <stdio.h>

void gen_program(AST_NODE* programNode, FILE* output);
void gen_programNode(AST_NODE *programNode);
void gen_globalVar(AST_NODE* varDeclListNode);
void gen_globalVar(AST_NODE* varDeclListNode);
void gen_functionDecl(AST_NODE *functionDeclNode);


#endif
