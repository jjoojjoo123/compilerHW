#ifndef __GENCODE_H__
#define __GENCODE_H__

#include "header.h"
#include "symbolTable.h"
#include <stdio.h>

void gen_program(AST_NODE* programNode, FILE* output);
void gen_programNode(AST_NODE *programNode);
int getArraySize(TypeDescriptor* idTypeDescriptor);
void gen_globalVar(AST_NODE* varDeclListNode);
void gen_functionDecl(AST_NODE *functionDeclNode);
void gen_generalNode(AST_NODE* node);
void gen_stmtNode(AST_NODE* stmtNode);
void gen_while(AST_NODE* whileNode);
void gen_for(AST_NODE* forNode);
void gen_assign(AST_NODE* assignNode);
void gen_if(AST_NODE* ifNode);
void gen_return(AST_NODE* returnNode);
void gen_assignOrExpr(AST_NODE* assignOrExprNode);
void gen_blockNode(AST_NODE* blockNode);
void gen_exprRelatedNode(AST_NODE* exprRelatedNode);
void gen_idNodeRef(AST_NODE* idNode);
void gen_idNode(AST_NODE* idNode);
void gen_constNode(AST_NODE* constNode);
void gen_integer(AST_NODE* node, int i);
void gen_float(AST_NODE* node, float f);
void gen_stringConst(AST_NODE* node, char* sc);
void gen_exprNode(AST_NODE* exprNode);
void gen_boolExprNode(AST_NODE* boolExprNode);
void gen_boolShortCircuitNode(AST_NODE* boolExprNode);
void gen_functionCallWithoutCatchReturn(AST_NODE* functionCallNode);
void gen_functionCall(AST_NODE* functionCallNode);


#endif
