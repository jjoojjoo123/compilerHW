#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"
#include "symbolTable.h"
// This file is for reference only, you are not required to follow the implementation. //
// You only need to check for errors stated in the hw4 document. //
int g_anyErrorOccur = 0;

DATA_TYPE getBiggerType(DATA_TYPE dataType1, DATA_TYPE dataType2);
void processProgramNode(AST_NODE *programNode);
void processDeclarationNode(AST_NODE* declarationNode);
void declareIdList(AST_NODE* typeNode, SymbolAttributeKind isVariableOrTypeAttribute, int ignoreArrayFirstDimSize);
void declareFunction(AST_NODE* returnTypeNode);
void processDeclDimList(AST_NODE* variableDeclDimList, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize);
void processTypeNode(AST_NODE* typeNode);
void processBlockNode(AST_NODE* blockNode);
void processStmtNode(AST_NODE* stmtNode);
void processGeneralNode(AST_NODE *node);
void checkAssignOrExpr(AST_NODE* assignOrExprRelatedNode);
void checkWhileStmt(AST_NODE* whileNode);
void checkForStmt(AST_NODE* forNode);
void checkAssignmentStmt(AST_NODE* assignmentNode);
void checkIfStmt(AST_NODE* ifNode);
void checkWriteFunction(AST_NODE* functionCallNode);
void checkFunctionCall(AST_NODE* functionCallNode);
void processExprRelatedNode(AST_NODE* exprRelatedNode);
void checkParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter);
void checkReturnStmt(AST_NODE* returnNode);
void processExprNode(AST_NODE* exprNode);
void processVariableLValue(AST_NODE* idNode);
void processVariableRValue(AST_NODE* idNode);
void processConstValueNode(AST_NODE* constValueNode);
void getExprOrConstValue(AST_NODE* exprOrConstNode, int* iValue, float* fValue);
void evaluateExprValue(AST_NODE* exprNode);


typedef enum ErrorMsgKind
{
    SYMBOL_IS_NOT_TYPE,
    SYMBOL_REDECLARE,
    SYMBOL_UNDECLARED,
    NOT_FUNCTION_NAME,
    TRY_TO_INIT_ARRAY,
    EXCESSIVE_ARRAY_DIM_DECLARATION,
    RETURN_ARRAY,
    VOID_VARIABLE,
    TYPEDEF_VOID_ARRAY,
    PARAMETER_TYPE_UNMATCH,
    TOO_FEW_ARGUMENTS,
    TOO_MANY_ARGUMENTS,
    RETURN_TYPE_UNMATCH,
    INCOMPATIBLE_ARRAY_DIMENSION,
    NOT_ASSIGNABLE,
    NOT_ARRAY,
    IS_TYPE_NOT_VARIABLE,
    IS_FUNCTION_NOT_VARIABLE,
    STRING_OPERATION,
    ARRAY_SIZE_NOT_INT,
    ARRAY_SIZE_NEGATIVE,
    ARRAY_SUBSCRIPT_NOT_INT,
    PASS_ARRAY_TO_SCALAR,
    PASS_SCALAR_TO_ARRAY
} ErrorMsgKind;

void printErrorMsgSpecial(AST_NODE* node1, char* name2, ErrorMsgKind errorMsgKind)
{
    g_anyErrorOccur = 1;
    printf("Error found in line %d\n", node1->linenumber);
    /*
    switch(errorMsgKind)
    {
    default:
        printf("Unhandled case in void printErrorMsg(AST_NODE* node, ERROR_MSG_KIND* errorMsgKind)\n");
        break;
    }
    */
     switch (errorMsgKind) {
        case SYMBOL_UNDECLARED:
            printf("ID <%s> undeclared.\n", name2);
            break;
        case SYMBOL_REDECLARE:
            printf("ID <%s> redeclared.\n", name2);
            break;
        case TOO_FEW_ARGUMENTS:
            printf("too few arguments to function <%s>.\n", name2);
            break;
        case TOO_MANY_ARGUMENTS:
            printf("too many arguments to function <%s>.\n", name2);
            break;
        case PASS_ARRAY_TO_SCALAR:
            printf("Array <%s> passed to scalar parameter <%s>.\n", node1->semantic_value.identifierSemanticValue.identifierName, name2);
            break;
        case PASS_SCALAR_TO_ARRAY:
            printf("Scalar <%s> passed to array parameter <%s>.\n", node1->semantic_value.identifierSemanticValue.identifierName, name2);
            break;
        case SYMBOL_IS_NOT_TYPE:
            printf("Type <%s> is not a valid type", name2);
            break;
        default:
            printf("Unhandled case in void printErrorMsg(AST_NODE* node, ERROR_MSG_KIND* errorMsgKind)\n");
            break;
    }
}


void printErrorMsg(AST_NODE* node, ErrorMsgKind errorMsgKind)
{
    g_anyErrorOccur = 1;
    printf("Error found in line %d\n", node->linenumber);
    /*
    switch(errorMsgKind)
    {
        printf("Unhandled case in void printErrorMsg(AST_NODE* node, ERROR_MSG_KIND* errorMsgKind)\n");
        break;
    }
    */
    switch (errorMsgKind) {
        case RETURN_TYPE_UNMATCH:
            puts("Incompatible return type.");
            break;
        case INCOMPATIBLE_ARRAY_DIMENSION:
            puts("Incompatible array dimensions.");
            break;
        case ARRAY_SUBSCRIPT_NOT_INT:
            printf("Array subscript is not an integer.");
            break;
        default:
            printf("Unhandled case in void printErrorMsg(AST_NODE* node, ERROR_MSG_KIND* errorMsgKind)\n");
            break;
    }
}


