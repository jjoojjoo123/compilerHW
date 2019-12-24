#include "header.h"
#include "symboltable.h"
#include "gencode.h"
#include <stdio.h>
#include <string.h>

int label_number = 0;
FILE* outputFile = NULL;
char* g_currentFunctionName = NULL;

#define write0(...) fprintf(outputfile, __VA_ARGS__)
#define write1(...) fprintf(outputfile, "\t" __VA_ARGS__)

char* a0 = "a0";
char* fa0 = "fa0";

char* int_reg[] = {"s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11"};
char* float_reg[] = {"fs0", "fs1", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7", "fs8", "fs9", "fs10", "fs11"};

#define N_INTREG (sizeof(int_reg) / sizeof(char*))
#define N_FLOATREG (sizeof(float_reg) / sizeof(char*))

int* int_reg_counter = NULL;
int* float_reg_counter = NULL;

int used_int_reg = 0;
int used_float_reg = 0;

int get_int_reg(){
	int index = (used_int_reg++);
	if(int_reg_counter[index]){
		write1("addi sp, sp, -4\n");
		write1("sw %s, 8(sp)\n", int_reg[index]);
	}
	int_reg_counter[index]++;
	return index;
}
void free_int_reg(int index){
	int_reg_counter[index]--;
	if(int_reg_counter[index]){
		write1("lw %s, 8(sp)\n", int_reg[index]);
		write1("addi sp, sp, 4\n");
	}
}
int get_float_reg(){
	int index = (used_float_reg++);
	if(float_reg_counter[index]){
		write1("addi sp, sp, -4\n");
		write1("sw %s, 8(sp)\n", float_reg[index]);
	}
	float_reg_counter[index]++;
	return index;
}
void free_float_reg(int index){
	float_reg_counter[index]--;
	if(float_reg_counter[index]){
		write1("lw %s, 8(sp)\n", float_reg[index]);
		write1("addi sp, sp, 4\n");
	}
}

#define get_int_reg() int_reg[(used_int_reg++) / N_INTREG]
#define get_float_reg() float_reg[(used_float_reg++) / N_FLOATREG]

#define BASE_FRAMESIZE (N_INTREG * 4 + N_FLOATREG * 4)

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
			write1(".data\n");
			gen_globalVar(declList);
			write1(".text\n");
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
						write1("_g_%s: .word %d\n", idSymbolTableEntry->name, (int)value);
					}
					else if(idTypeDescriptor->properties.dataType == FLOAT_TYPE){
						write1("_g_%s: .float %f\n", idSymbolTableEntry->name, value);
					}
				}
				else if(idTypeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR){
					//int variableSize = getVariableSize(idTypeDescriptor);
					//write1("_g_%s: .space %d\n", idSymbolTableEntry->name, variableSize);
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

	int* int_reg_counter_old = int_reg_counter;
	int* float_reg_counter_old = float_reg_counter;
	int_reg_counter = (int*)malloc(sizeof(int) * N_INTREG);
	float_reg_counter = (int*)malloc(sizeof(int) * N_FLOATREG);
	memset(int_reg_counter, 0, sizeof(int) * N_INTREG);
	memset(float_reg_counter, 0, sizeof(int) * N_FLOATREG);

	g_currentFunctionName = functionIdNode->semantic_value.identifierSemanticValue.identifierName;

	write0(".text\n");
	write0("_start_%s:\n", g_currentFunctionName);

	//prologue
	write1("sd ra, 0(sp)\n");
	write1("sd fp, -8(sp)\n");
	write1("addi fp, sp, -8\n");
	write1("add sp, sp, -16\n");
	write1("la ra, _frameSize_%s\n", g_currentFunctionName);
	write1("lw ra, 0(ra)\n");
	write1("sub sp, sp, ra\n");
	//printStoreRegister(outputFile);

	//resetRegisterTable(functionIdNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->offsetInAR);

	int reg_count = 0;
	for(int i = 0;i < N_INTREG;i++){
		write1("sw %s %d(sp)", int_reg[i], (reg_count++) * 4 + 8)
	}
	for(int i = 0;i < N_FLOATREG;i++){
		write1("sw %s %d(sp)", float_reg[i], (reg_count++) * 4 + 8)
	}

	AST_NODE* blockNode = functionIdNode->rightSibling->rightSibling;
	AST_NODE *listNode = blockNode->child;
	while(listNode)
	{
		gen_generalNode(listNode);
		listNode = listNode->rightSibling;
	}

	//epilogue
	write0("_end_%s:\n", g_currentFunctionName);

	reg_count = 0;
	for(int i = 0;i < N_INTREG;i++){
		write1("lw %s %d(sp)", int_reg[i], (reg_count++) * 4 + 8)
	}
	for(int i = 0;i < N_FLOATREG;i++){
		write1("lw %s %d(sp)", float_reg[i], (reg_count++) * 4 + 8)
	}

	//printRestoreRegister(outputFile);
	write1("ld ra, 8(fp)\n");
	write1("mov sp, fp\n");
	write1("add sp, sp, 8\n");
	write1("ld fp, 0(fp)\n");
	write1("jr ra\n");
	write1(".data\n");
	//offset is a minus number
	write1("_frameSize_%s: .word %d\n", g_currentFunctionName, (BASE_FRAMESIZE - functionIdNode->semantic_value.identifierSemanticValue.symbolTableEntry->offset) / 4);
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
		write1("_frameSize_main: .word %d\n", frameSize+8);
	}else{
		write1("_frameSize_%s: .word %d\n", functionIdNode->semantic_value.identifierSemanticValue.identifierName, 
								frameSize+8);
	}
	*/
	free(int_reg_counter);
	free(float_reg_counter);
	int_reg_counter = int_reg_counter_old;
	float_reg_counter = float_reg_counter_old;
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

