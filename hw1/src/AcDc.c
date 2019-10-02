#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "header.h"


int main( int argc, char *argv[] )
{
    FILE *source, *target;
    Program program;
    SymbolTable symtab;

    if( argc == 3){
        source = fopen(argv[1], "r");
        target = fopen(argv[2], "w");
        if( !source ){
            printf("can't open the source file\n");
            exit(2);
        }
        else if( !target ){
            printf("can't open the target file\n");
            exit(2);
        }
        else{
            program = parser(source);
            fclose(source);
            symtab = build(program);
            check(&program, &symtab);
            gencode(program, target);
        }
    }
    else{
        printf("Usage: %s source_file target_file\n", argv[0]);
    }


    return 0;
}

void ungetstring(char *s, FILE *source){
    int i = strlen(s) - 1;
    for(;i >= 0;i--){
        ungetc(s[i], source);
    }
}

/********************************************* 
  Scanning 
 *********************************************/
Token getNumericToken( FILE *source, char c )
{
    Token token;
    int i = 0;

    while( isdigit(c) ) {
        token.tok[i++] = c;
        c = fgetc(source);
    }

    if( c != '.' ){
        ungetc(c, source);
        token.tok[i] = '\0';
        token.type = IntValue;
        return token;
    }

    token.tok[i++] = '.';

    c = fgetc(source);
    if( !isdigit(c) ){
        ungetc(c, source);
        printf("Expect a digit : %c\n", c);
        exit(1);
    }

    while( isdigit(c) ){
        token.tok[i++] = c;
        c = fgetc(source);
    }

    ungetc(c, source);
    token.tok[i] = '\0';
    token.type = FloatValue;
    return token;
}

Token scanner( FILE *source )
{
    char c;
    Token token;

    while( !feof(source) ){
        c = fgetc(source);

        while( isspace(c) ) c = fgetc(source);

        if( isdigit(c) )
            return getNumericToken(source, c);

        token.tok[0] = c;
        // changed to receive extended var name
        if( isalpha(c) ){
            int i = 1;
            c = fgetc(source);
            while( isalnum(c) ){
                token.tok[i++] = c;
                c = fgetc(source);
            }
            if(i == 1){
                // check if 'fip'
                switch(token.tok[0]){
                	case 'f':
                    	token.type = FloatDeclaration;
                    	break;
                	case 'i':
                    	token.type = IntegerDeclaration;
                    	break;
                	case 'p':
                    	token.type = PrintOp;
                    	break;
                	default:
                    	token.type = Alphabet;
                }
            }else{
                token.type = Alphabet;
            }
            ungetc(c, source);
            token.tok[i] = '\0';
            return token;
        }
        token.tok[1] = '\0';

        switch(c){
            case '=':
                token.type = AssignmentOp;
                return token;
            case '+':
                token.type = PlusOp;
                return token;
            case '-':
                token.type = MinusOp;
                return token;
            case '*':
                token.type = MulOp;
                return token;
            case '/':
                token.type = DivOp;
                return token;
            case EOF:
                token.type = EOFsymbol;
                token.tok[0] = '\0';
                return token;
            default:
                printf("Invalid character : %c\n", c);
                exit(1);
        }
    }

    token.tok[0] = '\0';
    token.type = EOFsymbol;
    return token;
}


/********************************************************
  Parsing
 *********************************************************/
Declaration parseDeclaration( FILE *source, Token token )
{
    Token token2;
    switch(token.type){
        case FloatDeclaration:
        case IntegerDeclaration:
            token2 = scanner(source);
            if (strcmp(token2.tok, "f") == 0 ||
                    strcmp(token2.tok, "i") == 0 ||
                    strcmp(token2.tok, "p") == 0) {
                printf("Syntax Error: %s cannot be used as id\n", token2.tok);
                exit(1);
            }
            return makeDeclarationNode( token, token2 );
        default:
            printf("Syntax Error: Expect Declaration %s\n", token.tok);
            exit(1);
    }
}