void semanticAnalysis(AST_NODE *root)
{
    processProgramNode(root);
}


DATA_TYPE getBiggerType(DATA_TYPE dataType1, DATA_TYPE dataType2)
{
    if(dataType1 == FLOAT_TYPE || dataType2 == FLOAT_TYPE) {
        return FLOAT_TYPE;
    } else {
        return INT_TYPE;
    }
}


void processProgramNode(AST_NODE *programNode)
{
    programNode->dataType = NONE_TYPE;

    TypeDescriptor* inttype = (TypeDescriptor*)malloc(sizeof(TypeDescriptor));
    inttype->kind = SCALAR_TYPE_DESCRIPTOR;
    inttype->properties.dataType = INT_TYPE;
    SymbolAttribute* intattr = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
    intattr->attributeKind = TYPE_ATTRIBUTE;
    intattr->attr.typeDescriptor = inttype;
    enterSymbol(SYMBOL_TABLE_INT_NAME, intattr);

    TypeDescriptor* floattype = (TypeDescriptor*)malloc(sizeof(TypeDescriptor));
    floattype->kind = SCALAR_TYPE_DESCRIPTOR;
    floattype->properties.dataType = FLOAT_TYPE;
    SymbolAttribute* floatattr = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
    floatattr->attributeKind = TYPE_ATTRIBUTE;
    floatattr->attr.typeDescriptor = floattype;
    enterSymbol(SYMBOL_TABLE_FLOAT_NAME, floatattr);

    TypeDescriptor* voidtype = (TypeDescriptor*)malloc(sizeof(TypeDescriptor));
    voidtype->kind = SCALAR_TYPE_DESCRIPTOR;
    voidtype->properties.dataType = VOID_TYPE;
    SymbolAttribute* voidattr = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
    voidattr->attributeKind = TYPE_ATTRIBUTE;
    voidattr->attr.typeDescriptor = voidtype;
    enterSymbol(SYMBOL_TABLE_VOID_NAME, voidattr);

    AST_NODE* currentNode = programNode->child;
    AST_NODE* currentDeclareVariable = NULL;
    while(currentNode){
        if(currentNode->nodeType == VARIABLE_DECL_LIST_NODE){
            /*currentDeclareVariable = currentNode->child;
            while(currentDeclareVariable){
                processDeclarationNode(currentDeclareVariable);
            }*/
            processGeneralNode(currentNode);
        }else{
            processDeclarationNode(currentNode);
        }
        if(currentNode->dataType == ERROR_TYPE){
            programNode->dataType = ERROR_TYPE;
        }
        currentNode = currentNode->rightSibling;
    }
}

void processDeclarationNode(AST_NODE* declarationNode)
{
    AST_NODE* typeNode = declarationNode->child;
    processTypeNode(typeNode);
    if(typeNode->dataType == ERROR_TYPE){
        //error propagate
        declarationNode->dataType = ERROR_TYPE;
        return;
    }
    switch(declarationNode->semantic_value.declSemanticValue.kind){
        case VARIABLE_DECL:
            declareIdList(declarationNode, VARIABLE_ATTRIBUTE, 0);
            break;
        case TYPE_DECL:
            declareIdList(declarationNode, TYPE_ATTRIBUTE, 0);
            break;
        case FUNCTION_DECL:
            declareFunction(declarationNode);
            break;
        case FUNCTION_PARAMETER_DECL:
            declareIdList(declarationNode, VARIABLE_ATTRIBUTE, 1);
            break;
        /*default:
            printf("WTF in processDeclarationNode\n");*/
    }
}


void processTypeNode(AST_NODE* idNodeAsType)
{
    SymbolTableEntry* entry = retrieveSymbol(idNodeAsType->semantic_value.identifierSemanticValue.identifierName);
    if(!entry || entry->attribute->attributeKind != TYPE_ATTRIBUTE){
        idNodeAsType->dataType = ERROR_TYPE;
        printErrorMsgSpecial(idNodeAsType, idNodeAsType->semantic_value.identifierSemanticValue.identifierName, SYMBOL_IS_NOT_TYPE);
    }else{
        idNodeAsType->semantic_value.identifierSemanticValue.symbolTableEntry = entry;
        if(entry->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR){
            idNodeAsType->dataType = entry->attribute->attr.typeDescriptor->properties.dataType;
        }else{
            idNodeAsType->dataType = entry->attribute->attr.typeDescriptor->properties.arrayProperties.elementType;
        }
    }
}


