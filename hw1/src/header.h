#ifndef HEADER_H_INCLUDED
#define HEADER_H_INCLUDED

/******************************************************************************************************************************************
    All enumeration literals
       TokenType : Specify the type of the token scanner returns
     DataType  : The data type of the declared variable
     StmtType  : Indicate one statement in AcDc program is print or assignment statement.
     ValueType : The node types of the expression tree that represents the expression on the right hand side of the assignment statement.
                 Identifier, IntConst, FloatConst must be the leaf nodes ex: a, b, c , 1.5 , 3.
           PlusNode, MinusNode, MulNode, DivNode are the operations in AcDc. They must be the internal nodes.
                   Note that IntToFloatConvertNode to represent the type coercion may appear after finishing type checking.         
     Operation : Specify all arithematic expression, including +, - , *, / and type coercion.
*******************************************************************************************************************************************/

typedef enum TokenType { FloatDeclaration, IntegerDeclaration, PrintOp, AssignmentOp, PlusOp, MinusOp,
             MulOp, DivOp, Alphabet, IntValue, FloatValue, EOFsymbol } TokenType;
typedef enum DataType { Int, Float, Notype }DataType;
typedef enum StmtType { Print, Assignment } StmtType;
typedef enum ValueType { Identifier, IntConst, FloatConst, PlusNode, MinusNode, MulNode, DivNode, IntToFloatConvertNode } ValueType;
typedef enum Operation { Plus, Minus, Mul, Div, Assign, IntToFloatConvert } Operation;


/**************************************************************************************** 
   All structures to facilitate the processes of 
   scanning, parsing, AST, type-checking, building the symbol table, and code generation.
*****************************************************************************************/

typedef char *VarName;

/* For scanner */
typedef struct Token{
    TokenType type;
    char tok[1025];
}Token;

/*** The following are nodes of the AST. ***/

/* For decl production or say one declaration statement */
typedef struct Declaration{
    DataType type;
    VarName name;
}Declaration;

/* 
    For decls production or say all declarations. (
  You can view it as the subtree for decls in AST,
  or just view it as the linked list that stores 
  all declarations. ) 
*/
typedef struct Declarations{
    Declaration first;
    struct Declarations *rest;
}Declarations;

/* For the nodes of the expression on the right hand side of one assignment statement */
typedef struct Value{
    ValueType type;
    union{
        VarName id;                /* if the node represent the access of the identifier */
        Operation op;              /* store +, -, *, /, =, type_convert */
        int ivalue;                /* for integer constant in the expression */
        float fvalue;              /* for float constant */
    }val;
}Value;


/* 
   The data structure of the expression tree.
   Recall how to deal with expression by tree 
   in data structure course.   
*/
typedef struct Expression{
    Value v;
    struct Expression *leftOperand;
    struct Expression *rightOperand;
    DataType type;
}Expression;


/* For one assignment statement */
typedef struct AssignmentStatement{
    VarName id;
    Expression *expr;
    DataType type;      /* For type checking to store the type of all expression on the right. */
}AssignmentStatement;


/* For stmt production or say one statement*/
typedef struct Statement{
    StmtType type;
    union{
        VarName variable;              /* print statement */
        AssignmentStatement assign;
    }stmt;
}Statement;

/* For stmts production or say all productions */
typedef struct Statements{
    struct Statement first;
    struct Statements *rest;
}Statements;

/* For the root of the AST. */
typedef struct Program{
    Declarations *declarations;
    Statements *statements;
}Program;

/* For building the symbol table */
/*typedef struct SymbolTable{
    DataType table[26];
} SymbolTable;*/

typedef struct NameValueTuple{
    VarName name;
    union{
        DataType type;
        VarName newname;
    }value;
} NameValueTuple;

typedef struct NameValueTuples{
    NameValueTuple tuple;
    struct NameValueTuples *tuples;
} NameValueTuples;

#define HashTableSpace 100

typedef struct SymbolTable{
    //used in checker to see types
    //used in gencode to link to registers of the target language
    int size;
    NameValueTuples *table[HashTableSpace];
} SymbolTable;


void ungetstring(char *s, FILE *source);
Token getNumericToken( FILE *source, char c );
Token scanner( FILE *source );
Declaration makeDeclarationNode( Token declare_type, Token identifier );
Declarations *makeDeclarationTree( Declaration decl, Declarations *decls );
Declaration parseDeclaration( FILE *source, Token token );
Declarations *parseDeclarations( FILE *source );
void constantFolding( Expression *expr );
Expression *parseValue( FILE *source );
Expression *parseFactoryTail( FILE *source, Expression *lvalue );
Expression *parseFactory( FILE *source );
Expression *parseExpressionTail( FILE *source, Expression *lvalue );
Expression *parseExpression( FILE *source, Expression *lvalue );
Statement makeAssignmentNode( VarName id, Expression *f, Expression *expr_tail );
Statement makePrintNode( VarName id );
Statements *makeStatementTree( Statement stmt, Statements *stmts );
Statement parseStatement( FILE *source, Token token );
Statements *parseStatements( FILE * source );
Program parser( FILE *source );
void InitializeTable( SymbolTable *table );
int hash_name( VarName n );
NameValueTuples *makeHashNode( VarName n );
void add_table_type( SymbolTable *table, VarName n, DataType t );
SymbolTable build( Program program );
void convertType( Expression * old, DataType type );
DataType generalize( Expression *left, Expression *right );
DataType lookup_table_type( SymbolTable *table, VarName n );
void checkexpression( Expression * expr, SymbolTable * table );
void checkstmt( Statement *stmt, SymbolTable * table );
void check( Program *program, SymbolTable * table);
VarName lookup_table_newname( SymbolTable *table, VarName n );
void fprint_op( FILE *target, ValueType op );
void fprint_expr( FILE *target, Expression *expr, SymbolTable* table );
void gencode( Program prog, FILE * target );

void print_expr( Expression *expr );
void test_parser( FILE *source );

#endif // HEADER_H_INCLUDED