Declarations *parseDeclarations( FILE *source )
{
    Token token = scanner(source);
    Declaration decl;
    Declarations *decls;
    switch(token.type){
        case FloatDeclaration:
        case IntegerDeclaration:
            decl = parseDeclaration(source, token);
            decls = parseDeclarations(source);
            return makeDeclarationTree( decl, decls );
        case PrintOp:
        case Alphabet:
            ungetstring(token.tok, source);
            return NULL;
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect declarations %s\n", token.tok);
            exit(1);
    }
}

void constantFolding( Expression *expr ){
	if(expr != NULL && expr->leftOperand != NULL && expr->rightOperand != NULL){
		Expression *l = expr->leftOperand, *r = expr->rightOperand;
		if( ((l->v).type == IntConst || (l->v).type == FloatConst) &&
			((r->v).type == IntConst || (r->v).type == FloatConst) ){
			int bothInt = ((l->v).type == IntConst && (r->v).type == IntConst);
			int i1 = ((l->v).type == IntConst ? (l->v).val.ivalue : 0);
			int i2 = ((r->v).type == IntConst ? (r->v).val.ivalue : 0);
			float f1 = ((l->v).type == FloatConst ? (l->v).val.fvalue : 0.0);
			float f2 = ((r->v).type == FloatConst ? (r->v).val.fvalue : 0.0);
			switch((expr->v).type){
				case PlusNode:
					if(bothInt){
						(expr->v).type = IntConst;
						(expr->v).val.ivalue = i1 + i2;
					}else{
						(expr->v).type = FloatConst;
						(expr->v).val.fvalue = (((l->v).type == IntConst) ? (float)i1 : f1)
											+ (((r->v).type == IntConst) ? (float)i2 : f2);
					}
					break;
				case MinusNode:
					if(bothInt){
						(expr->v).type = IntConst;
						(expr->v).val.ivalue = i1 - i2;
					}else{
						(expr->v).type = FloatConst;
						(expr->v).val.fvalue = (((l->v).type == IntConst) ? (float)i1 : f1)
											- (((r->v).type == IntConst) ? (float)i2 : f2);
					}
					break;
				case MulNode:
					if(bothInt){
						(expr->v).type = IntConst;
						(expr->v).val.ivalue = i1 * i2;
					}else{
						(expr->v).type = FloatConst;
						(expr->v).val.fvalue = (((l->v).type == IntConst) ? (float)i1 : f1)
											* (((r->v).type == IntConst) ? (float)i2 : f2);
					}
					break;
				case DivNode:
					if(bothInt){
						if(i2 == 0){
							//div 0, I don't optimize it
							return;
						}
						(expr->v).type = IntConst;
						(expr->v).val.ivalue = i1 / i2;
					}else{
						if(((r->v).type == IntConst && i2 == 0) || 
							((r->v).type == FloatConst && f2 == 0.0)){
							//div 0, I don't optimize it
							return;
						}
						(expr->v).type = FloatConst;
						(expr->v).val.fvalue = (((l->v).type == IntConst) ? (float)i1 : f1)
											/ (((r->v).type == IntConst) ? (float)i2 : f2);
					}
					break;
				default:
					return;
			}
			free(l);
			free(r);
			expr->leftOperand = NULL;
			expr->rightOperand = NULL;
		}
	}
}

Expression *parseValue( FILE *source )
{
    Token token = scanner(source);
    Expression *value = (Expression *)malloc( sizeof(Expression) );
    value->leftOperand = value->rightOperand = NULL;

    switch(token.type){
        case Alphabet:
            (value->v).type = Identifier;
            (value->v).val.id = (VarName)malloc( sizeof(char) * 64 );
            strcpy((value->v).val.id, token.tok);
            break;
        case IntValue:
            (value->v).type = IntConst;
            (value->v).val.ivalue = atoi(token.tok);
            break;
        case FloatValue:
            (value->v).type = FloatConst;
            (value->v).val.fvalue = atof(token.tok);
            break;
        default:
            printf("Syntax Error: Expect Identifier or a Number %s\n", token.tok);
            exit(1);
    }

    return value;
}