void declareIdList(AST_NODE* declarationNode, SymbolAttributeKind isVariableOrTypeAttribute, int ignoreArrayFirstDimSize)
{
    declarationNode->dataType = NONE_TYPE;
    AST_NODE* typeNode = declarationNode->child;
    AST_NODE* currentNode = typeNode->rightSibling;
    if(typeNode->dataType == VOID_TYPE && isVariableOrTypeAttribute == VARIABLE_ATTRIBUTE){
        printErrorMsgSpecial(declarationNode, currentNode->semantic_value.identifierSemanticValue.identifierName, VOID_VARIABLE);
        declarationNode->dataType = ERROR_TYPE;
        return;
    }
    while(currentNode){
    	
        if(declaredLocally(currentNode->semantic_value.identifierSemanticValue.identifierName)){
            printErrorMsgSpecial(currentNode, currentNode->semantic_value.identifierSemanticValue.identifierName,SYMBOL_REDECLARE);
            declarationNode->dataType = ERROR_TYPE;
        }else{
            SymbolAttribute* attribute = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
            //TypeDescriptor* typeDescriptor = (TypeDescriptor*)malloc(sizeof(TypeDescriptor));
            attribute->attributeKind = isVariableOrTypeAttribute;
            //attribute->attr.typeDescriptor = typeDescriptor;
            currentNode->dataType = NONE_TYPE;
            switch(currentNode->semantic_value.identifierSemanticValue.kind){
                case WITH_INIT_ID:
                    processExprRelatedNode(currentNode->child);
                    if(currentNode->child->dataType == ERROR_TYPE){
                        currentNode->dataType = ERROR_TYPE;
                        break;
                    }
                case NORMAL_ID:
                    attribute->attr.typeDescriptor = typeNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.typeDescriptor;
                    break;
                case ARRAY_ID:
                    attribute->attr.typeDescriptor = (TypeDescriptor*)malloc(sizeof(TypeDescriptor));
                    attribute->attr.typeDescriptor->kind = ARRAY_TYPE_DESCRIPTOR;
                    attribute->attr.typeDescriptor->properties.arrayProperties.elementType = typeNode->dataType;
                    processDeclDimList(currentNode, attribute->attr.typeDescriptor, ignoreArrayFirstDimSize);
                    break;
            }
            if(currentNode->dataType == ERROR_TYPE){
                declarationNode->dataType = ERROR_TYPE;
                free(attribute);
            }else{
                declarationNode->semantic_value.identifierSemanticValue.symbolTableEntry = enterSymbol(currentNode->semantic_value.identifierSemanticValue.identifierName, attribute);
            }
        }
        currentNode = currentNode->rightSibling;
    }
}

void checkAssignOrExpr(AST_NODE* assignOrExprRelatedNode)
{
    //test -> STMT_NODE(ASSIGN_STMT/FUNCTION_CALL_STMT) / EXPR_NODE
    if(assignOrExprRelatedNode->nodeType == STMT_NODE){
        if(assignOrExprRelatedNode->semantic_value.stmtSemanticValue.kind == ASSIGN_STMT){
            checkAssignmentStmt(assignOrExprRelatedNode);
        }else{
            checkFunctionCall(assignOrExprRelatedNode);
        }
    }else{
        processExprRelatedNode(assignOrExprRelatedNode);
    }
}

void checkWhileStmt(AST_NODE* whileNode)
{
    AST_NODE* testNode = whileNode->child;
    checkAssignOrExpr(testNode);
    processStmtNode(testNode->rightSibling);
    if(testNode->dataType == ERROR_TYPE || testNode->rightSibling->dataType == ERROR_TYPE){
        whileNode->dataType = ERROR_TYPE;
    }else{
        whileNode->dataType = NONE_TYPE;
    }
}


void checkForStmt(AST_NODE* forNode)
{
    AST_NODE* assignExprListNode1 = forNode->child;
    AST_NODE* relopExprListNode = assignExprListNode1->rightSibling;
    AST_NODE* assignExprListNode2 = relopExprListNode->rightSibling;
    AST_NODE* stmtNode = assignExprListNode2->rightSibling;
    processGeneralNode(assignExprListNode1);
    processGeneralNode(relopExprListNode);
    processGeneralNode(assignExprListNode2);
    processStmtNode(stmtNode);
    if(assignExprListNode1->dataType == ERROR_TYPE || relopExprListNode->dataType == ERROR_TYPE || assignExprListNode2->dataType == ERROR_TYPE || stmtNode->dataType == ERROR_TYPE){
        forNode->dataType = ERROR_TYPE;
    }else{
        forNode->dataType = NONE_TYPE;
    }
}


void checkAssignmentStmt(AST_NODE* assignmentNode)
{
    AST_NODE* varNode = assignmentNode->child;
    AST_NODE* relopNode = varNode->rightSibling;
    processVariableLValue(varNode);
    processExprRelatedNode(relopNode);
    if(varNode->dataType == ERROR_TYPE || relopNode->dataType == ERROR_TYPE){
        assignmentNode->dataType = ERROR_TYPE;
    }else{
        assignmentNode->dataType = getBiggerType(varNode->dataType, relopNode->dataType);
    }
}


void checkIfStmt(AST_NODE* ifNode)
{
    AST_NODE* testNode = ifNode->child;
    AST_NODE* stmtNode = testNode->rightSibling;
    AST_NODE* elseNode = stmtNode->rightSibling;
    checkAssignOrExpr(testNode);
    processStmtNode(stmtNode);
    processStmtNode(elseNode);
    if(testNode->dataType == ERROR_TYPE || stmtNode->dataType == ERROR_TYPE || elseNode->dataType == ERROR_TYPE){
        ifNode->dataType = ERROR_TYPE;
    }else{
        ifNode->dataType = NONE_TYPE;
    }
}