void gen_exprRelatedNode(AST_NODE* exprRelatedNode){
	switch(exprRelatedNode->nodeType){
		case EXPR_NODE:
			gen_exprNode(exprRelatedNode);
			break;
		case STMT_NODE:
	        //function call
	        gen_functionCall(exprRelatedNode);
	        break;
	    case IDENTIFIER_NODE:
	        gen_idNode(exprRelatedNode);
	        break;
	    case CONST_VALUE_NODE:
	        gen_constNode(exprRelatedNode);
	        break;
	}
}

void gen_idNode(AST_NODE* idNode){
	//
}

void gen_constNode(AST_NODE* constNode){
	if(exprNode->dataType == INT_TYPE){
		gen_integer(constNode, constNode->semantic_value.const1->const_u.intval);
	}else{
		gen_float(constNode, constNode->semantic_value.const1->const_u.fval);
	}
}

void gen_integer(AST_NODE* node, int i){
	node->regType = INT_REG;
	node->registerIndex = get_int_reg();
	write1("li %s, %d\n", int_reg[node->registerIndex], i);
}

void gen_float(AST_NODE* node, float f){
	node->regType = FLOAT_REG;
	node->registerIndex = get_float_reg();
	int castIindex = get_int_reg();
	write1("li %s, %d\n", int_reg[castIindex], *(int*)(&f));
	write1("fmv.w.x %s, %s\n", float_reg[node->registerIndex], int_reg[castIindex]);
	free_int_reg(castIindex);
}

