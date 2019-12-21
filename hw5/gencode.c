#include "header.h"
#include "symboltable.h"
#include "gencode.h"
#include <stdio.h>
#include <string.h>

int label_number = 0;
FILE* outputFile = NULL;
char* g_currentFunctionName = NULL;

void gen_program(AST_NODE* programNode, FILE* output)
{
	outputFile = output;
	if(!outputFile)
	{
		printf("Cannot open file.");
		exit(EXIT_FAILURE);
	}

	gen_programNode(programNode);
}

void gen_programNode(AST_NODE *programNode)
{
	AST_NODE *declList = programNode->child;
	while(declList)
	{
		if(declList->nodeType == VARIABLE_DECL_LIST_NODE){
			fprintf(outputFile, ".data\n");
			gen_globalVar(declList);
			fprintf(outputFile, ".text\n");
		}
		else if(declList->nodeType == DECLARATION_NODE){
			gen_functionDecl(declList);
		}
		declList = declList->rightSibling;
	}
	return;
}

void gen_globalVar(AST_NODE* varDeclListNode)
{
	AST_NODE *declList = varDeclListNode->child;
	while(declList)
	{
		if(declList->semantic_value.declSemanticValue.kind == VARIABLE_DECL){
			AST_NODE *idNode = declList->child->rightSibling;
			while(idNode)
			{
				SymbolTableEntry* idSymbolTableEntry = idNode->semantic_value.identifierSemanticValue.symbolTableEntry;
				TypeDescriptor* idTypeDescriptor = idSymbolTableEntry->attribute->attr.typeDescriptor;
				
				float value = 0;
				if (idNode->semantic_value.identifierSemanticValue.kind == WITH_INIT_ID) {
					if(idNode->child->semantic_value.const1->const_type == FLOATC){
						value = idNode->child->semantic_value.const1->const_u.fval;
					}
					else{
						value = idNode->child->semantic_value.const1->const_u.intval;
					}
				}
				
				if(idTypeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR){
					if(idTypeDescriptor->properties.dataType == INT_TYPE){
						fprintf(outputFile, "_g_%s: .word %d\n", idSymbolTableEntry->name, (int)value);
					}
					else if(idTypeDescriptor->properties.dataType == FLOAT_TYPE){
						fprintf(outputFile, "_g_%s: .float %f\n", idSymbolTableEntry->name, value);
					}
				}
				else if(idTypeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR){
					//int variableSize = getVariableSize(idTypeDescriptor);
					//fprintf(outputFile, "_g_%s: .space %d\n", idSymbolTableEntry->name, variableSize);
				}
				idNode = idNode->rightSibling;
			}
		}
		declList = declList->rightSibling;
	}
	return;
}