void checkWriteFunction(AST_NODE* functionCallNode)
{
    AST_NODE* idNode = functionCallNode->child;

    AST_NODE* actualParameterList = idNode->rightSibling;
    processGeneralNode(actualParameterList);

    AST_NODE* actualParameter = actualParameterList->child;
    
    int actualParameterNumber = 0;
    while(actualParameter)
    {
        ++actualParameterNumber;
        if(actualParameter->dataType == ERROR_TYPE){
            functionCallNode->dataType = ERROR_TYPE;
        }else if(actualParameter->dataType != INT_TYPE &&
                actualParameter->dataType != FLOAT_TYPE &&
                actualParameter->dataType != CONST_STRING_TYPE){
            printErrorMsg(actualParameter, PARAMETER_TYPE_UNMATCH);
            functionCallNode->dataType = ERROR_TYPE;
        }
        actualParameter = actualParameter->rightSibling;
    }
    
    if(actualParameterNumber > 1){
        printErrorMsgSpecial(idNode, idNode->semantic_value.identifierSemanticValue.identifierName, TOO_MANY_ARGUMENTS);
        functionCallNode->dataType = ERROR_TYPE;
    }else if(actualParameterNumber < 1){
        printErrorMsgSpecial(idNode, idNode->semantic_value.identifierSemanticValue.identifierName, TOO_FEW_ARGUMENTS);
        functionCallNode->dataType = ERROR_TYPE;
    }else{
        functionCallNode->dataType = VOID_TYPE;
    }
}

void checkFunctionCall(AST_NODE* functionCallNode)
{
    AST_NODE* idNode = functionCallNode->child;
    if(strcmp(idNode->semantic_value.identifierSemanticValue.identifierName, "write") == 0){
        checkWriteFunction(functionCallNode);
        return;
    }
    
    AST_NODE* argListNode = idNode->rightSibling;
    SymbolTableEntry* entry = retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName);
    if(!entry){
        printErrorMsgSpecial(idNode, idNode->semantic_value.identifierSemanticValue.identifierName, SYMBOL_UNDECLARED);
        idNode->dataType = ERROR_TYPE;
        functionCallNode->dataType = ERROR_TYPE;
        return;
    }
    //we should check all params no matter if id is function or not
    processGeneralNode(argListNode); //may cause currentArgument ERROR_TYPE
    if(entry->attribute->attributeKind != FUNCTION_SIGNATURE){
        printErrorMsgSpecial(idNode, idNode->semantic_value.identifierSemanticValue.identifierName, NOT_FUNCTION_NAME);
        idNode->dataType = ERROR_TYPE;
        functionCallNode->dataType = ERROR_TYPE;
        return;
    }
    AST_NODE* currentArgument = argListNode->child;
    Parameter* currentParameter = entry->attribute->attr.functionSignature->parameterList;
    int errors = 0;
    while(currentArgument && currentParameter){
        checkParameterPassing(currentParameter, currentArgument);
        if(currentArgument->dataType == ERROR_TYPE){
            errors++;
        }
        currentArgument = currentArgument->rightSibling;
        currentParameter = currentParameter->next;
    }
    if(errors){
        functionCallNode->dataType = ERROR_TYPE;
    }else if(currentArgument){
        printErrorMsgSpecial(functionCallNode, idNode->semantic_value.identifierSemanticValue.identifierName, TOO_MANY_ARGUMENTS);
    }else if(currentParameter){
        printErrorMsgSpecial(functionCallNode, idNode->semantic_value.identifierSemanticValue.identifierName, TOO_FEW_ARGUMENTS);
    }else{
        functionCallNode->dataType = entry->attribute->attr.functionSignature->returnType;
    }
}

void checkParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter)
{
    if(actualParameter->dataType == ERROR_TYPE){
        return;
    }
    int argumentArray = (actualParameter->dataType == INT_PTR_TYPE || actualParameter->dataType == FLOAT_PTR_TYPE);
	if(formalParameter->type->kind == SCALAR_TYPE_DESCRIPTOR && argumentArray){
        printErrorMsgSpecial(actualParameter, formalParameter->parameterName, PASS_ARRAY_TO_SCALAR);
        actualParameter->dataType = ERROR_TYPE;
    }else if(formalParameter->type->kind == ARRAY_TYPE_DESCRIPTOR && !argumentArray){
        printErrorMsgSpecial(actualParameter, formalParameter->parameterName, PASS_SCALAR_TO_ARRAY);
        actualParameter->dataType = ERROR_TYPE;
    }
}


void processExprRelatedNode(AST_NODE* exprRelatedNode)
{
    switch(exprRelatedNode->nodeType){
        case EXPR_NODE:
            processExprNode(exprRelatedNode);
            break;
        case CONST_VALUE_NODE:
            processConstValueNode(exprRelatedNode);
            break;
        case STMT_NODE:
            checkFunctionCall(exprRelatedNode);
            break;
        case IDENTIFIER_NODE:
            //rel_op will be rvalue
            processVariableRValue(exprRelatedNode);
            break;
    }
}

void getExprOrConstValue(AST_NODE* exprOrConstNode, int* iValue, float* fValue)
{
    //get evaluated constexpr?
    if(exprOrConstNode->nodeType == EXPR_NODE){
        if(exprOrConstNode->dataType == INT_TYPE){
            if(iValue){
                *iValue = exprOrConstNode->semantic_value.exprSemanticValue.constEvalValue.iValue;
            }
            if(fValue){
                *fValue = exprOrConstNode->semantic_value.exprSemanticValue.constEvalValue.iValue;
            }
        }else{
            if(iValue){
                *iValue = exprOrConstNode->semantic_value.exprSemanticValue.constEvalValue.fValue;
            }
            if(fValue){
                *fValue = exprOrConstNode->semantic_value.exprSemanticValue.constEvalValue.fValue;
            }
        }
    }else{
        if(exprOrConstNode->dataType == INT_TYPE){
            if(iValue){
                *iValue = exprOrConstNode->semantic_value.const1->const_u.intval;
            }
            if(fValue){
                *fValue = exprOrConstNode->semantic_value.const1->const_u.intval;
            }
        }else{
            if(iValue){
                *iValue = exprOrConstNode->semantic_value.const1->const_u.fval;
            }
            if(fValue){
                *fValue = exprOrConstNode->semantic_value.const1->const_u.fval;
            }
        }
    }
}

