#ifndef __OFFSET_H__
#define __OFFSET_H__

#include "header.h"

void offsetAnalysis(AST_NODE* programNode);
int offsetSet(AST_NODE* node, int nowOffset);
int offsetBlock(AST_NODE* blockNode, int nowOffset);

#endif