void gen_functionDecl(AST_NODE *functionDeclNode)
{
	AST_NODE* functionIdNode = functionDeclNode->child->rightSibling;
	int i;

	g_currentFunctionName = functionIdNode->semantic_value.identifierSemanticValue.identifierName;

	fprintf(outputFile, ".text\n");
	if (strcmp(functionIdNode->semantic_value.identifierSemanticValue.identifierName, "MAIN") != 0) {
		fprintf(outputFile, "_start_%s:\n", functionIdNode->semantic_value.identifierSemanticValue.identifierName);
	} else {
		fprintf(outputFile, "%s:\n", functionIdNode->semantic_value.identifierSemanticValue.identifierName);
	}

	//prologue
	fprintf(outputFile, "sd ra, 0(sp)\n");
	fprintf(outputFile, "sd fp, -8(sp)\n");
	fprintf(outputFile, "addi fp, sp, -8\n");
	fprintf(outputFile, "add sp, sp, -16\n");
	if (strcmp(functionIdNode->semantic_value.identifierSemanticValue.identifierName, "MAIN") != 0) {
		fprintf(outputFile, "la ra, _frameSize_main\n");
	} else {
		fprintf(outputFile, "la ra, _frameSize_%s\n", functionIdNode->semantic_value.identifierSemanticValue.identifierName);
	}
	fprintf(outputFile, "lw ra, 0(ra)\n");
	fprintf(outputFile, "sub sp, sp, ra\n");
	//printStoreRegister(outputFile);

	//resetRegisterTable(functionIdNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->offsetInAR);

	AST_NODE* blockNode = functionIdNode->rightSibling->rightSibling;
	AST_NODE *listNode = blockNode->child;
	while(listNode)
	{
		gen_generalNode(listNode);
		listNode = listNode->rightSibling;
	}

	//epilogue
	fprintf(outputFile, "_end_%s:\n", g_currentFunctionName);
	//printRestoreRegister(outputFile);
	fprintf(outputFile, "ld ra, 8(fp)\n");
	fprintf(outputFile, "mov sp, fp\n");
	fprintf(outputFile, "add sp, sp, 8\n");
	fprintf(outputFile, "ld fp, 0(fp)\n");
	if (strcmp(functionIdNode->semantic_value.identifierSemanticValue.identifierName, "MAIN") != 0)
	{
		fprintf(outputFile, "jr ra\n");
	}
	fprintf(outputFile, ".data\n");
	/*int frameSize = abs(functionIdNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->offsetInAR) + 
		(INT_REGISTER_COUNT + INT_WORK_REGISTER_COUNT + INT_OTHER_REGISTER_COUNT + FLOAT_REGISTER_COUNT + FLOAT_WORK_REGISTER_COUNT) * 4 +
		g_pseudoRegisterTable.isAllocatedVector->size * 4;

	while(frameSize%8 != 0){
		frameSize=frameSize+4;	
	}
	frameSize = frameSize +(INT_REGISTER_COUNT+INT_WORK_REGISTER_COUNT+INT_OTHER_REGISTER_COUNT)*4;
	while(frameSize%8 != 0){
		frameSize=frameSize+4;	
	}
	if (strcmp(functionIdNode->semantic_value.identifierSemanticValue.identifierName, "MAIN") != 0){
		fprintf(outputFile, "_frameSize_main: .word %d\n", frameSize+8);
	}else{
		fprintf(outputFile, "_frameSize_%s: .word %d\n", functionIdNode->semantic_value.identifierSemanticValue.identifierName, 
								frameSize+8);
	}
	*/
	return;
}

void gen_generalNode(AST_NODE* node)
{
	AST_NODE *listNode = node->child;
	switch(node->nodeType)
	{
		case VARIABLE_DECL_LIST_NODE:
			while(listNode)
			{
				gen_generalNode(listNode);
				listNode = listNode->rightSibling;
			}
			break;
		case STMT_LIST_NODE:
			while(listNode)
			{
				gen_stmtNode(listNode);
				listNode = listNode->rightSibling;
			}
			break;
		case NONEMPTY_ASSIGN_EXPR_LIST_NODE:
			while(listNode)
			{
				gen_assignOrExpr(listNode);
				if(listNode->dataType == INT_TYPE)
				{
					//freeRegister(INT_REG, listNode->registerIndex);
				}
				else if(listNode->dataType == FLOAT_TYPE)
				{
					//freeRegister(FLOAT_REG, listNode->registerIndex);
				}
				node->registerIndex = listNode->registerIndex;
				listNode = listNode->rightSibling;
			}
			break;
		case NONEMPTY_RELOP_EXPR_LIST_NODE:
			while(listNode)
			{
				gen_exprRelatedNode(listNode);
				if(listNode->dataType == INT_TYPE)
				{
					//freeRegister(INT_REG, listNode->registerIndex);
				}
				else if(traverseChildren->dataType == FLOAT_TYPE)
				{
					//freeRegister(FLOAT_REG, listNode->registerIndex);
				}
				node->registerIndex = listNode->registerIndex;
				listNode = listNode->rightSibling;
			}
			break;
		case NUL_NODE:
			break;
		default:
			printf("Unhandle case in void processGeneralNode(AST_NODE *node)\n");
			node->dataType = ERROR_TYPE;
			break;
	}
}

void gen_blockNode(AST_NODE* blockNode)
{
	AST_NODE *listNode = blockNode->child;
	while(listNode)
	{
		gen_generalNode(listNode);
		listNode = listNode->rightSibling;
	}
}