void evaluateExprValue(AST_NODE* exprNode)
{
    //evaluate constexpr
    if(exprNode->semantic_value.exprSemanticValue.kind == UNARY_OPERATION){
        AST_NODE* operand = exprNode->child;
        if(operand->dataType == INT_TYPE){
            int i;
            getExprOrConstValue(operand, &i, NULL);
            exprNode->dataType = INT_TYPE;
            switch(exprNode->semantic_value.exprSemanticValue.op.unaryOp){
                case UNARY_OP_POSITIVE:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = +i;
                    break;
                case UNARY_OP_NEGATIVE:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = -i;
                    break;
                case UNARY_OP_LOGICAL_NEGATION:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = !i;
                    break;
            }
        }else{
            float i;
            getExprOrConstValue(operand, NULL, &i);
            exprNode->dataType = FLOAT_TYPE;
            switch(exprNode->semantic_value.exprSemanticValue.op.unaryOp){
                case UNARY_OP_POSITIVE:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = +i;
                    break;
                case UNARY_OP_NEGATIVE:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = -i;
                    break;
                case UNARY_OP_LOGICAL_NEGATION:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = !i;
                    break;
            }
        }
    }else{
        AST_NODE* operand1 = exprNode->child;
        AST_NODE* operand2 = operand1->rightSibling;
        if(operand1->dataType == INT_TYPE && operand2->dataType == INT_TYPE){
            int i, j;
            getExprOrConstValue(operand1, &i, NULL);
            getExprOrConstValue(operand2, &j, NULL);
            exprNode->dataType = INT_TYPE;
            switch(exprNode->semantic_value.exprSemanticValue.op.binaryOp){
                case BINARY_OP_ADD:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = i + j;
                    break;
                case BINARY_OP_SUB:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = i - j;
                    break;
                case BINARY_OP_MUL:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = i * j;
                    break;
                case BINARY_OP_DIV:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = i / j;
                    break;
                case BINARY_OP_EQ:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = i == j;
                    break;
                case BINARY_OP_GE:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = i >= j;
                    break;
                case BINARY_OP_LE:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = i <= j;
                    break;
                case BINARY_OP_NE:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = i != j;
                    break;
                case BINARY_OP_GT:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = i > j;
                    break;
                case BINARY_OP_LT:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = i < j;
                    break;
                case BINARY_OP_AND:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = i && j;
                    break;
                case BINARY_OP_OR:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = i || j;
                    break;
            }
        }else{
            float i, j;
            getExprOrConstValue(operand1, NULL, &i);
            getExprOrConstValue(operand2, NULL, &j);
            exprNode->dataType = FLOAT_TYPE;
            switch(exprNode->semantic_value.exprSemanticValue.op.binaryOp){
                case BINARY_OP_ADD:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = i + j;
                    break;
                case BINARY_OP_SUB:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = i - j;
                    break;
                case BINARY_OP_MUL:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = i * j;
                    break;
                case BINARY_OP_DIV:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = i / j;
                    break;
                case BINARY_OP_EQ:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = i == j;
                    break;
                case BINARY_OP_GE:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = i >= j;
                    break;
                case BINARY_OP_LE:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = i <= j;
                    break;
                case BINARY_OP_NE:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = i != j;
                    break;
                case BINARY_OP_GT:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = i > j;
                    break;
                case BINARY_OP_LT:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = i < j;
                    break;
                case BINARY_OP_AND:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = i && j;
                    break;
                case BINARY_OP_OR:
                    exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = i || j;
                    break;
            }
        }
    }
}


