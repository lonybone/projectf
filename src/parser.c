#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "lexer.h"
#include "utils.h"

DynamicArray* statements;
HashTable* identifiers;

Statement* parseStatement();
Expression* parseExpression(int minPrecedence);
Expression* parsePrimaryExpression();
BinOperation* parseBinOperation(BinOperationType type, Expression* left, Expression* right);
UnaryOperation* parseUnaryOperation(TokenType type, Expression* right);
BlockStmt* parseBlockStmt();
WhileStmt* parseWhileStmt();
IfStmt* parseIfStmt();
Assignment* parseAssignment(Variable* variable, Expression* expression);
Declaration* parseDeclaration();
Value* parseValue(Token* token);
Variable* parseVariable(Token* token);

void freeStatement(Statement* statement);
void freeBlockStmt(BlockStmt* block);
void freeExpression(Expression* expression);
void freeBinOperation(BinOperation* binOperation);
void freeUnaryOperation(UnaryOperation* unaryOperation);
void freeBlockStmt(BlockStmt* blockStmt);
void freeWhileStmt(WhileStmt* whileStmt);
void freeIfStmt(IfStmt* ifStmt);
void freeAssignment(Assignment* assignment);
void freeDeclaration(Declaration* declaration);
void freeVariable(Variable* variable);
void freeValue(Value* value);

int initializeParser() {
	// array for holding all statements one level beneath root
	statements = dynamicArray(2);
	identifiers = hashTable(256);

	if (statements == NULL || identifiers == NULL) {
		freeArray(statements);
		freeTable(identifiers);
		return 1;
	}

	return 0;
}

DynamicArray* parseBuffer() {

	for (;;) {
		Statement* statement = parseStatement();

		if (statement == NULL) {
			return NULL;
		}

		if (statement->type == E_O_F_STMT) {
			freeStatement(statement);
			break;
		}

		appendStmt(statements, statement);
	}

	return statements;
}

static int getBinOpPrecedence(TokenType type) {
	switch (type) {
        	case ASSIGN:
            		return 1;
        	case EQUALS:
		case NEQUALS:
		case ST:
		case STE:
		case GT:
		case GTE:
			return 2;
        	case PLUS:
        	case MINUS:
            		return 3;
        	case MUL:
        	case DIV:
		case MODULUS:
            		return 4;
        default:
            return 0;
    }
}

char* parseToken(Token* token) {

	char* tokenString = malloc(token->length+1);

	if (tokenString == NULL) {
		fprintf(stderr, "failed to allocate memory for tokenstring\n");
		return NULL;
	}

	memcpy(tokenString, token->start, token->length);

	tokenString[token->length] = '\0';
	return tokenString;
}

Statement* parseStatement() {

	Token* token = peekToken();

	if (token == NULL) {
		fprintf(stderr, "Error: received no token in parser\n");
		return NULL;
	}

	Statement* statement = calloc(1, sizeof(Statement));
	if (statement == NULL) {
		fprintf(stderr, "Error: failed to allocate statement in parser\n");
		return NULL;
	}

	if (token->type == E_O_F) {
		statement->type = E_O_F_STMT;
		return statement;
	}

	switch (token->type) {

		case IF:
			advanceToken();
			statement->type = IF_STMT;
			statement->as.ifStmt = parseIfStmt();

			if (statement->as.ifStmt == NULL) {
				goto error;
			}

			break;

		case WHILE:
			advanceToken();
			statement->type = WHILE_STMT;
			statement->as.whileStmt = parseWhileStmt();

			if (statement->as.whileStmt == NULL) {
				goto error;
			}

			break;
			
		case LCURL:
			advanceToken();
			statement->type = BLOCK_STMT;
			statement->as.blockStmt = parseBlockStmt();

			if (statement->as.blockStmt == NULL) {
				goto error;
			}
			
			break;

		case ID:
		case NUM:
		case FNUM:
		case LPAREN:
			statement->type = EXPRESSION_STMT;
			Expression* expression = parseExpression(0);
			statement->as.expression = expression;

			if (expression == NULL) {
				goto error;
			}
			
			break;

		default: {
			fprintf(stderr,"Error: unexpected token in Statement of type %d\n", token->type);
			goto error;
		}

	}

	return statement;

error:
	fprintf(stderr, "Error: failed to parse Statement\n");
	freeStatement(statement);
	return NULL;

}

