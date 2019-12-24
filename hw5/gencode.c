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

int getArraySize(TypeDescriptor* idTypeDescriptor){
	ArrayProperties properties = idTypeDescriptor->properties.arrayProperties;
	int array_size = 0;
	switch(properties.elementType){
		case INT_TYPE:
		case FLOAT_TYPE:
			array_size = 4;
			break;
	}
	for(int i = 0; i < properties.dimension; i++) {
		array_size *= properties.sizeInEachDimension[i];
	}
	return array_size;
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
				
				float fValue = 0;
				int iValue = 0;
				int isInt = 0;
				if (idNode->semantic_value.identifierSemanticValue.kind == WITH_INIT_ID) {
					if(idNode->child->semantic_value.const1->const_type == FLOATC){
						fValue = idNode->child->semantic_value.const1->const_u.fval;
					}
					else{
						isInt = 1;
						iValue = idNode->child->semantic_value.const1->const_u.intval;
					}
				}
				
				if(idTypeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR){
					if(idTypeDescriptor->properties.dataType == INT_TYPE){
						write1("_g_%s: .word %d\n", idSymbolTableEntry->name, (isInt ? iValue : (int)fValue));
					}
					else if(idTypeDescriptor->properties.dataType == FLOAT_TYPE){
						write1("_g_%s: .float %f\n", idSymbolTableEntry->name, (isInt ? (float)iValue : fValue));
					}
				}
				else if(idTypeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR){
					int variableSize = getArraySize(idTypeDescriptor);
					write1("_g_%s: .space %d\n", idSymbolTableEntry->name, variableSize);
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
				/*if(listNode->regType == INT_REG)
				{
					free_int_reg(listNode->registerIndex);
				}
				else if(listNode->regType == FLOAT_TYPE)
				{
					free_float_reg(listNode->registerIndex);
				}*/
				//node->registerIndex = listNode->registerIndex;
				listNode = listNode->rightSibling;
			}
			break;
		/*case NONEMPTY_RELOP_EXPR_LIST_NODE:
			while(listNode)
			{
				gen_exprRelatedNode(listNode);
				if(listNode->regType == INT_REG)
				{
					free_int_reg(listNode->registerIndex);
				}
				else if(listNode->regType == FLOAT_TYPE)
				{
					free_float_reg(listNode->registerIndex);
				}
				//node->registerIndex = listNode->registerIndex;
				listNode = listNode->rightSibling;
			}
			break;*/
		case NUL_NODE:
			break;
		default:
			printf("Unhandle case in void processGeneralNode(AST_NODE *node)\n");
			node->dataType = ERROR_TYPE;
			break;
	}
}

void gen_stmtNode(AST_NODE* stmtNode){
	if(stmtNode->nodeType == BLOCK_NODE){
		gen_blockNode(stmtNode);
		return;
	}
	switch(stmtNode->semantic_value.stmtSemanticValue.kind){
		case WHILE_STMT:
			gen_while(stmtNode);
			break;
		case FOR_STMT:
			gen_for(stmtNode);
			break;
		case ASSIGN_STMT:
			gen_assign(stmtNode);
			break;
		case IF_STMT:
			gen_if(stmtNode);
			break;
		case FUNCTION_CALL_STMT:
			gen_functionCallWithoutCatchReturn(stmtNode);
			break;
		case RETURN_STMT:
			gen_return(stmtNode);
			break;
	}
}

void gen_test(AST_NODE* exprNode){
	gen_exprRelatedNode(exprNode);
	char* rd, rs1;
	if(exprNode->regType == FLOAT_REG){
		int newIndex = get_int_reg();
		rd = int_reg[newIndex];
		rs1 = float_reg[exprNode->registerIndex];
		write1("fabs.s %s, %s\n", rs1, rs1);
		write1("fclass.s %s, %s\n", rd, rs1);
		write1("subi %s, %s, 4\n", rd, rd);
		write1("snez %s, %s\n", rd, rd);
		free_float_reg(exprNode->registerIndex);
		exprNode->regType = INT_REG;
		exprNode->registerIndex = newIndex;
	}else{
		rd = int_reg[exprNode->registerIndex];
		exprNode->regType = INT_REG;
		write1("snez %s, %s\n", rd, rd);
	}
}