void processExprNode(AST_NODE* exprNode)
{
    if(exprNode->semantic_value.exprSemanticValue.kind == UNARY_OPERATION){
        AST_NODE* operand = exprNode->child;
        processExprRelatedNode(operand);
        if(operand->dataType == ERROR_TYPE){
            exprNode->dataType = ERROR_TYPE;
        }else if(operand->dataType == INT_PTR_TYPE || operand->dataType == FLOAT_PTR_TYPE){
            printErrorMsg(operand, INCOMPATIBLE_ARRAY_DIMENSION);
            exprNode->dataType = ERROR_TYPE;
        }else{
            exprNode->dataType = operand->dataType;
            //operand is a expr related node, so check EXPR_NODE
            if(operand->nodeType == CONST_VALUE_NODE || (operand->nodeType == EXPR_NODE && operand->semantic_value.exprSemanticValue.isConstEval)){
                evaluateExprValue(exprNode);
                exprNode->semantic_value.exprSemanticValue.isConstEval = 1;
            }
        }
    }else{
        AST_NODE* operand1 = exprNode->child;
        AST_NODE* operand2 = operand1->rightSibling;
        processExprRelatedNode(operand1);
        processExprRelatedNode(operand2);
        if(operand1->dataType == ERROR_TYPE || operand2->dataType == ERROR_TYPE){
            exprNode->dataType = ERROR_TYPE;
        }else{
            int error = 0;
            if(operand1->dataType == INT_PTR_TYPE || operand1->dataType == FLOAT_PTR_TYPE){
                printErrorMsg(operand1, INCOMPATIBLE_ARRAY_DIMENSION);
                error++;
            }
            if(operand2->dataType == INT_PTR_TYPE || operand2->dataType == FLOAT_PTR_TYPE){
                printErrorMsg(operand2, INCOMPATIBLE_ARRAY_DIMENSION);
                error++;
            }
            if(error){
                exprNode->dataType = ERROR_TYPE;
            }else{
                exprNode->dataType = getBiggerType(operand1->dataType, operand2->dataType);
                if((operand1->nodeType == CONST_VALUE_NODE || (operand1->nodeType == EXPR_NODE && operand1->semantic_value.exprSemanticValue.isConstEval)) && 
                    (operand2->nodeType == CONST_VALUE_NODE || (operand2->nodeType == EXPR_NODE && operand2->semantic_value.exprSemanticValue.isConstEval))){
                    evaluateExprValue(exprNode);
                    exprNode->semantic_value.exprSemanticValue.isConstEval = 1;
                }
            }
        }
    }
}


void processVariableLValue(AST_NODE* idNode)
{
    SymbolTableEntry* entry = retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName);
    if(!entry){
        printErrorMsgSpecial(idNode, idNode->semantic_value.identifierSemanticValue.identifierName, SYMBOL_UNDECLARED);
        idNode->dataType = ERROR_TYPE;
        return;
    }else{
        idNode->semantic_value.identifierSemanticValue.symbolTableEntry = entry;
	    switch(entry->attribute->attributeKind){
	        case TYPE_ATTRIBUTE:
	            printErrorMsgSpecial(idNode, idNode->semantic_value.identifierSemanticValue.identifierName, IS_TYPE_NOT_VARIABLE);
	            idNode->dataType = ERROR_TYPE;
	            return;
	        case FUNCTION_SIGNATURE:
	            printErrorMsgSpecial(idNode, idNode->semantic_value.identifierSemanticValue.identifierName, IS_FUNCTION_NOT_VARIABLE);
	            idNode->dataType = ERROR_TYPE;
	            return;
	    }
	}
    TypeDescriptor *typeDescriptor = entry->attribute->attr.typeDescriptor;
    if(idNode->semantic_value.identifierSemanticValue.kind == NORMAL_ID){
        if(typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR){
            idNode->dataType = typeDescriptor->properties.dataType;
        }else{
            printErrorMsg(idNode, INCOMPATIBLE_ARRAY_DIMENSION);
            idNode->dataType = ERROR_TYPE;
        }
    }else{
        //must be ARRAY_ID, WITH_INIT_ID is impossible here
        //we should process all dim nodes no matter the variable is an array or not.
        int dim = 0, error = 0;
        AST_NODE* dimExprNode = idNode->child;
        while(dimExprNode){
            processExprRelatedNode(dimExprNode);
            if(dimExprNode->dataType == ERROR_TYPE){
                idNode->dataType = ERROR_TYPE;
                error++;
            }else if(dimExprNode->dataType != INT_TYPE){
                printErrorMsg(idNode, ARRAY_SUBSCRIPT_NOT_INT);
                idNode->dataType = ERROR_TYPE;
                error++;
            }
            dim++;
            dimExprNode = dimExprNode->rightSibling;
        }
        int dimInfo = (typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR) ? 0 : (typeDescriptor->properties.arrayProperties.dimension);
        if(dimInfo == 0){
            printErrorMsgSpecial(idNode, idNode->semantic_value.identifierSemanticValue.identifierName, NOT_ARRAY);
        }else if(dim > dimInfo){
            printErrorMsg(idNode, INCOMPATIBLE_ARRAY_DIMENSION);
        }else if(dim == dimInfo){
            idNode->dataType = typeDescriptor->properties.arrayProperties.elementType;
        }else{
            printErrorMsg(idNode, INCOMPATIBLE_ARRAY_DIMENSION);
        }
        if(error){
            idNode->dataType = ERROR_TYPE;
        }
    }
}