Expression* parsePrimaryExpression() {
	Token* token = peekToken();

	if (token == NULL) {
		fprintf(stderr, "Error: received invalid token\n");
		return NULL;
	}

	if (token->type == NOT || token->type == MINUS) {

		TokenType type = token->type;
		advanceToken();

		Expression* right = parsePrimaryExpression();

		if (right == NULL) {
			return NULL;
		}

		Expression* unaryExpression = calloc(1, sizeof(Expression));

		if (unaryExpression == NULL) {
			freeExpression(right);
			return NULL;
		}

		unaryExpression->as.unop = parseUnaryOperation(type, right);

		if (unaryExpression->as.unop == NULL) {
			freeExpression(right);
			freeExpression(unaryExpression);
			return NULL;
		}

		unaryExpression->type = UNARY_EXPR;

		return unaryExpression;
	}

	Expression* expression = NULL;

	switch(token->type) {
		case NUM:
		case FNUM:
			expression = calloc(1, sizeof(Expression));
			expression->type = VALUE_EXPR;
			expression->as.value = parseValue(token);

			if (expression->as.value == NULL) {
				goto error;
			}

			advanceToken();
			break;

		case ID:
			expression = calloc(1, sizeof(Expression));
			expression->type = VARIABLE_EXPR;
			expression->as.variable = parseVariable(token);

			if (expression->as.variable == NULL) {
				goto error;
			}

			advanceToken();
			break;
		case LPAREN:
			advanceToken();

			expression = parseExpression(0);

			if (expression == NULL) {
				return NULL;
			}

			if (peekToken() == NULL || peekToken()->type != RPAREN) {
				fprintf(stderr, "Error: Expected token ')'.\n");
				return NULL;
			}

			advanceToken();
			break;
        	default:
			fprintf(stderr, "Error: Unexpected token in primary expression.\n");
			goto error;
	}

	return expression;

error:
	freeExpression(expression);
	return NULL;
}

Expression* parseExpression(int minPrecedence) {
	Expression* leftExpression = parsePrimaryExpression();

	if (leftExpression == NULL) {
		fprintf(stderr, "Error: failed to allocate expression in parser\n");
		return NULL;
	}

	while (1) {
		Token* token = peekToken();

		if (token == NULL || token->type == E_O_F) {
			break;
		}

		int precedence = getBinOpPrecedence(token->type);

		if (precedence == 0 || precedence < minPrecedence) {
			break;
		}

		TokenType type = token->type;

		advanceToken();

		int nextMinPrecedence = (type == ASSIGN) ? precedence : precedence + 1;
		Expression* rightExpression = parseExpression(nextMinPrecedence);

		if (rightExpression == NULL) {
			freeExpression(leftExpression);
			return NULL;
		}

		Expression* newLeftExpression = calloc(1, sizeof(Expression));

		if (newLeftExpression == NULL) {
			fprintf(stderr, "Error: failed to allocate expression in parser\n");
			freeExpression(leftExpression);
			freeExpression(rightExpression);
			return NULL;
		}

		if (type == ASSIGN) {
			if(leftExpression->type != VARIABLE_EXPR) {
				fprintf(stderr, "Error: Invalid target for assignment. Must be a variable.\n");
				freeExpression(leftExpression);
				freeExpression(rightExpression);
				freeExpression(newLeftExpression);
				return NULL;
			}
			newLeftExpression->type = ASSIGN_EXPR;
			newLeftExpression->as.assignment = parseAssignment(leftExpression->as.variable, rightExpression);

		}

		else {
			newLeftExpression->type = BINOP_EXPR;
			BinOperationType binOpType;
			
			switch (type) {
				case PLUS:
					binOpType = ADD_OP;
					break;
				case MINUS:
					binOpType = SUB_OP;
					break;
				case MUL:
					binOpType = MUL_OP;
					break;
				case DIV:
					binOpType = DIV_OP;
					break;
				case MODULUS:
					binOpType = MOD_OP;
					break;
				case ST:
					binOpType = ST_OP;
					break;
				case STE:
					binOpType = STE_OP;
					break;
				case GT:
					binOpType = GT_OP;
					break;
				case GTE:
					binOpType = GTE_OP;
					break;
				case EQUALS:
					binOpType = EQ_OP;
					break;
				case NEQUALS:
					binOpType = NEQ_OP;
					break;
				default:
					break;
			}

			newLeftExpression->as.binop = parseBinOperation(binOpType, leftExpression, rightExpression);

			if (newLeftExpression->as.binop == NULL) {
				freeExpression(leftExpression);
				freeExpression(rightExpression);
				freeExpression(newLeftExpression);
				return NULL;
			}
		}

		leftExpression = newLeftExpression;
	}
	return leftExpression;
}