void gen_while(AST_NODE* whileNode){
	int label = (labelNumber++);
	AST_NODE* testNode = ifNode->child;
	AST_NODE* stmtNode = testNode->rightSibling;
	write0("_Test%d\n", label);
	gen_test(testNode);
	write1("beqz %s, _LExit%d\n", int_reg[testNode->registerIndex], label);
	free_int_reg(testNode->registerIndex);
	gen_stmtNode(stmtNode);
	write1("j _Test%d\n", label);
	write0("_LExit%d\n", label);
}

void gen_for(AST_NODE* forNode){
	//hw6
}

void gen_assign(AST_NODE* assignNode){
	AST_NODE* idNode = assignNode->child;
	AST_NODE* exprNode = idNode->rightSibling;
	gen_idNodeRef(idNode);
	gen_exprRelatedNode(exprNode);
	char* regName = int_reg[idNode->registerIndex];
	if(idNode->dataType == INT_TYPE && exprNode->regType == INT_REG){
		write1("sw %s, 0(%s)\n", int_reg[exprNode->registerIndex], regName);
		free_int_reg(exprNode->registerIndex);
	}else if(idNode->dataType == FLOAT_TYPE && exprNode->regType == FLOAT_REG){
		write1("fsw %s, 0(%s)\n", float_reg[exprNode->registerIndex], regName);
		free_float_reg(exprNode->registerIndex);
	}else{
		//hw6
	}
}

void gen_if(AST_NODE* ifNode){
	int label = (labelNumber++);
	AST_NODE* testNode = ifNode->child;
	AST_NODE* stmtNode = testNode->rightSibling;
	AST_NODE* elseNode = stmtNode->rightSibling;
	gen_test(testNode);
	if(elseNode->nodeType == NUL_NODE){
		write1("bnez %s, Lexit%d\n", int_reg[testNode->registerIndex], label);
	}else{
		write1("beqz %s, Lelse%d\n", int_reg[testNode->registerIndex], label);
	}
	free_int_reg(testNode->registerIndex);
	gen_stmtNode(stmtNode);
	if(elseNode->nodeType == NUL_NODE){
		write0("Lexit%d:\n", label);
	}else{
		write0("Lelse%d:\n", label);
		gen_stmtNode(elseNode);
		write0("Lexit%d:\n", label);
	}
}

void gen_return(AST_NODE* returnNode){
	AST_NODE* child = returnNode->child;
	if(child->nodeType != NUL_NODE){
		gen_exprRelatedNode(child);
		if(child->regType == INT_REG && returnNode->dataType == INT_TYPE){
			write1("mv %s, %s\n", a0, int_reg[child->registerIndex]);
			free_int_reg(child->registerIndex);
		}else if(child->regType == FLOAT_REG && returnNode->dataType == FLOAT_TYPE){
			write1("fmv.s %s, %s\n", fa0, float_reg[child->registerIndex]);
			free_float_reg(child->registerIndex);
		}else if(child->regType == INT_REG && returnNode->dataType == FLOAT_TYPE){
			//hw6
		}else if(child->regType == INT_REG && returnNode->dataType == FLOAT_TYPE){
			//hw6
		}
	}
}