void processVariableRValue(AST_NODE* idNode)
{
    SymbolTableEntry* entry = retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName);
    if(!entry){
        printErrorMsgSpecial(idNode, idNode->semantic_value.identifierSemanticValue.identifierName, SYMBOL_UNDECLARED);
        idNode->dataType = ERROR_TYPE;
        return;
    }else{
        idNode->semantic_value.identifierSemanticValue.symbolTableEntry = entry;
        switch(entry->attribute->attributeKind){
            case TYPE_ATTRIBUTE:
                printErrorMsgSpecial(idNode, idNode->semantic_value.identifierSemanticValue.identifierName, IS_TYPE_NOT_VARIABLE);
                idNode->dataType = ERROR_TYPE;
                return;
            case FUNCTION_SIGNATURE:
                printErrorMsgSpecial(idNode, idNode->semantic_value.identifierSemanticValue.identifierName, IS_FUNCTION_NOT_VARIABLE);
                idNode->dataType = ERROR_TYPE;
                return;
        }
    }
    TypeDescriptor *typeDescriptor = entry->attribute->attr.typeDescriptor;
    if(idNode->semantic_value.identifierSemanticValue.kind == NORMAL_ID){
        if(typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR){
            idNode->dataType = typeDescriptor->properties.dataType;
        }else{
            idNode->dataType = (typeDescriptor->properties.arrayProperties.elementType == INT_TYPE) ? INT_PTR_TYPE : FLOAT_PTR_TYPE;
        }
    }else{
        //must be ARRAY_ID, WITH_INIT_ID is impossible here
        //we should process all dim nodes no matter the variable is an array or not.
        int dim = 0, error = 0;
        AST_NODE* dimExprNode = idNode->child;
        while(dimExprNode){
            processExprRelatedNode(dimExprNode);
            if(dimExprNode->dataType == ERROR_TYPE){
                idNode->dataType = ERROR_TYPE;
                error++;
            }else if(dimExprNode->dataType != INT_TYPE){
                printErrorMsg(idNode, ARRAY_SUBSCRIPT_NOT_INT);
                idNode->dataType = ERROR_TYPE;
                error++;
            }
            dim++;
            dimExprNode = dimExprNode->rightSibling;
        }
        int dimInfo = (typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR) ? 0 : (typeDescriptor->properties.arrayProperties.dimension);
        if(dimInfo == 0){
            printErrorMsgSpecial(idNode, idNode->semantic_value.identifierSemanticValue.identifierName, NOT_ARRAY);
        }else if(dim > dimInfo){
            printErrorMsg(idNode, INCOMPATIBLE_ARRAY_DIMENSION);
        }else if(dim == dimInfo){
            idNode->dataType = typeDescriptor->properties.arrayProperties.elementType;
        }else{
            idNode->dataType = (typeDescriptor->properties.arrayProperties.elementType == INT_TYPE) ? INT_PTR_TYPE : FLOAT_PTR_TYPE;
        }
        if(error){
            idNode->dataType = ERROR_TYPE;
        }
    }
}


void processConstValueNode(AST_NODE* constValueNode)
{
    switch(constValueNode->semantic_value.const1->const_type){
        case INTEGERC:
            constValueNode->dataType = INT_TYPE;
            break;
        case FLOATC:
            constValueNode->dataType = FLOAT_TYPE;
            break;
        case STRINGC:
            constValueNode->dataType = CONST_STRING_TYPE;
            break;
    }
}


void checkReturnStmt(AST_NODE* returnNode)
{
    AST_NODE* child = returnNode->child;
    //trace the function type
    AST_NODE* parent = returnNode->parent;
    while(parent->nodeType != DECLARATION_NODE){
        parent = parent->parent;
    }
    AST_NODE* typeNode = parent->child;
    int error = 0;
    if(child->nodeType == NUL_NODE){
        if(typeNode->dataType != VOID_TYPE){
            printErrorMsg(returnNode, RETURN_TYPE_UNMATCH);
            error++;
        }
    }else{
        processExprRelatedNode(child);
        if(child->dataType != typeNode->dataType){
            if(!((child->dataType == INT_TYPE || child->dataType == FLOAT_TYPE) && (typeNode->dataType == INT_TYPE || typeNode->dataType == FLOAT_TYPE))){
                printErrorMsg(returnNode, RETURN_TYPE_UNMATCH);
                error++;
            }
        }
    }
    if(error){
        returnNode->dataType = ERROR_TYPE;
    }else{
        returnNode->dataType = typeNode->dataType;
    }
}


void processBlockNode(AST_NODE* blockNode)
{
    //please open scope before calling me
    blockNode->dataType = NONE_TYPE;
    AST_NODE* whatList = blockNode->child;
    while(whatList){
        processGeneralNode(whatList);
        if(whatList->dataType == ERROR_TYPE){
            blockNode->dataType = ERROR_TYPE;
        }
        whatList = whatList->rightSibling;
    }
}


void processStmtNode(AST_NODE* stmtNode)
{
    if(stmtNode->nodeType == BLOCK_NODE){
        openScope();
        processBlockNode(stmtNode);
        closeScope();
    }else if(stmtNode->nodeType == STMT_NODE){
        switch(stmtNode->semantic_value.stmtSemanticValue.kind){
            case WHILE_STMT:
                checkWhileStmt(stmtNode);
                break;
            case FOR_STMT:
                checkForStmt(stmtNode);
                break;
            case ASSIGN_STMT:
                checkAssignmentStmt(stmtNode);
                break;
            case IF_STMT:
                checkIfStmt(stmtNode);
                break;
            case FUNCTION_CALL_STMT:
                checkFunctionCall(stmtNode);
                break;
            case RETURN_STMT:
                checkReturnStmt(stmtNode);
                break;
        }
    }else{
        //NUL_NODE
        stmtNode->dataType = NONE_TYPE;
    }
}


void processGeneralNode(AST_NODE *node)
{
    node->dataType = NONE_TYPE;
    AST_NODE* child = node->child;
    while(child){
        switch(node->nodeType){
            case VARIABLE_DECL_LIST_NODE:
                processDeclarationNode(child);
                break;
            case STMT_LIST_NODE:
                processStmtNode(child);
                break;
            case NONEMPTY_ASSIGN_EXPR_LIST_NODE:
                checkAssignmentStmt(child);
                break;
            case NONEMPTY_RELOP_EXPR_LIST_NODE:
                processExprRelatedNode(child);
                break;
        }
        if(child->dataType == ERROR_TYPE){
            node->dataType = ERROR_TYPE;
        }
        child = child->rightSibling;
    }
}