BinOperation* parseBinOperation(BinOperationType type, Expression* left, Expression* right) {
	BinOperation* binOp = calloc(1, sizeof(BinOperation));

	if (binOp == NULL) {
		return NULL;
	}

	binOp->type = type;
	binOp->left = left;
	binOp->right = right;

	return binOp;
}

UnaryOperation* parseUnaryOperation(TokenType type, Expression* right) {
	UnaryOperation* unaryOperation = calloc(1, sizeof(UnaryOperation));

	if (unaryOperation == NULL) {
		return NULL;
	}

	unaryOperation->type = type;
	unaryOperation->right = right;
	return unaryOperation;
}

BlockStmt* parseBlockStmt() {
	BlockStmt* blockStmt = calloc(1, sizeof(BlockStmt));

	if (blockStmt == NULL) {
		return NULL;
	}

	blockStmt->stmts = dynamicArray(2);

	if (blockStmt->stmts == NULL) {
		freeBlockStmt(blockStmt);
		return NULL;
	}

	while (peekToken() != NULL && peekToken()->type != RCURL) {

		Statement* statement = parseStatement();

		if (statement == NULL) {
			freeBlockStmt(blockStmt);
			return NULL;
		}

		appendStmt(blockStmt->stmts, statement);
	}

	if (peekToken() == NULL || peekToken()->type != RCURL) {
		fprintf(stderr, "Error: Expected '}'\n");
		freeBlockStmt(blockStmt);
		return NULL;
	}

	advanceToken();
	return blockStmt;
}

WhileStmt* parseWhileStmt() {
	WhileStmt* whileStmt = calloc(1, sizeof(WhileStmt));

	if (whileStmt == NULL) {
		return NULL;
	}

	whileStmt->condition = parseExpression(0);
	
	if (whileStmt->condition == NULL) {
		freeWhileStmt(whileStmt);
		return NULL;
	}

	if (peekToken() == NULL || peekToken()->type != LCURL) {
		fprintf(stderr, "Error: Expected '{' after while condition\n");
		freeWhileStmt(whileStmt);
		return NULL;
	}

	advanceToken();

	whileStmt->body = parseBlockStmt();

	if (whileStmt->body == NULL) {
		freeWhileStmt(whileStmt);
		return NULL;
	}

	return whileStmt;

}

IfStmt* parseIfStmt() {
	IfStmt* ifStmt = calloc(1, sizeof(IfStmt));

	if (ifStmt == NULL) {
		return NULL;
	}

	ifStmt->condition = parseExpression(0);
	
	if (ifStmt->condition == NULL) {
		freeIfStmt(ifStmt);
		return NULL;
	}

	if (peekToken() == NULL || peekToken()->type != LCURL) {
		fprintf(stderr, "Error: Expected '{' after if condition\n");
		fprintf(stderr, "token type was %d\n", peekToken()->type);
		freeIfStmt(ifStmt);
		return NULL;
	}

	advanceToken();

	ifStmt->trueBody = parseBlockStmt();

	if (ifStmt->trueBody == NULL) {
		freeIfStmt(ifStmt);
		return NULL;
	}

	ifStmt->type = ONLYIF;

	if (peekToken() != NULL && peekToken()->type == ELSE) {
		advanceToken();
		ifStmt->type = IF_ELSE;

		if (peekToken() != NULL && peekToken()->type == IF) {
			advanceToken();

			ifStmt->type = IF_ELSE_IF;
			ifStmt->as.ifElseIf = parseIfStmt();

			if (ifStmt->as.ifElseIf == NULL) {
				freeIfStmt(ifStmt);
				return NULL;
			}

			return ifStmt;
		}

		if (peekToken() == NULL || peekToken()->type != LCURL) {
			fprintf(stderr, "Error: Expected '{' after else\n");
			freeIfStmt(ifStmt);
			return NULL;
		}

		advanceToken();

		ifStmt->as.ifElse = parseBlockStmt();

		if (ifStmt->as.ifElse == NULL) {
			freeIfStmt(ifStmt);
			return NULL;
		}
	}

	return ifStmt;
}

