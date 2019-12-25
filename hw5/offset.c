#include "offset.h"
#include "header.h"
#include "symbolTable.h"

void offsetAnalysis(AST_NODE* programNode){
	offsetSet(programNode, 0);
}

int offsetSet(AST_NODE* node, int nowOffset){
	if(node->nodeType == DECLARATION_NODE && node->semantic_value.declSemanticValue.kind == FUNCTION_DECL){
		//nowOffset = 0;
		AST_NODE* paramListNode = node->child->rightSibling->rightSibling;
		AST_NODE* blockNode = paramListNode->rightSibling;
		//offsetParamList(paramListNode); //hw6
		nowOffset = offsetSet(blockNode, 0);
		AST_NODE *idNode = node->child->rightSibling;
        idNode->semantic_value.identifierSemanticValue.symbolTableEntry = retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName);
        idNode->semantic_value.identifierSemanticValue.symbolTableEntry->offset = nowOffset;
        return nowOffset;
	}else if(node->nodeType == BLOCK_NODE){
		nowOffset = offsetBlock(node, nowOffset);
	}
	AST_NODE* child = node->child;
	while(child){
		nowOffset = offsetSet(child, nowOffset);
		child = child->rightSibling;
	}
	return nowOffset;
}

int offsetBlock(AST_NODE* blockNode, int nowOffset){
	if(!blockNode->child || blockNode->child->nodeType == STMT_LIST_NODE){
		return nowOffset;
	}
	AST_NODE* declNode = blockNode->child->child;
	while(declNode){
		AST_NODE* idNode = declNode->child->rightSibling;
		while(idNode){
			SymbolTableEntry* entry = idNode->semantic_value.identifierSemanticValue.symbolTableEntry;
			TypeDescriptor* descriptor = entry->attribute->attr.typeDescriptor;
			if(descriptor->kind == ARRAY_TYPE_DESCRIPTOR){
				int space = 1;
				ArrayProperties properties = descriptor->properties.arrayProperties;
				for(int i = 0;i < properties.dimension;i++){
					space *= properties.sizeInEachDimension[i];
				}
				nowOffset -= 4 * space;
			}else{
				nowOffset -= 4;
			}
			entry->offset = nowOffset;
			idNode = idNode->rightSibling;
		}
		declNode = declNode->rightSibling;
	}
	return nowOffset;
}