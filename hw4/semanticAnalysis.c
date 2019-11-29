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
    initializeSymbolTable();

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
            currentDeclareVariable = currentNode->child;
            while(currentDeclareVariable){
                processDeclarationNode(currentDeclareVariable);
            }
        }else{
            processDeclarationNode(currentNode);
        }
        currentNode = currentNode->rightSibling;
    }
    symbolTableEnd();
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
        default:
            printf("WTF in processDeclarationNode\n");
    }
}


void processTypeNode(AST_NODE* idNodeAsType)
{
    SymbolTableEntry* entry = retrieveSymbol(idNodeAsType->semantic_value.identifierSemanticValue.identifierName);
    if(!entry || entry->attribute->attributeKind != TYPE_ATTRIBUTE){
        idNodeAsType->dataType = ERROR_TYPE;
        printErrorMsg(idNodeAsType, SYMBOL_IS_NOT_TYPE);
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
    AST_NODE* typeNode = declarationNode->child;
    AST_NODE* currentNode = typeNode->rightSibling;
    while(currentNode){
        if(declaredLocally(currentNode->semantic_value.identifierSemanticValue.identifierName)){
            printErrorMsg(currentNode, SYMBOL_REDECLARE);
        }else{
            SymbolAttribute* attribute = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
            //TypeDescriptor* typeDescriptor = (TypeDescriptor*)malloc(sizeof(TypeDescriptor));
            attribute->attributeKind = isVariableOrTypeAttribute;
            //attribute->attr.typeDescriptor = typeDescriptor;
            switch(currentNode->semantic_value.identifierSemanticValue.kind){
                case WITH_INIT_ID:
                    processExprNode(currentNode->child);
                case NORMAL_ID:
                    attribute->attr.typeDescriptor = typeNode->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.typeDescriptor;
                    break;
                case ARRAY_ID:
                    attribute->attr.typeDescriptor = (TypeDescriptor*)malloc(sizeof(TypeDescriptor));
                    attribute->attr.typeDescriptor.kind = ARRAY_TYPE_DESCRIPTOR;
                    processDeclDimList(currentNode, attribute->attr.typeDescriptor, ignoreArrayFirstDimSize);
                    break;
            }
            enterSymbol(currentNode->semantic_value.identifierSemanticValue.identifierName, attribute);
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
}


void checkAssignmentStmt(AST_NODE* assignmentNode)
{
    AST_NODE* varNode = assignmentNode->child;
    AST_NODE* relopNode = varNode->rightSibling;
    processVariableLValue(varNode);
    processExprRelatedNode(relopNode);
    if(varNode->dataType == ERROR_TYPE || relopNode->dataType == ERROR_TYPE){
        assignmentNode->DATA_TYPE = ERROR_TYPE;
    }else{
        assignmentNode->DATA_TYPE = getBiggerType(varNode->dataType, relopNode->dataType);
    }
}


void checkIfStmt(AST_NODE* ifNode)
{
    AST_NODE* testNode = whileNode->child;
    AST_NODE* stmtNode = testNode->rightSibling;
    AST_NODE* elseNode = stmtNode->rightSibling;
    checkAssignOrExpr(testNode);
    processStmtNode(stmtNode);
    processStmtNode(elseNode);
}

void checkWriteFunction(AST_NODE* functionCallNode)
{
    //What is this?
}

void checkFunctionCall(AST_NODE* functionCallNode)
{
    AST_NODE* idNode = functionCallNode->child;
    AST_NODE* paramListNode = idNode->rightSibling;
    SymbolTableEntry* entry = retrieveSymbol(idNodeAsType->semantic_value.identifierSemanticValue.identifierName);
    if(!entry){
        printErrorMsg(idNode, SYMBOL_UNDECLARED);
        idNode->dataType = ERROR_TYPE;
        functionCallNode->dataType = ERROR_TYPE;
        return;
    }
    if(entry->attribute->attributeKind != FUNCTION_SIGNATURE){
        printErrorMsg(idNode, NOT_FUNCTION_NAME);
        idNode->dataType = ERROR_TYPE;
        functionCallNode->dataType = ERROR_TYPE;
        return;
    }
    processGeneralNode(paramListNode); //may cause currentArgument ERROR_TYPE
    AST_NODE* currentArgument = paramListNode->child;
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
        printErrorMsg(functionCallNode, TOO_MANY_ARGUMENTS);
    }else if(currentParameter){
        printErrorMsg(functionCallNode, TOO_FEW_ARGUMENTS);
    }else{
        functionCallNode->dataType = entry->attribute->attr.functionSignature.returnType;
    }
}

void checkParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter)
{
    if(actualParameter->dataType == ERROR_TYPE){
        return;
    }
    int argumentArray = (actualParameter->dataType == INT_PTR_TYPE || actualParameter->dataType == FLOAT_PTR_TYPE);
    if(formalParameter->type.kind = SCALAR_TYPE_DESCRIPTOR && argumentArray){
        printErrorMsgSpecial(actualParameter, formalParameter->parameterName, PASS_ARRAY_TO_SCALAR);
        actualParameter->dataType = ERROR_TYPE;
    }else if(formalParameter->type.kind = ARRAY_TYPE_DESCRIPTOR && !argumentArray){
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
    /*switch(exprRelatedNode->nodeType){
        case EXPR_NODE:
            processExprNode(exprRelatedNode);
            break;
        case CONST_VALUE_NODE:
            processConstValueNode(exprRelatedNode);
            break;
    }*/
}

void evaluateExprValue(AST_NODE* exprNode)
{
}


void processExprNode(AST_NODE* exprNode)
{
}


void processVariableLValue(AST_NODE* idNode)
{
    SymbolTableEntry* entry = retrieveSymbol(idNodeAsType->semantic_value.identifierSemanticValue.identifierName);
    if(!entry){
        printErrorMsg(idNode, SYMBOL_UNDECLARED);
        idNode->dataType = ERROR_TYPE;
        return;
    }
    switch(entry->attribute->attributeKind){
        case TYPE_ATTRIBUTE:
            printErrorMsg(idNode, IS_TYPE_NOT_VARIABLE);
            idNode->dataType = ERROR_TYPE;
            return;
        case FUNCTION_SIGNATURE:
            printErrorMsg(idNode, IS_FUNCTION_NOT_VARIABLE);
            idNode->dataType = ERROR_TYPE;
            return;
    }
}

void processVariableRValue(AST_NODE* idNode)
{
    SymbolTableEntry* entry = retrieveSymbol(idNodeAsType->semantic_value.identifierSemanticValue.identifierName);
    if(!entry){
        printErrorMsg(idNode, SYMBOL_UNDECLARED);
        idNode->dataType = ERROR_TYPE;
        return;
    }else{
        switch(entry->attribute->attributeKind){
            case TYPE_ATTRIBUTE:
                printErrorMsg(idNode, IS_TYPE_NOT_VARIABLE);
                idNode->dataType = ERROR_TYPE;
                return;
            case FUNCTION_SIGNATURE:
                printErrorMsg(idNode, IS_FUNCTION_NOT_VARIABLE);
                idNode->dataType = ERROR_TYPE;
                return;
        }
    }
}


void processConstValueNode(AST_NODE* constValueNode)
{
}


void checkReturnStmt(AST_NODE* returnNode)
{
}


void processBlockNode(AST_NODE* blockNode)
{
}


void processStmtNode(AST_NODE* stmtNode)
{
}


void processGeneralNode(AST_NODE *node)
{
}

void processDeclDimList(AST_NODE* idNode, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize)
{
}


void declareFunction(AST_NODE* declarationNode)
{
}