void gen_exprNode(AST_NODE* exprNode)
{
	if(exprNode->semantic_value.exprSemanticValue.isConstEval){
		if(exprNode->dataType == INT_TYPE){
			gen_integer(exprNode, exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue);
		}else{
			gen_integer(exprNode, exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue);
		}
		return;
	}
	if(exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION)
	{
		switch(exprNode->semantic_value.exprSemanticValue.op.binaryOp){
			case BINARY_OP_EQ:
			case BINARY_OP_GE:
			case BINARY_OP_LE:
			case BINARY_OP_NE:
			case BINARY_OP_GT:
			case BINARY_OP_LT:
			case BINARY_OP_AND:
			case BINARY_OP_OR:
				gen_boolExprNode(exprNode);
				return;
			default:
		}
		AST_NODE* leftOp = exprNode->child;
		AST_NODE* rightOp = leftOp->rightSibling;
		gen_exprRelatedNode(leftOp);
		gen_exprRelatedNode(rightOp);
		int lindex = leftOp->registerIndex;
		int rindex = rightOp->registerIndex;
		int castFindex = -1;
		if(leftOp->regType == FLOAT_REG || rightOp->regType == FLOAT_REG)
		{
			char *rs1, *rs2, *rd;
			exprNode->regType = FLOAT_REG;
			if(leftOp->regType == INT_REG){
				castFindex = get_float_reg();
				rs1 = float_reg[castFindex];
				write1("fcvt.s.w %s, %s\n", rs1, int_reg[lindex]);
				rs2 = float_reg[rindex];
				exprNode->registerIndex = rindex;
				rd = rs2;
			}else if(rightOp->regType == INT_REG){
				rs1 = float_reg[lindex];
				castFindex = get_float_reg();
				rs2 = float_reg[castFindex];
				write1("fcvt.s.w %s, %s\n", rs2, int_reg[rindex]);
				exprNode->registerIndex = lindex;
				rd = rs1;
			}else{
				rs1 = float_reg[lindex];
				rs2 = float_reg[rindex];
				exprNode->registerIndex = lindex;
				rd = rs1;
			}

			switch(exprNode->semantic_value.exprSemanticValue.op.binaryOp)
			{
				case BINARY_OP_ADD:
					write1("fadd.s %s, %s, %s\n", rd, rs1, rs2);
					break;
				case BINARY_OP_SUB:
					write1("fsub.s %s, %s, %s\n", rd, rs1, rs2);
					break;
				case BINARY_OP_MUL:
					write1("fmul.s %s, %s, %s\n", rd, rs1, rs2);
					break;
				case BINARY_OP_DIV:
					write1("fdiv.s %s, %s, %s\n", rd, rs1, rs2);
					break;
				default:
					printf("Unhandled case in void evaluateExprValue(AST_NODE* exprNode)\n");
					break;
			}
			if(castFindex){
				free_float_reg(castFindex);
			}
			if(leftOp->regType == INT_REG){
				free_int_reg(lindex);
			}else if(rightOp->regType == INT_REG){
				free_int_reg(rindex);
			}else{
				free_float_reg(rindex);
			}
		}else{
			char *rs1 = int_reg[lindex], *rs2 = int_reg[rindex], *rd = int_reg[lindex];
			exprNode->regType = INT_REG;
			exprNode->registerIndex = lindex;
			switch(exprNode->semantic_value.exprSemanticValue.op.binaryOp)
			{
				case BINARY_OP_ADD:
					write1("add %s, %s, %s\n", rd, rs1, rs2);
					break;
				case BINARY_OP_SUB:
					write1("sub %s, %s, %s\n", rd, rs1, rs2);
					break;
				case BINARY_OP_MUL:
					write1("mul %s, %s, %s\n", rd, rs1, rs2);
					break;
				case BINARY_OP_DIV:
					write1("div %s, %s, %s\n", rd, rs1, rs2);
					break;
				default:
					printf("Unhandled case in void evaluateExprValue(AST_NODE* exprNode)\n");
					break;
			}
			free_int_reg(rindex);
		}
	}//endif BINARY_OPERATION
	else if(exprNode->semantic_value.exprSemanticValue.kind == UNARY_OPERATION)
	{
		switch(exprNode->semantic_value.exprSemanticValue.op.unaryOp){
			case UNARY_OP_LOGICAL_NEGATION:
				gen_boolExprNode(exprNode);
				return;
			default:
		}
		AST_NODE* operand = exprNode->child;
		gen_exprRelatedNode(operand);
		char* rd;
		if(operand->regType == FLOAT_REG)
		{
			exprNode->regType = FLOAT_REG;
			rd = float_reg[operand->registerIndex];
			switch(exprNode->semantic_value.exprSemanticValue.op.unaryOp)
			{
				case UNARY_OP_POSITIVE:
					exprNode->registerIndex = operand->registerIndex;
					break;
				case UNARY_OP_NEGATIVE:
					exprNode->registerIndex = operand->registerIndex;
					write1("fneg.s %s, %s\n", rd, rd);
					break;
				default:
					printf("Unhandled case in void evaluateExprValue(AST_NODE* exprNode)\n");
					break;
			}
		}
		else if(operand->regType == INT_REG)
		{
			exprNode->regType = INT_REG;
			rd = int_reg[operand->registerIndex];
			switch(exprNode->semantic_value.exprSemanticValue.op.unaryOp)
			{
				case UNARY_OP_POSITIVE:
					exprNode->registerIndex = operand->registerIndex;
					break;
				case UNARY_OP_NEGATIVE:
					exprNode->registerIndex = operand->registerIndex;
					write1("neg %s, %s\n", rd, rd);
					break;
				default:
					printf("Unhandled case in void evaluateExprValue(AST_NODE* exprNode)\n");
					break;
			}
		}
	}
}