void gen_assignOrExpr(AST_NODE* assignOrExprNode){
	if(assignOrExprNode->nodeType == STMT_NODE){
		if(assignOrExprNode->semantic_value.stmtSemanticValue.kind == FUNCTION_CALL_STMT){
			gen_functionCallWithoutCatchReturn(assignOrExprNode);
		}else{
			gen_assign(assignOrExprNode);
		}
	}else{
		gen_exprRelatedNode(assignOrExprNode);
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

void gen_idNodeRef(AST_NODE* idNode){
	SymbolTableEntry* entry = idNode->semantic_value.identifierSemanticValue.symbolTableEntry;
	char* name = idNode->semantic_value.identifierSemanticValue.identifierName;
	idNode->regType = PTR_REG;
	idNode->registerIndex = get_int_reg();
	char* regName = int_reg[idNode->registerIndex];
	if(idNode->semantic_value.identifierSemanticValue.kind == ARRAY_ID){
		ArrayProperties properties = idNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.typeDescriptor->properties.arrayProperties;
		int castIindex = -1;
		write1("mv %s, x0\n", regName);
		AST_NODE* child;
		for(int i = 0;child != NULL;i++, child = child->rightSibling){
			gen_exprRelatedNode(child);
			if(i > 0){
				write1("mul %s, %s, %d\n", regName, regName, properties.sizeInEachDimension[i]);
			}
			if(child->regType == INT_REG){
				write1("add %s, %s, %s\n", regName, regName, int_reg[child->registerIndex]);
				free_int_reg(child->registerIndex);
			}else{
				castIindex = get_int_reg();
				write1("fcvt.w.s %s, %s\n", int_reg[castIindex], float_reg[child->registerIndex]);
				write1("add %s, %s, %s\n", regName, regName, int_reg[castIindex]);
				free_int_reg(castIindex);
				free_float_reg(child->registerIndex);
			}
		}

		if(entry->nestingLevel > 0){
			write1("add %s, %s, fp\n", tmpName, tmpName);
			write1("addi %s, %s, %d\n", tmpName, tmpName, entry->offset);
		}else{
			castIindex = get_int_reg();
			write1("la %s, _g_%s\n", int_reg[castIindex], name);
			write1("add %s, %s, %s\n", regName, regName, int_reg[castIindex]);
			free_int_reg(castIindex);
		}
	}else{
		if(entry->nestingLevel > 0){
			write1("add %s, fp, %d\n", regName, entry->offset);
		}else{
			write1("la %s, _g_%s\n", regName, name);
		}
	}
}

void gen_idNode(AST_NODE* idNode){
	//Rvalue
	SymbolTableEntry* entry = idNode->semantic_value.identifierSemanticValue.symbolTableEntry;
	char* name = idNode->semantic_value.identifierSemanticValue.identifierName;
	if(idNode->semantic_value.identifierSemanticValue.kind == ARRAY_ID){
		switch(idNode->dataType){
			case INT_PTR_TYPE:
			case FLOAT_PTR_TYPE:
				//hw6: multi dimension offset shift
				idNode->regType = PTR_REG;
				idNode->registerIndex = get_int_reg();
				if(entry->nestingLevel > 0){
					write1("add %s, fp, %d\n", int_reg[idNode->registerIndex], entry->offset);
				}else{
					write1("la %s, _g_%s\n", int_reg[idNode->registerIndex], name);
				}
				return;
		}
	}
	gen_idNodeRef(idNode);
	if(idNode->dataType == INT_TYPE){
		idNode->regType = INT_REG;
		write1("lw %s, 0(%s)\n", int_reg[idNode->registerIndex], int_reg[idNode->registerIndex]);
	}else{
		idNode->regType = FLOAT_REG;
		int newIndex = get_float_reg();
		write1("flw %s, 0(%s)\n", float_reg[newIndex], int_reg[idNode->registerIndex]);
		free_int_reg(idNode->registerIndex);
		idNode->registerIndex = newIndex;
	}
}

void gen_constNode(AST_NODE* constNode){
	switch(constNode->dataType){
		case INT_TYPE:
			gen_integer(constNode, constNode->semantic_value.const1->const_u.intval);
			break;
		case FLOAT_TYPE:
			gen_float(constNode, constNode->semantic_value.const1->const_u.fval);
			break;
		case CONST_STRING_TYPE:
			gen_stringConst(constNode, constNode->semantic_value.const1->const_u.sc);
			break;
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

void gen_stringConst(AST_NODE* node, char* sc){
	node->regType = PTR_REG;
	node->registerIndex = get_int_reg();
	int label = labelNumber++;
	write0(".CSTR%d:\n", label);

	int length = strlen(sc);
	sc[length - 1] = '\0';
	write1(".string %s\\000\"\n", sc);
	sc[length - 1] = '\"';

	write1(".align 4\n");

	write1("lui %s, %%hi(.CSTR%d)\n", int_reg[node->registerIndex], label);
	write1("addi %s, %s, %%lo(.CSTR%d)\n", int_reg[node->registerIndex], int_reg[node->registerIndex], label);
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
					write1("fabs.s %s, %s\n", rs1, rs1);
					write1("fclass.s %s, %s\n", rd, rs1);
					write1("slti %s, %s, 4\n", rd, rd);
					write1("seqz %s, %s\n", rd, rd);
					break;
			}
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

void gen_functionCallWithoutCatchReturn(AST_NODE* functionCallNode){
	AST_NODE* functionIdNode = functionCallNode->child;
	AST_NODE* parameterList = functionIdNode->rightSibling;
	if(strcmp(functionIdNode->semantic_value.identifierSemanticValue.identifierName, "write") == 0)
	{
		AST_NODE* firstParameter = parameterList->child;
		gen_exprRelatedNode(firstParameter);
		char* parameterRegName = NULL;
		switch(firstParameter->regType)
		{
			case INT_REG:
				parameterRegName = int_reg[firstParameter->registerIndex];
				write1("mv a0, %s\n", parameterRegName);
				write1("jal _write_int\n");
				free_int_reg(firstParameter->registerIndex);
				break;
			case FLOAT_REG:
				parameterRegName = float_reg[firstParameter->registerIndex];
				write1("fmv.s fa0, %s\n", parameterRegName);
				write1("jal _write_float\n");
				free_float_reg(firstParameter->registerIndex);
				break;
			case PTR_REG:
				parameterRegName = int_reg[firstParameter->registerIndex];
				write1("mv a0, %s\n", parameterRegName);
				write1("jal _write_str\n");
				free_int_reg(firstParameter->registerIndex);
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
		/*functionCallNode->regType = INT_REG;
		functionCallNode->registerIndex = get_int_reg();
		write1("mv %s, %s\n", int_reg[functionCallNode->registerIndex], a0);*/
	}
	else if(strcmp(functionIdNode->semantic_value.identifierSemanticValue.identifierName, "fread") == 0)
	{
		write1("jal _read_float\n");
		/*functionCallNode->regType = FLOAT_REG;
		functionCallNode->registerIndex = get_float_reg();
		write1("fmv.s %s, %s\n", float_reg[functionCallNode->registerIndex], fa0);*/
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
}

void gen_functionCall(AST_NODE* functionCallNode)
{
	gen_functionCallWithoutCatchReturn(functionCallNode);
	if (functionIdNode->semantic_value.identifierSemanticValue.symbolTableEntry) {
		if(functionIdNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.functionSignature->returnType == INT_TYPE)
		{
			functionCallNode->regType = INT_REG;
			functionCallNode->registerIndex = get_int_reg();
			char* returnIntRegName = int_reg[functionCallNode->registerIndex];

			write1("mv %s, %s\n", returnIntRegName, a0);
		}
		else if(functionIdNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.functionSignature->returnType == FLOAT_TYPE)
		{
			functionCallNode->regType = FLOAT_REG;
			functionCallNode->registerIndex = get_float_reg();
			char* returnfloatRegName = float_reg[functionCallNode->registerIndex];

			write1("fmv.s %s, %s\n", returnfloatRegName, fa0);
		}
	}
}

