#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
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
            content_folding(program);
            gencode(program, target, &symtab);
        }
    }
    else{
        printf("Usage: %s source_file target_file\n", argv[0]);
    }


    return 0;
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

Token getAlphabetToken( FILE *source, char c )
{
	Token token;
    int i = 0;

    while( islower(c) ) {
        token.tok[i++] = c;
        c = fgetc(source);
    }

    ungetc(c, source);
    token.tok[i] = '\0';
    token.type = Alphabet;
    return token;
}

Token scanner( FILE *source )
{
    char c;
    char c_next;
    Token token;

    while( !feof(source) ){
        c = fgetc(source);

        while( isspace(c) ) c = fgetc(source);
        
        c_next = fgetc(source);
        ungetc(c_next, source);

        if( isdigit(c) )
            return getNumericToken(source, c);

        if( islower(c) ){
            if( c == 'f' && isspace(c_next) )
            {
            	token.tok[0] = c;
        		token.tok[1] = '\0';
                token.type = FloatDeclaration;
            }
            else if( c == 'i' && isspace(c_next) )
            {
            	token.tok[0] = c;
        		token.tok[1] = '\0';
                token.type = IntegerDeclaration;
            }
            else if( c == 'p' && isspace(c_next) )
            {
            	token.tok[0] = c;
        		token.tok[1] = '\0';
                token.type = PrintOp;
            }
            else
            	return getAlphabetToken(source, c);
            return token;
        }

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
    	int i;
        case FloatDeclaration:
        case IntegerDeclaration:
            decl = parseDeclaration(source, token);
            decls = parseDeclarations(source);
            return makeDeclarationTree( decl, decls );
        case PrintOp:
        	ungetc(token.tok[0], source);
            return NULL;
        case Alphabet:
        	for (i = strlen(token.tok) - 1; i >= 0; i--)
            	ungetc(token.tok[i], source);
            return NULL;
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect declarations %s\n", token.tok);
            exit(1);
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
            strncpy ( (value->v).val.id, token.tok, sizeof((value->v).val.id) );
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

Expression *parseTermTail( FILE *source, Expression *lvalue )
{
    Token token = scanner(source);
    Expression *expr;
	
    switch(token.type){
    	int i;
    	case PlusOp:
    		ungetc('+', source);
            return lvalue;
        case MinusOp:
            ungetc('-', source);
            return lvalue;
        case MulOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MulNode;
            (expr->v).val.op = Mul;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseTermTail(source, expr);
        case DivOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = DivNode;
            (expr->v).val.op = Div;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseTermTail(source, expr);
        case Alphabet:
        	for (i = strlen(token.tok) - 1; i >= 0; i--)
            	ungetc(token.tok[i], source);
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

Expression *parseTerm( FILE *source, Expression *lvalue )
{
    Token token = scanner(source);
    Expression *expr;
    switch(token.type){
    	int i;
    	case PlusOp:
    		ungetc('+', source);
            return lvalue;
    	case MinusOp:
    		ungetc('-', source);
    		return lvalue;
        case MulOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MulNode;
            (expr->v).val.op = Mul;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseTermTail(source, expr);
        case DivOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = DivNode;
            (expr->v).val.op = Div;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseTermTail(source, expr);
        case Alphabet:
        	for (i = strlen(token.tok) - 1; i >= 0; i--)
            	ungetc(token.tok[i], source);
            return lvalue;
        case EOFsymbol:
            return lvalue;
        case PrintOp:
            ungetc(token.tok[0], source);
            return lvalue;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Expression *parseExpressionTail( FILE *source, Expression *lterm )
{
    Token token = scanner(source);
    Expression *expr;
	
    switch(token.type){
    	int i;
        case PlusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = PlusNode;
            (expr->v).val.op = Plus;
            expr->leftOperand = lterm;
            expr->rightOperand = parseTerm(source, parseValue(source));
            return parseExpressionTail(source, expr);
        case MinusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MinusNode;
            (expr->v).val.op = Minus;
            expr->leftOperand = lterm;
            expr->rightOperand = parseTerm(source, parseValue(source));
            return parseExpressionTail(source, expr);
        case Alphabet:
        	for (i = strlen(token.tok) - 1; i >= 0; i--)
            	ungetc(token.tok[i], source);
            return lterm;
        case PrintOp:
            ungetc(token.tok[0], source);
            return lterm;
        case EOFsymbol:
            return lterm;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Expression *parseExpression( FILE *source, Expression *lterm )
{
    Token token = scanner(source);
    Expression *expr;

    switch(token.type){
    	int i;
        case PlusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = PlusNode;
            (expr->v).val.op = Plus;
            expr->leftOperand = lterm;
            expr->rightOperand = parseTerm(source, parseValue(source));
            return parseExpressionTail(source, expr);
        case MinusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MinusNode;
            (expr->v).val.op = Minus;
            expr->leftOperand = lterm;
            expr->rightOperand = parseTerm(source, parseValue(source));
            return parseExpressionTail(source, expr);
        case Alphabet:
        	for (i = strlen(token.tok) - 1; i >= 0; i--)
            	ungetc(token.tok[i], source);
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
    Expression *value, *term, *expr;

    switch(token.type){
        case Alphabet:
            next_token = scanner(source);
            if(next_token.type == AssignmentOp){
                value = parseValue(source);
                term = parseTerm(source, value);
                expr = parseExpression(source, term);
                return makeAssignmentNode(token.tok, value, term, expr);
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
    strncpy(tree_node.name, identifier.tok, sizeof(tree_node.name));

    return tree_node;
}

Declarations *makeDeclarationTree( Declaration decl, Declarations *decls )
{
    Declarations *new_tree = (Declarations *)malloc( sizeof(Declarations) );
    new_tree->first = decl;
    new_tree->rest = decls;

    return new_tree;
}


Statement makeAssignmentNode( char *id, Expression *v, Expression *term_tail, Expression *expr_tail )
{
    Statement stmt;
    AssignmentStatement assign;

    stmt.type = Assignment;
    strncpy ( assign.id, id, sizeof(assign.id) );
    if(term_tail == NULL)
        assign.expr = v;
    else if(expr_tail == NULL)
    	assign.expr = term_tail;
    else
        assign.expr = expr_tail;
    stmt.stmt.assign = assign;

    return stmt;
}

Statement makePrintNode( char *id )
{
    Statement stmt;
    stmt.type = Print;
    strncpy( stmt.stmt.variable, id, sizeof( stmt.stmt.variable ) );

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
  Hashing function
 *********************************************************/
int hash(char *str)
{
    int hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	
	hash = hash % TABLE_SIZE;
    return (hash >= 0) ? hash : (hash % TABLE_SIZE) * (-1);
}

/********************************************************
  Build symbol table
 *********************************************************/
void InitializeTable( SymbolTable *table )
{
    int i;

    for(i = 0 ; i < TABLE_SIZE; i++)
    {
    	table->table[i].type = Notype;
    	table->table[i].original_name[0] = '\0';
	}
        
}

void add_table( SymbolTable *table, char *c, DataType t )
{
    int index = hash(c);
    
    int i;
    for(i = 0; i < TABLE_SIZE; i++)
    {
    	if(strcmp(table->table[index%TABLE_SIZE].original_name, c) == 0)
    	{
    		printf("Error : id %s has been declared\n", c);//error
    		break;
    	}
    	if(strlen(table->table[index%TABLE_SIZE].original_name) == 0)
    	{
    		strncpy(table->table[index%TABLE_SIZE].original_name, c, sizeof(table->table[index%TABLE_SIZE].original_name));
    		table->table[index%TABLE_SIZE].type = t;
    		break;
		}
		index++;
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
        add_table(&table, current.name, current.type);
        decls = decls->rest;
    }
    
    //printTable(&table);

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

DataType lookup_table( SymbolTable *table, char *c )
{
    int id = hash(c);
    int i;
    
    for(i = 0; i < TABLE_SIZE; i++)
    {
    	if(strcmp(table->table[id%TABLE_SIZE].original_name, c) == 0)
        	return table->table[id%TABLE_SIZE].type;
		if(strlen(table->table[id%TABLE_SIZE].original_name) == 0)
			break;
		id++;
	}
	printf("Error : identifier %s is not declared\n", c);//error
	return -1;
}

int lookup_index( SymbolTable *table, char *c )
{
    int id = hash(c);
    int i;
    
    for(i = 0; i < TABLE_SIZE; i++)
    {
    	if(strcmp(table->table[id%TABLE_SIZE].original_name, c) == 0)
        	return id%TABLE_SIZE;
        if(strlen(table->table[id%TABLE_SIZE].original_name) == 0)
        	break;
		id++;
	}
	printf("Error : identifier %s is not declared\n", c);//error
	return -1;
}

void checkexpression( Expression * expr, SymbolTable * table )
{
    char c[64];
    if(expr->leftOperand == NULL && expr->rightOperand == NULL){
        switch(expr->v.type){
            case Identifier:
            	strncpy( c, expr->v.val.id, sizeof(c) );
                printf("identifier : %s\n",c);
                expr->type = lookup_table(table, c);
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
        stmt->stmt.assign.type = lookup_table(table, assign.id);
        if (assign.expr->type == Float && stmt->stmt.assign.type == Int) {
            printf("error : can't convert float to integer\n");
        } else {
            convertType(assign.expr, stmt->stmt.assign.type);
        }
    }
    else if (stmt->type == Print){
        printf("print : %s \n",stmt->stmt.variable);
        //lookup_table(table, stmt->stmt.variable);
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
  Content folding
 ************************************************************************/
void content_folding( Program prog )
{
	Statements *stmts = prog.statements;
    Statement stmt;
    
    while(stmts != NULL){
        stmt = stmts->first;
        if(stmt.type == Assignment)
        	fold_expr(stmt.stmt.assign.expr);
        stmts=stmts->rest;
    }
}

void fold_expr( Expression *expr )
{
	if(expr->leftOperand != NULL){
		Expression *left = expr->leftOperand;
		if(expr->v.type == IntToFloatConvertNode)
		{
			fold_expr(left);
			expr->v.type = FloatConst;
			expr->v.val.fvalue = (float)left->v.val.ivalue;
			expr->leftOperand = NULL;
		}
			
			
		else
		{
			if(expr->rightOperand != NULL)
			{
        		Expression *right = expr->rightOperand;
		
        		fold_expr(left);
        		fold_expr(right);
        		
        		switch(expr->v.type){
        			case PlusNode:
        				if(left->v.type == IntConst && right->v.type == IntConst)
        				{
        					expr->v.type = IntConst;
        					expr->v.val.ivalue = left->v.val.ivalue + right->v.val.ivalue;
        					expr->leftOperand = NULL;
    						expr->rightOperand = NULL;
						}
						else if(left->v.type == FloatConst && right->v.type == FloatConst)
						{
							expr->v.type = FloatConst;
        					expr->v.val.fvalue = left->v.val.fvalue + right->v.val.fvalue;
        					expr->leftOperand = NULL;
    						expr->rightOperand = NULL;
						}
						else
						{
						}
						break;
					case MinusNode:
        				if(left->v.type == IntConst && right->v.type == IntConst)
        				{
        					expr->v.type = IntConst;
        					expr->v.val.ivalue = left->v.val.ivalue - right->v.val.ivalue;
        					expr->leftOperand = NULL;
    						expr->rightOperand = NULL;
						}
						else if(left->v.type == FloatConst && right->v.type == FloatConst)
						{
							expr->v.type = FloatConst;
        					expr->v.val.fvalue = left->v.val.fvalue - right->v.val.fvalue;
        					expr->leftOperand = NULL;
    						expr->rightOperand = NULL;
						}
						else
						{
						}
						break;
					case MulNode:
        				if(left->v.type == IntConst && right->v.type == IntConst)
        				{
        					expr->v.type = IntConst;
        					expr->v.val.ivalue = left->v.val.ivalue * right->v.val.ivalue;
        					expr->leftOperand = NULL;
    						expr->rightOperand = NULL;
						}
						else if(left->v.type == FloatConst && right->v.type == FloatConst)
						{
							expr->v.type = FloatConst;
        					expr->v.val.fvalue = left->v.val.fvalue * right->v.val.fvalue;
        					expr->leftOperand = NULL;
    						expr->rightOperand = NULL;
						}
						else
						{
						}
						break;
					case DivNode:
        				if(left->v.type == IntConst && right->v.type == IntConst)
        				{
        					expr->v.type = IntConst;
        					expr->v.val.ivalue = left->v.val.ivalue / right->v.val.ivalue;
        					expr->leftOperand = NULL;
    						expr->rightOperand = NULL;
						}
						else if(left->v.type == FloatConst && right->v.type == FloatConst)
						{
							expr->v.type = FloatConst;
        					expr->v.val.fvalue = left->v.val.fvalue / right->v.val.fvalue;
        					expr->leftOperand = NULL;
    						expr->rightOperand = NULL;
						}
						else
						{
						}
						break;
    			}
			}
		}
	}
}


/***********************************************************************
  Code generation
 ************************************************************************/
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

void fprint_expr( FILE *target, Expression *expr, SymbolTable * table )
{

    if(expr->leftOperand == NULL){
        switch( (expr->v).type ){
            case Identifier:
            	if (lookup_index(table, (expr->v).val.id) != -1)
                	fprintf(target,"l%c\n", 'a' + lookup_index(table, (expr->v).val.id));
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
            //	fprint_right_expr(expr->rightOperand);
            fprint_expr(target, expr->rightOperand, table);
            fprint_op(target, (expr->v).type);
        }
    }
}

void gencode(Program prog, FILE * target, SymbolTable * table )
{
    Statements *stmts = prog.statements;
    Statement stmt;

    while(stmts != NULL){
        stmt = stmts->first;
        switch(stmt.type){
            case Print:
            	if (lookup_index(table, stmt.stmt.variable) != -1)
            	{
                	fprintf(target,"l%c\n", 'a' + lookup_index(table, stmt.stmt.variable));
            		fprintf(target,"p\n");
            	}
                break;
            case Assignment:
                fprint_expr(target, stmt.stmt.assign.expr, table);
                /*
                   if(stmt.stmt.assign.type == Int){
                   fprintf(target,"0 k\n");
                   }
                   else if(stmt.stmt.assign.type == Float){
                   fprintf(target,"5 k\n");
                   }*/
                if (lookup_index(table, stmt.stmt.assign.id) != -1)
                {
                	fprintf(target,"s%c\n", 'a' + lookup_index(table, stmt.stmt.assign.id));
                	fprintf(target,"0 k\n");
                }
                break;
        }
        stmts=stmts->rest;
    }

}

/***************************************
  For my debug
 ****************************************/

void printTable(SymbolTable * table)
{
	int i;
	printf("\n--------------------------\n");
	printf("\n-------SymbolTable--------\n");
	for(i = 0; i < TABLE_SIZE; i++)
	{
		printf("%s, %d\n", table->table[i].original_name, table->table[i].type);
	}
	printf("--------------------------\n");
}


/***************************************
  For our debug,
  you can omit them.
 ****************************************/
/*
void print_expr(Expression *expr)
{
    if(expr == NULL)
        return;
    else{
        print_expr(expr->leftOperand);
        switch((expr->v).type){
            case Identifier:
                printf("%c ", (expr->v).val.id);
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
        printf("%c ",decl.name);
        decls = decls->rest;
    }

    stmts = program.statements;

    while(stmts != NULL){
        stmt = stmts->first;
        if(stmt.type == Print){
            printf("p %c ", stmt.stmt.variable);
        }

        if(stmt.type == Assignment){
            printf("%c = ", stmt.stmt.assign.id);
            print_expr(stmt.stmt.assign.expr);
        }
        stmts = stmts->rest;
    }

}
*/