void gen_boolExprNode(AST_NODE* boolExprNode){
	boolExprNode->regType = INT_REG;
	char* rd, rs1, rs2;
	if(exprNode->semantic_value.exprSemanticValue.kind == UNARY_OPERATION){
		int castIindex = -1;
		AST_NODE* operand = boolExprNode->child;
		gen_exprRelatedNode(operand);
		if(operand->regType == INT_REG){
			rd = rs1 = int_reg[operand->registerIndex];
			boolExprNode->registerIndex = operand->registerIndex;
			switch(exprNode->semantic_value.exprSemanticValue.op.unaryOp){
				case UNARY_OP_LOGICAL_NEGATION:
					write1("snez %s, %s", rd, rs1);
					break;
			}
		}else{
			boolExprNode->registerIndex = get_int_reg();
			rd = int_reg[boolExprNode->registerIndex];
			rs1 = float_reg[operand->registerIndex];
			switch(exprNode->semantic_value.exprSemanticValue.op.unaryOp){
				case UNARY_OP_LOGICAL_NEGATION:
					castIindex = get_int_reg();
					rs2 = int_reg[castIindex];
					write1("fclass.s %s, %s\n", rs1, rd);
					write1("slti %s, %s, 3\n", rs2, rd);
					write1("slti %s, %s, 5\n", rd, rd);
					write1("xor %s, %s, %s\n", rd, rd, rs2);
					break;
			}
		}
		if(castIindex){
			free_int_reg(castIindex);
		}
		if(operand->regType == FLOAT_REG){
			free_float_reg(operand->registerIndex);
		}
	}else{
		if(exprNode->semantic_value.exprSemanticValue.op.binaryOp == BINARY_OP_AND || exprNode->semantic_value.exprSemanticValue.op.binaryOp == BINARY_OP_OR){
			gen_boolShortCircuitNode(exprNode);
			return;
		}
		int castFindex = -1, castIindex = -1;
		AST_NODE* leftOp = exprNode->child;
		AST_NODE* rightOp = leftOp->rightSibling;
		gen_exprRelatedNode(leftOp);
		gen_exprRelatedNode(rightOp);
		int lindex = leftOp->registerIndex;
		int rindex = rightOp->registerIndex;
		char* rd, rs1, rs2;
		int bothInt = 0;
		if(leftOp->regType == rightOp->regType){
			if(leftOp->regType == INT_REG){
				boolExprNode->registerIndex = lindex;
				rd = rs1 = int_reg[lindex];
				rs2 = int_reg[rindex];
				bothInt = 1;
			}else{
				boolExprNode->registerIndex = get_int_reg();
				rd = int_reg[boolExprNode->registerIndex];
				rs1 = float_reg[lindex];
				rs2 = float_reg[rindex];
			}
		}else{
			castFindex = get_float_reg();
			if(leftOp->regType == INT_REG){
				boolExprNode->registerIndex = lindex;
				rs1 = float_reg[castFindex];
				write1("fcvt.s.w %s, %s\n", rs1, int_reg[lindex]);
				rs2 = float_reg[rindex];
				rd = int_reg[lindex];
			}else{
				boolExprNode->registerIndex = rindex;
				rs1 = float_reg[lindex];
				rs2 = float_reg[castFindex];
				write1("fcvt.s.w %s, %s\n", rs2, int_reg[rindex]);
				rd = int_reg[rindex];
			}
		}
		switch(exprNode->semantic_value.exprSemanticValue.op.binaryOp){
			case BINARY_OP_EQ:
				if(bothInt){
					write1("xor %s, %s, %s\n", rd, rs1, rs2);
					write1("seqz %s, %s\n", rd, rd);
				}else{
					write1("feq.s %s, %s, %s\n", rd, rs1, rs2);
				}
				break;
			case BINARY_OP_GE:
				if(bothInt){
					castIindex = get_int_reg();
					write1("slt %s, %s, %s\n", int_reg[castIindex], rs2, rs1);
					write1("xor %s, %s, %s\n", rd, rs1, rs2);
					write1("seqz %s, %s\n", rd, rd);
					write1("or %s, %s, %s\n", rd, rd, int_reg[castIindex]);
				}else{
					write1("fle.s %s, %s, %s\n", rd, rs2, rs1);
				}
				break;
			case BINARY_OP_LE:
				if(bothInt){
					castIindex = get_int_reg();
					write1("slt %s, %s, %s\n", int_reg[castIindex], rs1, rs2);
					write1("xor %s, %s, %s\n", rd, rs1, rs2);
					write1("seqz %s, %s\n", rd, rd);
					write1("or %s, %s, %s\n", rd, rd, int_reg[castIindex]);
				}else{
					write1("fle.s %s, %s, %s\n", rd, rs2, rs1);
				}
				break;
			case BINARY_OP_NE:
				if(bothInt){
					write1("xor %s, %s, %s\n", rd, rs1, rs2);
					write1("snez %s, %s\n", rd, rd);
				}else{
					write1("feq.s %s, %s, %s\n", rd, rs1, rs2);
					write1("xor %s, %s, 1\n", rd, rd);
				}
				break;
			case BINARY_OP_GT:
				if(bothInt){
					write1("slt %s, %s, %s\n", rd, rs2, rs1);
				}else{
					write1("flt.s %s, %s, %s\n", rd, rs2, rs1);
				}
				break;
			case BINARY_OP_LT:
				if(bothInt){
					write1("slt %s, %s, %s\n", rd, rs1, rs2);
				}else{
					write1("flt.s %s, %s, %s\n", rd, rs1, rs2);
				}
				break;
		}
		if(castIindex){
			free_int_reg(castIindex);
		}
		if(castFindex){
			free_float_reg(castFindex);
		}
		if(leftOp->regType == rightOp->regType){
			if(leftOp->regType == INT_REG){
				free_int_reg(rindex);
			}else{
				free_float_reg(rindex);
				free_float_reg(lindex);
			}
		}else{
			if(leftOp->regType == INT_REG){
				free_float_reg(rindex);
			}else{
				free_float_reg(lindex);
			}
		}
	}
}