Expression *parseFactoryTail( FILE *source, Expression *lvalue ){
	Token token = scanner(source);
    Expression *expr;

    switch(token.type){
        case MulOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MulNode;
            (expr->v).val.op = Mul;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            constantFolding(expr);
            return parseExpressionTail(source, expr);
        case DivOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = DivNode;
            (expr->v).val.op = Div;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            constantFolding(expr);
            return parseExpressionTail(source, expr);
        case Alphabet:
            ungetstring(token.tok, source);
            return lvalue;
        case PlusOp:
        case MinusOp:
        case PrintOp:
            ungetc(token.tok[0], source);
            return lvalue;
        case EOFsymbol:
            return lvalue;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Expression *parseFactory( FILE *source ){
	Expression *lvalue = parseValue(source);
	Token token = scanner(source);
    Expression *expr;

    switch(token.type){
        case MulOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MulNode;
            (expr->v).val.op = Mul;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            constantFolding(expr);
            return parseFactoryTail(source, expr);
        case DivOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = DivNode;
            (expr->v).val.op = Div;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            constantFolding(expr);
            return parseFactoryTail(source, expr);
        case Alphabet:
            ungetstring(token.tok, source);
            return lvalue;
        case PlusOp:
        case MinusOp:
        case PrintOp:
            ungetc(token.tok[0], source);
            return lvalue;
        case EOFsymbol:
            return lvalue;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Expression *parseExpressionTail( FILE *source, Expression *lvalue )
{
    Token token = scanner(source);
    Expression *expr;

    switch(token.type){
        case PlusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = PlusNode;
            (expr->v).val.op = Plus;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseFactory(source);
            constantFolding(expr);
            return parseExpressionTail(source, expr);
        case MinusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MinusNode;
            (expr->v).val.op = Minus;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseFactory(source);
            constantFolding(expr);
            return parseExpressionTail(source, expr);
        case Alphabet:
            ungetstring(token.tok, source);
            return lvalue;
        case PrintOp:
            ungetc(token.tok[0], source);
            return lvalue;
        case EOFsymbol:
            return lvalue;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Expression *parseExpression( FILE *source, Expression *lvalue )
{
    Token token = scanner(source);
    Expression *expr;

    switch(token.type){
        case PlusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = PlusNode;
            (expr->v).val.op = Plus;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseFactory(source);
            constantFolding(expr);
            return parseExpressionTail(source, expr);
        case MinusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MinusNode;
            (expr->v).val.op = Minus;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseFactory(source);
            constantFolding(expr);
            return parseExpressionTail(source, expr);
        case Alphabet:
            ungetstring(token.tok, source);
            return NULL;
        case PrintOp:
            ungetc(token.tok[0], source);
            return NULL;
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Statement parseStatement( FILE *source, Token token )
{
    Token next_token;
    Expression *factory, *expr;

    switch(token.type){
        case Alphabet:
            next_token = scanner(source);
            if(next_token.type == AssignmentOp){
                //value = parseValue(source);
                factory = parseFactory(source);
                expr = parseExpression(source, factory);
                return makeAssignmentNode(token.tok, factory, expr);
            }
            else{
                printf("Syntax Error: Expect an assignment op %s\n", next_token.tok);
                exit(1);
            }
        case PrintOp:
            next_token = scanner(source);
            if(next_token.type == Alphabet)
                return makePrintNode(next_token.tok);
            else{
                printf("Syntax Error: Expect an identifier %s\n", next_token.tok);
                exit(1);
            }
            break;
        default:
            printf("Syntax Error: Expect a statement %s\n", token.tok);
            exit(1);
    }
}

Statements *parseStatements( FILE * source )
{

    Token token = scanner(source);
    Statement stmt;
    Statements *stmts;

    switch(token.type){
        case Alphabet:
        case PrintOp:
            stmt = parseStatement(source, token);
            stmts = parseStatements(source);
            return makeStatementTree(stmt , stmts);
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect statements %s\n", token.tok);
            exit(1);
    }
}


/*********************************************************************
  Build AST
 **********************************************************************/
Declaration makeDeclarationNode( Token declare_type, Token identifier )
{
    Declaration tree_node;

    switch(declare_type.type){
        case FloatDeclaration:
            tree_node.type = Float;
            break;
        case IntegerDeclaration:
            tree_node.type = Int;
            break;
        default:
            break;
    }
    tree_node.name = (VarName)malloc( sizeof(char) * 64 );
    strcpy(tree_node.name, identifier.tok);

    return tree_node;
}

Declarations *makeDeclarationTree( Declaration decl, Declarations *decls )
{
    Declarations *new_tree = (Declarations *)malloc( sizeof(Declarations) );
    new_tree->first = decl;
    new_tree->rest = decls;

    return new_tree;
}


Statement makeAssignmentNode( VarName id, Expression *f, Expression *expr_tail )
{
    Statement stmt;
    AssignmentStatement assign;

    stmt.type = Assignment;
    assign.id = (VarName)malloc( sizeof(char) * 64 );
    strcpy(assign.id, id);
    if(expr_tail == NULL)
        assign.expr = f;
    else
        assign.expr = expr_tail;
    stmt.stmt.assign = assign;

    return stmt;
}

Statement makePrintNode( VarName id )
{
    Statement stmt;
    stmt.type = Print;
    stmt.stmt.variable = (VarName)malloc( sizeof(char) * 64 );
    strcpy(stmt.stmt.variable, id);

    return stmt;
}

Statements *makeStatementTree( Statement stmt, Statements *stmts )
{
    Statements *new_tree = (Statements *)malloc( sizeof(Statements) );
    new_tree->first = stmt;
    new_tree->rest = stmts;

    return new_tree;
}

/* parser */
Program parser( FILE *source )
{
    Program program;

    program.declarations = parseDeclarations(source);
    program.statements = parseStatements(source);

    return program;
}


/********************************************************
  Build symbol table
 *********************************************************/
void InitializeTable( SymbolTable *table )
{
    int i;

    for(i = 0 ; i < HashTableSpace; i++)
        table->table[i] = NULL;

    table->size = 0;
}

int hash_name( VarName n ){
    int i = 0;
    int value = 0;
    while(n[i] != '\0'){
        value += (int)n[i];
        i++;
    }
    return value % HashTableSpace;
}

NameValueTuples *makeHashNode( VarName n ){
    NameValueTuples *tuple = (NameValueTuples *)malloc( sizeof(NameValueTuples) );
    tuple->tuple.name = (VarName)malloc( sizeof(char) * 64 );
    strcpy(tuple->tuple.name, n);
    tuple->tuples = NULL;
    return tuple;
}

void add_table_type( SymbolTable *table, VarName n, DataType t )
{
    int index = hash_name(n);
    NameValueTuples *tuple = table->table[index];

    if(tuple == NULL){
        tuple = makeHashNode(n);
        tuple->tuple.value.type = t;
        table->table[index] = tuple;
        table->size++;
    }else{
        while(1){
            if(strcmp(tuple->tuple.name, n) == 0){
                printf("Error : id %s has been declared\n", n);//error
                return;
            }
            if(tuple->tuples == NULL){
                tuple = makeHashNode(n);
                tuple->tuple.value.type = t;
                tuple->tuples = tuple;
                table->size++;
                return;
            }
            tuple = tuple->tuples;
        }
    }
}

SymbolTable build( Program program )
{
    SymbolTable table;
    Declarations *decls = program.declarations;
    Declaration current;

    InitializeTable(&table);

    while(decls !=NULL){
        current = decls->first;
        add_table_type(&table, current.name, current.type);
        decls = decls->rest;
    }

    return table;
}


/********************************************************************
  Type checking
 *********************************************************************/

void convertType( Expression * old, DataType type )
{
    if(old->type == Float && type == Int){
        printf("error : can't convert float to integer\n");
        return;
    }
    if(old->type == Int && type == Float){
        Expression *tmp = (Expression *)malloc( sizeof(Expression) );
        if(old->v.type == Identifier)
            printf("convert to float %s \n",old->v.val.id);
        else
            printf("convert to float %d \n", old->v.val.ivalue);
        tmp->v = old->v;
        tmp->leftOperand = old->leftOperand;
        tmp->rightOperand = old->rightOperand;
        tmp->type = old->type;

        Value v;
        v.type = IntToFloatConvertNode;
        v.val.op = IntToFloatConvert;
        old->v = v;
        old->type = Int;
        old->leftOperand = tmp;
        old->rightOperand = NULL;
    }
}

DataType generalize( Expression *left, Expression *right )
{
    if(left->type == Float || right->type == Float){
        printf("generalize : float\n");
        return Float;
    }
    printf("generalize : int\n");
    return Int;
}

DataType lookup_table_type( SymbolTable *table, VarName n )
{
    int index = hash_name(n);
    NameValueTuples *tuple = table->table[index];
    while(tuple != NULL){
        if(strcmp(tuple->tuple.name, n) == 0){
            return tuple->tuple.value.type;
        }
        tuple = tuple->tuples;
    }
    printf("Error : identifier %s is not declared\n", n);//error
    return Notype;
}

void checkexpression( Expression * expr, SymbolTable * table )
{
    VarName n;
    if(expr->leftOperand == NULL && expr->rightOperand == NULL){
        switch(expr->v.type){
            case Identifier:
                n = expr->v.val.id;
                printf("identifier : %s\n",n);
                expr->type = lookup_table_type(table, n);
                break;
            case IntConst:
                printf("constant : int\n");
                expr->type = Int;
                break;
            case FloatConst:
                printf("constant : float\n");
                expr->type = Float;
                break;
                //case PlusNode: case MinusNode: case MulNode: case DivNode:
            default:
                break;
        }
    }
    else{
        Expression *left = expr->leftOperand;
        Expression *right = expr->rightOperand;

        checkexpression(left, table);
        checkexpression(right, table);

        DataType type = generalize(left, right);
        convertType(left, type);//left->type = type;//converto
        convertType(right, type);//right->type = type;//converto
        expr->type = type;
    }
}

void checkstmt( Statement *stmt, SymbolTable * table )
{
    if(stmt->type == Assignment){
        AssignmentStatement assign = stmt->stmt.assign;
        printf("assignment : %s \n",assign.id);
        checkexpression(assign.expr, table);
        stmt->stmt.assign.type = lookup_table_type(table, assign.id);
        if (assign.expr->type == Float && stmt->stmt.assign.type == Int) {
            printf("error : can't convert float to integer\n");
        } else {
            convertType(assign.expr, stmt->stmt.assign.type);
        }
    }
    else if (stmt->type == Print){
        printf("print : %s \n",stmt->stmt.variable);
        lookup_table_type(table, stmt->stmt.variable);
    }
    else printf("error : statement error\n");//error
}

void check( Program *program, SymbolTable * table )
{
    Statements *stmts = program->statements;
    while(stmts != NULL){
        checkstmt(&stmts->first,table);
        stmts = stmts->rest;
    }
}


/***********************************************************************
  Code generation
 ************************************************************************/
VarName lookup_table_newname( SymbolTable *table, VarName n ){
    //link identifiers to DC registers
    int index = hash_name(n);
    NameValueTuples *tuple = table->table[index];

    if(tuple == NULL){
        tuple = makeHashNode(n);
        tuple->tuple.value.newname = (VarName)malloc( sizeof(char) * 64 );
        //naming style for DC
        tuple->tuple.value.newname[0] = 'a' + table->size;
        tuple->tuple.value.newname[1] = '\0';
        table->table[index] = tuple;
        table->size++;
    }else{
        while(1){
            if(strcmp(tuple->tuple.name, n) == 0){
                break;
            }
            if(tuple->tuples == NULL){
                tuple = makeHashNode(n);
                tuple->tuple.value.newname = (VarName)malloc( sizeof(char) * 64 );
                tuple->tuple.value.newname[0] = 'a' + table->size;
                tuple->tuple.value.newname[1] = '\0';
                tuple->tuples = tuple;
                table->size++;
                break;
            }
            tuple = tuple->tuples;
        }
    }
    return tuple->tuple.value.newname;
}

void fprint_op( FILE *target, ValueType op )
{
    switch(op){
        case MinusNode:
            fprintf(target,"-\n");
            break;
        case PlusNode:
            fprintf(target,"+\n");
            break;
        case MulNode:
            fprintf(target,"*\n");
            break;
        case DivNode:
            fprintf(target,"/\n");
            break;
        default:
            fprintf(target,"Error in fprintf_op ValueType = %d\n",op);
            break;
    }
}

void fprint_expr( FILE *target, Expression *expr, SymbolTable *table )
{

    if(expr->leftOperand == NULL){
        switch( (expr->v).type ){
            case Identifier:
                fprintf(target,"l%s\n",lookup_table_newname(table, (expr->v).val.id));
                break;
            case IntConst:
                fprintf(target,"%d\n",(expr->v).val.ivalue);
                break;
            case FloatConst:
                fprintf(target,"%f\n", (expr->v).val.fvalue);
                break;
            default:
                fprintf(target,"Error In fprint_left_expr. (expr->v).type=%d\n",(expr->v).type);
                break;
        }
    }
    else{
        fprint_expr(target, expr->leftOperand, table);
        if(expr->rightOperand == NULL){
            fprintf(target,"5k\n");
        }
        else{
            //  fprint_right_expr(expr->rightOperand);
            fprint_expr(target, expr->rightOperand, table);
            fprint_op(target, (expr->v).type);
        }
    }
}

void gencode(Program prog, FILE * target)
{
    Statements *stmts = prog.statements;
    Statement stmt;

    SymbolTable table;
    InitializeTable(&table);

    while(stmts != NULL){
        stmt = stmts->first;
        switch(stmt.type){
            case Print:
                fprintf(target,"l%s\n",lookup_table_newname(&table, stmt.stmt.variable));
                fprintf(target,"p\n");
                break;
            case Assignment:
                fprint_expr(target, stmt.stmt.assign.expr, &table);
                /*
                   if(stmt.stmt.assign.type == Int){
                   fprintf(target,"0 k\n");
                   }
                   else if(stmt.stmt.assign.type == Float){
                   fprintf(target,"5 k\n");
                   }*/
                fprintf(target,"s%s\n",lookup_table_newname(&table, stmt.stmt.assign.id));
                fprintf(target,"0 k\n");
                break;
        }
        stmts=stmts->rest;
    }

}


/***************************************
  For our debug,
  you can omit them.
 ****************************************/
void print_expr(Expression *expr)
{
    if(expr == NULL)
        return;
    else{
        print_expr(expr->leftOperand);
        switch((expr->v).type){
            case Identifier:
                printf("%s ", (expr->v).val.id);
                break;
            case IntConst:
                printf("%d ", (expr->v).val.ivalue);
                break;
            case FloatConst:
                printf("%f ", (expr->v).val.fvalue);
                break;
            case PlusNode:
                printf("+ ");
                break;
            case MinusNode:
                printf("- ");
                break;
            case MulNode:
                printf("* ");
                break;
            case DivNode:
                printf("/ ");
                break;
            case IntToFloatConvertNode:
                printf("(float) ");
                break;
            default:
                printf("error ");
                break;
        }
        print_expr(expr->rightOperand);
    }
}

void test_parser( FILE *source )
{
    Declarations *decls;
    Statements *stmts;
    Declaration decl;
    Statement stmt;
    Program program = parser(source);

    decls = program.declarations;

    while(decls != NULL){
        decl = decls->first;
        if(decl.type == Int)
            printf("i ");
        if(decl.type == Float)
            printf("f ");
        printf("%s ",decl.name);
        decls = decls->rest;
    }

    stmts = program.statements;

    while(stmts != NULL){
        stmt = stmts->first;
        if(stmt.type == Print){
            printf("p %s ", stmt.stmt.variable);
        }

        if(stmt.type == Assignment){
            printf("%s = ", stmt.stmt.assign.id);
            print_expr(stmt.stmt.assign.expr);
        }
        stmts = stmts->rest;
    }

}