void processDeclDimList(AST_NODE* idNode, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize)
{
    typeDescriptor->properties.arrayProperties.dimension = 0;
    AST_NODE* dimListNode = idNode->child;
    if(ignoreFirstDimSize && dimListNode->nodeType == NUL_NODE){
        typeDescriptor->properties.arrayProperties.dimension++;
        typeDescriptor->properties.arrayProperties.sizeInEachDimension[0] = 0;
        dimListNode = dimListNode->rightSibling;
    }
    while(dimListNode){
        if(typeDescriptor->properties.arrayProperties.dimension >= MAX_ARRAY_DIMENSION){
            printErrorMsg(idNode, EXCESSIVE_ARRAY_DIM_DECLARATION);
            idNode->dataType = ERROR_TYPE;
            return;
        }
        processExprRelatedNode(dimListNode);
        if(dimListNode->dataType == ERROR_TYPE){
            idNode->dataType = ERROR_TYPE;
        }else if(dimListNode->dataType != INT_TYPE){
            printErrorMsg(dimListNode, ARRAY_SUBSCRIPT_NOT_INT);
            idNode->dataType = ERROR_TYPE;
        }else if(dimListNode->nodeType == CONST_VALUE_NODE && dimListNode->semantic_value.const1->const_u.intval < 0){
            printErrorMsg(dimListNode, ARRAY_SIZE_NEGATIVE);
            idNode->dataType = ERROR_TYPE;
        }else if(dimListNode->nodeType == EXPR_NODE && dimListNode->semantic_value.exprSemanticValue.isConstEval && dimListNode->semantic_value.exprSemanticValue.constEvalValue.iValue < 0){
            printErrorMsg(dimListNode, ARRAY_SIZE_NEGATIVE);
            idNode->dataType = ERROR_TYPE;
        }else{
            //but dim_fn accept expr, which may not be constexpr. how to do?
            typeDescriptor->properties.arrayProperties.sizeInEachDimension[typeDescriptor->properties.arrayProperties.dimension] =
                (dimListNode->nodeType == CONST_VALUE_NODE ? dimListNode->semantic_value.const1->const_u.intval : dimListNode->semantic_value.exprSemanticValue.constEvalValue.iValue);
        }
        typeDescriptor->properties.arrayProperties.dimension++;
        dimListNode = dimListNode->rightSibling;
    }
}


void declareFunction(AST_NODE* declarationNode)
{
    AST_NODE* typeNode = declarationNode->child;
    AST_NODE* idNode = typeNode->rightSibling;
    AST_NODE* paramListNode = idNode->rightSibling;
    AST_NODE* blockNode = paramListNode->rightSibling;
    declarationNode->dataType = NONE_TYPE;
    processTypeNode(typeNode);
    if(typeNode->dataType == ERROR_TYPE){
        declarationNode->dataType = ERROR_TYPE;
    }
    if(declaredLocally(idNode->semantic_value.identifierSemanticValue.identifierName)){
        printErrorMsgSpecial(idNode, idNode->semantic_value.identifierSemanticValue.identifierName, SYMBOL_REDECLARE);
        declarationNode->dataType = ERROR_TYPE;
    }
    SymbolAttribute* attribute = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
    attribute->attributeKind = FUNCTION_SIGNATURE;
    attribute->attr.functionSignature = (FunctionSignature*)malloc(sizeof(FunctionSignature));
    attribute->attr.functionSignature->parametersCount = 0;
    attribute->attr.functionSignature->parameterList = NULL;
    attribute->attr.functionSignature->returnType = typeNode->dataType;
    if(declarationNode->dataType != ERROR_TYPE){
        //temp add
        enterSymbol(idNode->semantic_value.identifierSemanticValue.identifierName, attribute);
    }
    openScope();
    AST_NODE* paramNode = paramListNode->child;
    Parameter* nowParam = NULL;
    int paramError = 0;
    while(paramNode){
        processDeclarationNode(paramNode);
        attribute->attr.functionSignature->parametersCount++;
        if(paramNode->dataType == ERROR_TYPE){
            paramError++;
        }
        Parameter* param = (Parameter*)malloc(sizeof(Parameter));
        param->next = NULL;
        param->type = (paramNode->dataType == ERROR_TYPE) ? NULL : paramNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.typeDescriptor;
        param->parameterName = paramNode->child->rightSibling->semantic_value.identifierSemanticValue.identifierName;
		if(!nowParam){
            attribute->attr.functionSignature->parameterList = param;
        }else{
            nowParam->next = param;
        }
        nowParam = param;
        paramNode = paramNode->rightSibling;
    }
    if(paramError){
        paramNode->dataType = ERROR_TYPE;
    }
    //block
    processBlockNode(blockNode);
    closeScope();
    if(declarationNode->dataType == ERROR_TYPE){
        removeSymbol(idNode->semantic_value.identifierSemanticValue.identifierName);
    }
    if(paramError || blockNode->dataType == ERROR_TYPE){
        declarationNode->dataType = ERROR_TYPE;
    }
    if(declarationNode->dataType == ERROR_TYPE){
        nowParam = attribute->attr.functionSignature->parameterList;
        while(nowParam){
            Parameter* freeParam = nowParam;
            nowParam = nowParam->next;
            free(freeParam);
        }
        free(attribute->attr.functionSignature);
        free(attribute);
    }
}