void gen_boolShortCircuitNode(AST_NODE* boolExprNode){
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
					write1("fcmp %s, #0.0\n", leftOpRegName);
					write1("beq _booleanFalse%d\n", labelNumber);
					codeGenExprRelatedNode(rightOp);
					if(rightOp->dataType == INT_TYPE)
						rightOp->registerIndex = codeGenConvertFromIntToFloat(rightOp->registerIndex);
					codeGenPrepareRegister(FLOAT_REG, rightOp->registerIndex, 1, 1, &rightOpRegName);
					write1("fcmp %s, #0.0\n", rightOpRegName);
					write1("beq _booleanFalse%d\n", labelNumber);
					write1("_booleanTrue%d:\n", labelNumber);
					write1("mov %s, #%d\n", exprRegName, 1);
					write1("b _booleanExit%d\n", labelNumber);
					write1("_booleanFalse%d:\n", labelNumber);
					write1("mov %s, #%d\n", exprRegName, 0);
					write1("_booleanExit%d:\n", labelNumber);
				} else {
					codeGenExprRelatedNode(leftOp);
					if(leftOp->dataType == INT_TYPE)
						leftOp->registerIndex = codeGenConvertFromIntToFloat(leftOp->registerIndex);
					codeGenPrepareRegister(FLOAT_REG, leftOp->registerIndex, 1, 1, &leftOpRegName);// need think for isAddr
					write1("fcmp %s, #0.0\n", leftOpRegName);
					write1("bne _booleanTrue%d\n", labelNumber);
					codeGenExprRelatedNode(rightOp);
					if(rightOp->dataType == INT_TYPE)
						rightOp->registerIndex = codeGenConvertFromIntToFloat(rightOp->registerIndex);
					codeGenPrepareRegister(FLOAT_REG, rightOp->registerIndex, 1, 1, &rightOpRegName);// need think for isAddr
					write1("fcmp %s, #0.0\n", rightOpRegName);
					write1("bne _booleanTrue%d\n", labelNumber);
					write1("_booleanFalse%d:\n", labelNumber);
					write1("mov %s, #%d\n", exprRegName, 0);
					write1("b _booleanExit%d\n", labelNumber);
					write1("_booleanTrue%d:\n", labelNumber);
					write1("mov %s, #%d\n", exprRegName, 1);
					write1("_booleanExit%d:\n", labelNumber);
				}
				//freeRegister(FLOAT_REG, leftOp->registerIndex);
				//freeRegister(FLOAT_REG, rightOp->registerIndex);
			} else if (exprNode->dataType == INT_TYPE) {
				if (expr_bin_op(exprNode) == BINARY_OP_AND) {
					codeGenExprRelatedNode(leftOp);
					codeGenPrepareRegister(INT_REG, leftOp->registerIndex, 1, 0, &leftOpRegName);
					write1("cmp %s, #0\n", leftOpRegName);
					write1("beq _booleanFalse%d\n", labelNumber);
					codeGenExprRelatedNode(rightOp);
					codeGenPrepareRegister(INT_REG, rightOp->registerIndex, 1, 0, &rightOpRegName);
					write1("cmp %s, #0\n", rightOpRegName);
					write1("beq _booleanFalse%d\n", labelNumber);
					write1("_booleanTrue%d:\n", labelNumber);
					write1("mov %s, #%d\n", leftOpRegName, 1);
					write1("b _booleanExit%d\n", labelNumber);
					write1("_booleanFalse%d:\n", labelNumber);
					write1("mov %s, #%d\n", leftOpRegName, 0);
					write1("_booleanExit%d:\n", labelNumber);
				} else {
					codeGenExprRelatedNode(leftOp);
					codeGenPrepareRegister(INT_REG, leftOp->registerIndex, 1, 0, &leftOpRegName);
					write1("cmp %s, #0\n", leftOpRegName);
					write1("bne _booleanTrue%d\n", labelNumber);
					codeGenExprRelatedNode(rightOp);
					codeGenPrepareRegister(INT_REG, rightOp->registerIndex, 1, 0, &rightOpRegName);
					write1("cmp %s, #0\n", rightOpRegName);
					write1("bne _booleanTrue%d\n", labelNumber);
					write1("_booleanFalse%d:\n", labelNumber);
					write1("mov %s, #%d\n", leftOpRegName, 0);
					write1("b _booleanExit%d\n", labelNumber);
					write1("_booleanTrue%d:\n", labelNumber);
					write1("mov %s, #%d\n", leftOpRegName, 1);
					write1("_booleanExit%d:\n", labelNumber);
				}
				exprNode->registerIndex = leftOp->registerIndex;
				freeRegister(INT_REG, rightOp->registerIndex);
			}*/
			/*case BINARY_OP_AND:
				shortCircuitNumber = (labelNumber++);
				if(bothInt){

				}
				exprNode->registerIndex = getRegister(INT_REG);
				codeGenLogicalInstruction(FLOAT_REG, "and", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
				freeRegister(FLOAT_REG, leftOp->registerIndex);
				break;
			case BINARY_OP_OR:
				shortCircuitNumber = (labelNumber++);
				exprNode->registerIndex = getRegister(INT_REG);
				codeGenLogicalInstruction(FLOAT_REG, "or", exprNode->registerIndex, leftOp->registerIndex, rightOp->registerIndex);
				freeRegister(FLOAT_REG, leftOp->registerIndex);
				break;*/
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
				write1("lw %s, -4(fp)", parameterRegName);
				write1("mv a0, %s\n", parameterRegName);
				write1("jal _write_int\n");
				freeRegister(INT_REG, firstParameter->registerIndex);
				break;
			case FLOAT_TYPE:
				codeGenPrepareRegister(FLOAT_REG, firstParameter->registerIndex, 1, 0, &parameterRegName);
				write1("lw %s, -8(fp)", parameterRegName);
				write1("fmv.s fa0, %s\n", parameterRegName);
				write1("jal _write_float\n");
				break;
			case CONST_STRING_TYPE:
				int constantLabelNumber = codeGenConstantLabel(STRINGC, &firstParameter->semantic_value.const1->const_u.sc);
				codeGenPrepareRegister(INT_REG, firstParameter->registerIndex, 1, 0, &parameterRegName);
				write1("lui %s, %%hi(_CONSTANT_%d)\n", parameterRegName, constantLabelNumber);
				write1("addi a0, %s, %%lo(_CONSTANT_%d)\n", parameterRegName, constantLabelNumber);
				write1("jal _write_str\n");
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
		write1("jal _read_int\n");
	}
	else if(strcmp(functionIdNode->semantic_value.identifierSemanticValue.identifierName, "fread") == 0)
	{
		write1("jal _read_float\n");
	}
	else
	{
		if (strcmp(functionIdNode->semantic_value.identifierSemanticValue.identifierName, "MAIN") != 0) {
			AST_NODE* traverseParameter = parameterList->child;
			//hw6
			int paramOffset = 0;
			while(traverseParameter) {
				paramOffset += 8;
				traverseParameter = traverseParameter->rightSibling;
			}
			if(paramOffset){
				write1("subi sp, sp, %d\n", paramOffset);
				traverseParameter = parameterList->child;
				paramOffset = 0;
				while(traverseParameter) {
					gen_exprRelatedNode(traverseParameter);
					int index = traverseParameter->registerIndex;
					switch(traverseParameter->regType){
						case INT_REG:
							write1("sw %s, (%d)sp\n", int_reg[index], 8 + paramOffset);
							free_int_reg(index);
							break;
						case FLOAT_REG:
							write1("sw %s, (%d)sp\n", float_reg[index], 8 + paramOffset);
							free_float_reg(index);
							break;
						case PTR_REG:
							write1("sd %s, (%d)sp\n", int_reg[index], 8 + paramOffset);
							free_int_reg(index);
							break;
						default:
					}
					paramOffset += 8;
					traverseParameter = traverseParameter->rightSibling;
				}
			}
			write1("jal _start_%s:\n", functionIdNode->semantic_value.identifierSemanticValue.identifierName);
			if (paramOffset) {
				write1("addi sp, sp, %d\n", paramOffset);
			}
		} else {
			write1("jal _start_MAIN\n");
		}
	}




	if (functionIdNode->semantic_value.identifierSemanticValue.symbolTableEntry) {
		if(functionIdNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.functionSignature->returnType == INT_TYPE)
		{
			functionCallNode->registerIndex = getRegister(INT_REG);
			char* returnIntRegName = NULL;
			codeGenPrepareRegister(INT_REG, functionCallNode->registerIndex, 0, 0, &returnIntRegName);

			write1("mv %s, w0\n", returnIntRegName);

			codeGenSaveToMemoryIfPsuedoRegister(INT_REG, functionCallNode->registerIndex, returnIntRegName);
		}
		else if(functionIdNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.functionSignature->returnType == FLOAT_TYPE)
		{
			functionCallNode->registerIndex = getRegister(FLOAT_REG);
			char* returnfloatRegName = NULL;
			codeGenPrepareRegister(FLOAT_REG, functionCallNode->registerIndex, 0, 0, &returnfloatRegName);

			write1("fmv.s %s, s0\n", returnfloatRegName);

			codeGenSaveToMemoryIfPsuedoRegister(INT_REG, functionCallNode->registerIndex, returnfloatRegName);
		}
	}
}