void gen_exprNode(AST_NODE* exprNode)
{
	if(exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION)
	{
		AST_NODE* leftOp = exprNode->child;
		AST_NODE* rightOp = leftOp->rightSibling;
		if (exprNode->semantic_value.exprSemanticValue.op.binaryOp == BINARY_OP_OR || exprNode->semantic_value.exprSemanticValue.op.binaryOp == BINARY_OP_AND) {
			//hw6
			/*int labelNumber = getLabelNumber();
			char *leftOpRegName, *rightOpRegName;

			if(leftOp->dataType == FLOAT_TYPE || rightOp->dataType == FLOAT_TYPE) {
				char *exprRegName;
				exprNode->registerIndex = getRegister(INT_REG);
				codeGenPrepareRegister(INT_REG, exprNode->registerIndex, 0, 0, &exprRegName);
				
				if (expr_bin_op(exprNode) == BINARY_OP_AND) {
					gen_exprRelatedNode(leftOp);
					if(leftOp->dataType == INT_TYPE)
						leftOp->registerIndex = codeGenConvertFromIntToFloat(leftOp->registerIndex);
					codeGenPrepareRegister(FLOAT_REG, leftOp->registerIndex, 1, 1, &leftOpRegName);
					fprintf(outputFile, "fcmp %s, #0.0\n", leftOpRegName);
					fprintf(outputFile, "beq _booleanFalse%d\n", labelNumber);
					codeGenExprRelatedNode(rightOp);
					if(rightOp->dataType == INT_TYPE)
						rightOp->registerIndex = codeGenConvertFromIntToFloat(rightOp->registerIndex);
					codeGenPrepareRegister(FLOAT_REG, rightOp->registerIndex, 1, 1, &rightOpRegName);
					fprintf(outputFile, "fcmp %s, #0.0\n", rightOpRegName);
					fprintf(outputFile, "beq _booleanFalse%d\n", labelNumber);
					fprintf(outputFile, "_booleanTrue%d:\n", labelNumber);
					fprintf(outputFile, "mov %s, #%d\n", exprRegName, 1);
					fprintf(outputFile, "b _booleanExit%d\n", labelNumber);
					fprintf(outputFile, "_booleanFalse%d:\n", labelNumber);
					fprintf(outputFile, "mov %s, #%d\n", exprRegName, 0);
					fprintf(outputFile, "_booleanExit%d:\n", labelNumber);
				} else {
					codeGenExprRelatedNode(leftOp);
					if(leftOp->dataType == INT_TYPE)
						leftOp->registerIndex = codeGenConvertFromIntToFloat(leftOp->registerIndex);
					codeGenPrepareRegister(FLOAT_REG, leftOp->registerIndex, 1, 1, &leftOpRegName);// need think for isAddr
					fprintf(outputFile, "fcmp %s, #0.0\n", leftOpRegName);
					fprintf(outputFile, "bne _booleanTrue%d\n", labelNumber);
					codeGenExprRelatedNode(rightOp);
					if(rightOp->dataType == INT_TYPE)
						rightOp->registerIndex = codeGenConvertFromIntToFloat(rightOp->registerIndex);
					codeGenPrepareRegister(FLOAT_REG, rightOp->registerIndex, 1, 1, &rightOpRegName);// need think for isAddr
					fprintf(outputFile, "fcmp %s, #0.0\n", rightOpRegName);
					fprintf(outputFile, "bne _booleanTrue%d\n", labelNumber);
					fprintf(outputFile, "_booleanFalse%d:\n", labelNumber);
					fprintf(outputFile, "mov %s, #%d\n", exprRegName, 0);
					fprintf(outputFile, "b _booleanExit%d\n", labelNumber);
					fprintf(outputFile, "_booleanTrue%d:\n", labelNumber);
					fprintf(outputFile, "mov %s, #%d\n", exprRegName, 1);
					fprintf(outputFile, "_booleanExit%d:\n", labelNumber);
				}
				//freeRegister(FLOAT_REG, leftOp->registerIndex);
				//freeRegister(FLOAT_REG, rightOp->registerIndex);
			} else if (exprNode->dataType == INT_TYPE) {
				if (expr_bin_op(exprNode) == BINARY_OP_AND) {
					codeGenExprRelatedNode(leftOp);
					codeGenPrepareRegister(INT_REG, leftOp->registerIndex, 1, 0, &leftOpRegName);
					fprintf(outputFile, "cmp %s, #0\n", leftOpRegName);
					fprintf(outputFile, "beq _booleanFalse%d\n", labelNumber);
					codeGenExprRelatedNode(rightOp);
					codeGenPrepareRegister(INT_REG, rightOp->registerIndex, 1, 0, &rightOpRegName);
					fprintf(outputFile, "cmp %s, #0\n", rightOpRegName);
					fprintf(outputFile, "beq _booleanFalse%d\n", labelNumber);
					fprintf(outputFile, "_booleanTrue%d:\n", labelNumber);
					fprintf(outputFile, "mov %s, #%d\n", leftOpRegName, 1);
					fprintf(outputFile, "b _booleanExit%d\n", labelNumber);
					fprintf(outputFile, "_booleanFalse%d:\n", labelNumber);
					fprintf(outputFile, "mov %s, #%d\n", leftOpRegName, 0);
					fprintf(outputFile, "_booleanExit%d:\n", labelNumber);
				} else {
					codeGenExprRelatedNode(leftOp);
					codeGenPrepareRegister(INT_REG, leftOp->registerIndex, 1, 0, &leftOpRegName);
					fprintf(outputFile, "cmp %s, #0\n", leftOpRegName);
					fprintf(outputFile, "bne _booleanTrue%d\n", labelNumber);
					codeGenExprRelatedNode(rightOp);
					codeGenPrepareRegister(INT_REG, rightOp->registerIndex, 1, 0, &rightOpRegName);
					fprintf(outputFile, "cmp %s, #0\n", rightOpRegName);
					fprintf(outputFile, "bne _booleanTrue%d\n", labelNumber);
					fprintf(outputFile, "_booleanFalse%d:\n", labelNumber);
					fprintf(outputFile, "mov %s, #%d\n", leftOpRegName, 0);
					fprintf(outputFile, "b _booleanExit%d\n", labelNumber);
					fprintf(outputFile, "_booleanTrue%d:\n", labelNumber);
					fprintf(outputFile, "mov %s, #%d\n", leftOpRegName, 1);
					fprintf(outputFile, "_booleanExit%d:\n", labelNumber);
				}
				exprNode->registerIndex = leftOp->registerIndex;
				freeRegister(INT_REG, rightOp->registerIndex);
			}*/
		} else {
			gen_exprRelatedNode(leftOp);
			gen_exprRelatedNode(rightOp);
			if(leftOp->dataType == FLOAT_TYPE || rightOp->dataType == FLOAT_TYPE)
			{
				//hw6
				/*if(leftOp->dataType == INT_TYPE)
				{
					leftOp->registerIndex = codeGenConvertFromIntToFloat(leftOp->registerIndex);
				}
				if(rightOp->dataType == INT_TYPE)
				{
					rightOp->registerIndex = codeGenConvertFromIntToFloat(rightOp->registerIndex);
				}*/

				switch(exprNode->semantic_value.exprSemanticValue.op.binaryOp)
				{
					case BINARY_OP_ADD:
						exprNode->registerIndex = leftOp->registerIndex;
						codeGen3RegInstruction(FLOAT_REG, "fadd.s", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						break;
					case BINARY_OP_SUB:
						exprNode->registerIndex = leftOp->registerIndex;
						codeGen3RegInstruction(FLOAT_REG, "fsub.s", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						break;
					case BINARY_OP_MUL:
						exprNode->registerIndex = leftOp->registerIndex;
						codeGen3RegInstruction(FLOAT_REG, "fmul.s", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						break;
					case BINARY_OP_DIV:
						exprNode->registerIndex = leftOp->registerIndex;
						codeGen3RegInstruction(FLOAT_REG, "fdiv.s", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						break;
					case BINARY_OP_EQ:
						exprNode->registerIndex = getRegister(INT_REG);
						codeGen3RegInstruction(FLOAT_REG, "feq.s", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						freeRegister(FLOAT_REG, leftOp->registerIndex);
						break;
					case BINARY_OP_GE:
						exprNode->registerIndex = getRegister(INT_REG);
						codeGen3RegInstruction(FLOAT_REG, "fle.s", exprNode->registerIndex, rightOp->registerIndex, leftOp->registerIndex);
						freeRegister(FLOAT_REG, leftOp->registerIndex);
						break;
					case BINARY_OP_LE:
						exprNode->registerIndex = getRegister(INT_REG);
						codeGen3RegInstruction(FLOAT_REG, "fle.s", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						freeRegister(FLOAT_REG, leftOp->registerIndex);
						break;
					case BINARY_OP_NE:
						exprNode->registerIndex = getRegister(INT_REG);
						codeGen3RegInstruction(FLOAT_REG, "flt.s", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						codeGen3RegInstruction(FLOAT_REG, "flt.s", leftOp->registerIndex, rightOp->registerIndex, leftOp->registerIndex);
						codeGenLogicalInstruction(FLOAT_REG, "or", exprNode->registerIndex, exprNode->registerIndex, leftOp->registerIndex);
						freeRegister(FLOAT_REG, leftOp->registerIndex);
						break;
					case BINARY_OP_GT:
						exprNode->registerIndex = getRegister(INT_REG);
						codeGen3RegInstruction(FLOAT_REG, "flt.s", exprNode->registerIndex, rightOp->registerIndex, leftOp->registerIndex);
						freeRegister(FLOAT_REG, leftOp->registerIndex);
						break;
					case BINARY_OP_LT:
						exprNode->registerIndex = getRegister(INT_REG);
						codeGen3RegInstruction(FLOAT_REG, "flt.s", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						freeRegister(FLOAT_REG, leftOp->registerIndex);
						break;
					case BINARY_OP_AND:
						exprNode->registerIndex = getRegister(INT_REG);
						codeGenLogicalInstruction(FLOAT_REG, "and", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						freeRegister(FLOAT_REG, leftOp->registerIndex);
						break;
					case BINARY_OP_OR:
						exprNode->registerIndex = getRegister(INT_REG);
						codeGenLogicalInstruction(FLOAT_REG, "or", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						freeRegister(FLOAT_REG, leftOp->registerIndex);
						break;
					default:
						printf("Unhandled case in void evaluateExprValue(AST_NODE* exprNode)\n");
						break;
				}

				freeRegister(FLOAT_REG, rightOp->registerIndex);
			}
			else if(exprNode->dataType == INT_TYPE)
			{
				exprNode->registerIndex = leftOp->registerIndex;
				switch(exprNode->semantic_value.exprSemanticValue.op.binaryOp)
				{
					case BINARY_OP_ADD:
						if(rightOp->semantic_value.EXPRSemanticValue.isConstEval){
							//code gen addi...
						}
						else{
							codeGen3RegInstruction(FLOAT_REG, "add", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						}
						break;
					case BINARY_OP_SUB:
						codeGen3RegInstruction(INT_REG, "sub", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						break;
					case BINARY_OP_MUL:
						codeGen3RegInstruction(INT_REG, "mul", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						break;
					case BINARY_OP_DIV:
						codeGen3RegInstruction(INT_REG, "div", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						break;
					case BINARY_OP_EQ:
						codeGen3RegInstruction(INT_REG, "slt", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						codeGen2RegInstruction(INT_REG, "neg", exprNode->registerIndex, exprNode->registerIndex);
						codeGen3RegInstruction(INT_REG, "slt", leftOp->registerIndex, rightOp->registerIndex, leftOp->registerIndex);
						codeGen2RegInstruction(INT_REG, "neg", leftOp->registerIndex, leftOp->registerIndex);
						codeGenLogicalInstruction(INT_REG, "and", exprNode->registerIndex, exprNode->registerIndex, leftOp->registerIndex);
						break;
					case BINARY_OP_GE:
						codeGen3RegInstruction(INT_REG, "slt", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						codeGen2RegInstruction(INT_REG, "neg", exprNode->registerIndex, exprNode->registerIndex);
						break;
					/*不會這個 
					case BINARY_OP_LE:
						codeGen2RegInstruction(INT_REG, "cmp", leftOp->registerIndex, rightOp->registerIndex);
						codeGenSetReg_cond(INT_REG, "cset",exprNode->registerIndex, "le");
						break;*/
					case BINARY_OP_NE:
						codeGen3RegInstruction(INT_REG, "slt", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						codeGen2RegInstruction(INT_REG, "neg", exprNode->registerIndex, exprNode->registerIndex);
						codeGen3RegInstruction(INT_REG, "slt", leftOp->registerIndex, rightOp->registerIndex, leftOp->registerIndex);
						codeGen2RegInstruction(INT_REG, "neg", leftOp->registerIndex, leftOp->registerIndex);
						codeGenLogicalInstruction(INT_REG, "and", exprNode->registerIndex, exprNode->registerIndex, leftOp->registerIndex);
						codeGen2RegInstruction(INT_REG, "neg", exprNode->registerIndex, exprNode->registerIndex);
						break;
					/*跟這個 
					case BINARY_OP_GT:
						codeGen2RegInstruction(INT_REG, "cmp", leftOp->registerIndex, rightOp->registerIndex);
						codeGenSetReg_cond(INT_REG, "cset",exprNode->registerIndex, "gt");
						break;*/ 
					case BINARY_OP_LT:
						codeGen2RegInstruction(INT_REG, "lt", leftOp->registerIndex, rightOp->registerIndex);
						break;
					case BINARY_OP_AND:
						codeGenLogicalInstruction(INT_REG, "and", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						break;
					case BINARY_OP_OR:
						codeGenLogicalInstruction(INT_REG, "or", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
						break;
					default:
						printf("Unhandled case in void evaluateExprValue(AST_NODE* exprNode)\n");
						break;
				}

				freeRegister(INT_REG, rightOp->registerIndex);
			}
		}
	}//endif BINARY_OPERATION
	else if(exprNode->semantic_value.exprSemanticValue.kind == UNARY_OPERATION)
	{
		int tmpZero = 0;
		AST_NODE* operand = exprNode->child;
		gen_exprRelatedNode(operand);
		if(operand->dataType == FLOAT_TYPE)
		{
			switch(exprNode->semantic_value.exprSemanticValue.op.unaryOp)
			{
				case UNARY_OP_POSITIVE:
					exprNode->registerIndex = operand->registerIndex;
					break;
				case UNARY_OP_NEGATIVE:
					exprNode->registerIndex = operand->registerIndex;
					codeGen2RegInstruction(FLOAT_REG, "fneg.s", exprNode->registerIndex, exprNode->registerIndex);
					break;
				case UNARY_OP_LOGICAL_NEGATION:
					exprNode->registerIndex = getRegister(INT_REG);
					//codeGenGetBoolOfFloat(exprNode->registerIndex, operand->registerIndex);
					freeRegister(FLOAT_REG, operand->registerIndex);
					break;
				default:
					printf("Unhandled case in void evaluateExprValue(AST_NODE* exprNode)\n");
					break;
			}
		}
		else if(operand->dataType == INT_TYPE)
		{
			switch(exprNode->semantic_value.exprSemanticValue.op.unaryOp)
			{
				case UNARY_OP_POSITIVE:
					exprNode->registerIndex = operand->registerIndex;
					break;
				case UNARY_OP_NEGATIVE:
					exprNode->registerIndex = operand->registerIndex;
					codeGen2RegInstruction(INT_REG, "neg", exprNode->registerIndex, exprNode->registerIndex);
					break;
					/*hw6
				case UNARY_OP_LOGICAL_NEGATION:
					exprNode->registerIndex = operand->registerIndex;
					codeGenCmp0Instruction(INT_REG,"cmp",exprNode->registerIndex,0);
					codeGenSetReg(INT_REG, "mov",exprNode->registerIndex, 0);
					codeGenSetReg(INT_REG, "moveq",exprNode->registerIndex, 1);*/
					break;
				default:
					printf("Unhandled case in void evaluateExprValue(AST_NODE* exprNode)\n");
					break;
			}
		}
	}
}

void gen_functionCall(AST_NODE* functionCallNode)
{
	AST_NODE* functionIdNode = functionCallNode->child;
	AST_NODE* parameterList = functionIdNode->rightSibling;
	if(strcmp(functionIdNode->semantic_value.identifierSemanticValue.identifierName, "write") == 0)
	{
		AST_NODE* firstParameter = parameterList->child;
		codeGenExprRelatedNode(firstParameter);
		char* parameterRegName = NULL;
		switch(firstParameter->dataType)
		{
			case INT_TYPE:
				codeGenPrepareRegister(INT_REG, firstParameter->registerIndex, 1, 0, &parameterRegName);
				fprintf(outputfile, "lw %s, -4(fp)", parameterRegName);
				fprintf(outputFile, "mv a0, %s\n", parameterRegName);
				fprintf(outputFile, "jal _write_int\n");
				freeRegister(INT_REG, firstParameter->registerIndex);
				break;
			case FLOAT_TYPE:
				codeGenPrepareRegister(FLOAT_REG, firstParameter->registerIndex, 1, 0, &parameterRegName);
				fprintf(outputfile, "lw %s, -8(fp)", parameterRegName);
				fprintf(outputFile, "fmv.s fa0, %s\n", parameterRegName);
				fprintf(outputFile, "jal _write_float\n");
				break;
			case CONST_STRING_TYPE:
				int constantLabelNumber = codeGenConstantLabel(STRINGC, &firstParameter->semantic_value.const1->const_u.sc);
				codeGenPrepareRegister(INT_REG, firstParameter->registerIndex, 1, 0, &parameterRegName);
				fprintf(outputfile, "lui %s, %%hi(_CONSTANT_%d)\n", parameterRegName, constantLabelNumber);
				fprintf(outputfile, "addi a0, %s, %%lo(_CONSTANT_%d)\n", parameterRegName, constantLabelNumber);
				fprintf(outputfile, "jal _write_str\n");
				freeRegister(INT_REG, firstParameter->registerIndex);
				break;
			default:
				printf("Unhandled case in void codeGenFunctionCall(AST_NODE* functionCallNode)\n");
				printf("firstParameter->registerIndex was not free\n");
				break;
		}
		return;
	}


	if(strcmp(functionIdNode->semantic_value.identifierSemanticValue.identifierName, "read") == 0)
	{
		fprintf(outputfile, "jal _read_int\n");
	}
	else if(strcmp(functionIdNode->semantic_value.identifierSemanticValue.identifierName, "fread") == 0)
	{
		fprintf(outputfile, "jal _read_float\n");
	}
	else
	{
		if (strcmp(functionIdNode->semantic_value.identifierSemanticValue.identifierName, "MAIN") != 0) {
			AST_NODE* traverseParameter = parameterList->child;
			//hw6
			codeGenStoreParam(traverseParameter, id_sym(functionIdNode)->attribute->attr.functionSignature->parameterList);
			fprintf(g_codeGenOutputFp, "jal _start_%s:\n", functionIdNode->semantic_value.identifierSemanticValue.identifierName);
			int paramOffset = 0;
			while(traverseParameter) {
				paramOffset += 8;
				traverseParameter = traverseParameter->rightSibling;
			}
			if (paramOffset > 0) {
				fprintf(g_codeGenOutputFp, "add sp, sp, %d\n", paramOffset);
			}
		} else {
			fprintf(g_codeGenOutputFp, "jal %s\n", functionIdNode->semantic_value.identifierSemanticValue.identifierName);
		}
	}




	if (functionIdNode->semantic_value.identifierSemanticValue.symbolTableEntry) {
		if(functionIdNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.functionSignature->returnType == INT_TYPE)
		{
			functionCallNode->registerIndex = getRegister(INT_REG);
			char* returnIntRegName = NULL;
			codeGenPrepareRegister(INT_REG, functionCallNode->registerIndex, 0, 0, &returnIntRegName);

			fprintf(g_codeGenOutputFp, "mv %s, w0\n", returnIntRegName);

			codeGenSaveToMemoryIfPsuedoRegister(INT_REG, functionCallNode->registerIndex, returnIntRegName);
		}
		else if(functionIdNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.functionSignature->returnType == FLOAT_TYPE)
		{
			functionCallNode->registerIndex = getRegister(FLOAT_REG);
			char* returnfloatRegName = NULL;
			codeGenPrepareRegister(FLOAT_REG, functionCallNode->registerIndex, 0, 0, &returnfloatRegName);

			fprintf(g_codeGenOutputFp, "fmv.s %s, s0\n", returnfloatRegName);

			codeGenSaveToMemoryIfPsuedoRegister(INT_REG, functionCallNode->registerIndex, returnfloatRegName);
		}
	}
}