Assignment* parseAssignment(Variable* variable, Expression* expression) {
	Assignment* assignment = calloc(1, sizeof(Assignment));
	
	if (assignment == NULL) {
		return NULL;
	}

	assignment->variable = variable;
	assignment->expression = expression;

	return assignment;
}

Variable* parseVariable(Token* token) {
	Variable* variable = calloc(1, sizeof(Variable));

	if (variable == NULL) {
		return NULL;
	}

	variable->id = parseToken(token);
	
	if (variable->id == NULL) {
		freeVariable(variable);
	}

	return variable;
}

Value* parseValue(Token* token) {
	Value* value = calloc(1, sizeof(Value));

	if (value == NULL) return NULL;

	switch (token->type) {
		case NUM:
			value->type = LONG_TYPE;
			value->as.i_32 = strtol(token->start, NULL, 10);
			break;
		case FNUM:
			value->type = FLOAT_TYPE;
			value->as.f = strtod(token->start, NULL);
			break;
		default:
			return NULL;
	}

	return value;

}

void freeStatement(Statement* statement) {

	if (statement == NULL) {
		return;
	}

	switch (statement->type) {
		case EXPRESSION_STMT:
			freeExpression(statement->as.expression);
			break;
		case BLOCK_STMT:
			freeBlockStmt(statement->as.blockStmt);
			break;
		case WHILE_STMT:
			freeWhileStmt(statement->as.whileStmt);
			break;
		case IF_STMT:
			freeIfStmt(statement->as.ifStmt);
			break;
		case DECLARATION_STMT:
			freeDeclaration(statement->as.declaration);
			break;
		case E_O_F_STMT:
			break;
	}
	
	free(statement);
}

void freeBlockStmt(BlockStmt* blockStmt) {

	if (blockStmt == NULL) {
		return;
	}

	freeArray(blockStmt->stmts);
	free(blockStmt);
}

void freeExpression(Expression* expression) {

	if (expression == NULL) {
		return;
	}

	switch (expression->type) {
		case BINOP_EXPR:
			freeBinOperation(expression->as.binop);
			break;
		case UNARY_EXPR:
			freeUnaryOperation(expression->as.unop);
			break;
		case ASSIGN_EXPR:
			freeAssignment(expression->as.assignment);
			break;
		case VARIABLE_EXPR:
			freeVariable(expression->as.variable);
			break;
		case VALUE_EXPR:
			freeValue(expression->as.value);
			break;
		default:
			break;
		
	}
	
	free(expression);
}

void freeBinOperation(BinOperation* binOperation) {

	if (binOperation == NULL) {
		return;
	}

	freeExpression(binOperation->right);
	freeExpression(binOperation->left);
	free(binOperation);
}

void freeUnaryOperation(UnaryOperation* unaryOperation) {

	if (unaryOperation == NULL) {
		return;
	}

	freeExpression(unaryOperation->right);
	free(unaryOperation);
}

void freeWhileStmt(WhileStmt* whileStmt) {

	if (whileStmt == NULL) {
		return;
	}

	freeExpression(whileStmt->condition);
	freeBlockStmt(whileStmt->body);
	free(whileStmt);
}

void freeIfStmt(IfStmt* ifStmt) {

	if (ifStmt == NULL) {
		return;
	}

	freeExpression(ifStmt->condition);
	freeBlockStmt(ifStmt->trueBody);

	if (ifStmt->type == IF_ELSE) {
		freeBlockStmt(ifStmt->as.ifElse);
	}
	else if (ifStmt->type == IF_ELSE_IF) {
		freeIfStmt(ifStmt->as.ifElseIf);
	}

	free(ifStmt);
}

void freeAssignment(Assignment* assignment) {

	if (assignment == NULL) {
		return;
	}

	freeVariable(assignment->variable);
	freeExpression(assignment->expression);
	free(assignment);
}

void freeDeclaration(Declaration* declaration) {

	if (declaration == NULL) {
		return;
	}

	freeVariable(declaration->variable);
	freeExpression(declaration->initializer);
	free(declaration);
}

void freeVariable(Variable* variable) {

	if (variable == NULL) {
		return;
	}

	free(variable->id);
	free(variable);
}

void freeValue(Value* value) {

	if (value == NULL) {
		return;
	}

	if (value->type == STR_TYPE) {
		free(value->as.str);
	}
	free(value);
}

void freeParser() {
	freeArray(statements);
	freeTable(identifiers);
}
