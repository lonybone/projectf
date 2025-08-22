#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "parser.h"
#include "utils.h"

Statement* parseStatement(Parser* parser);
Expression* parseExpression(Parser* parser, int minPrecedence);
Expression* parsePrimaryExpression(Parser* parser);
BinOperation* parseBinOperation(BinOperationType type, Expression* left, Expression* right);
UnaryOperation* parseUnaryOperation(TokenType type, Expression* right);
BlockStmt* parseBlockStmt(Parser* parser);
WhileStmt* parseWhileStmt(Parser* parser);
IfStmt* parseIfStmt(Parser* parser);
Assignment* parseAssignment(Variable* variable, Expression* expression);
Declaration* parseDeclaration(); // not implemented
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

Parser* initializeParser(Lexer* lexer) {
	if (lexer == NULL) {
		return NULL;
	}

	Parser* parser = calloc(1, sizeof(Parser));

	if (parser == NULL) {
		return NULL;
	}
	
	DynamicArray* statements = dynamicArray(2);
	HashTable* identifiers = hashTable(256);

	if (statements == NULL || identifiers == NULL) {
		freeArray(statements);
		freeTable(identifiers);
		freeParser(parser);
		return NULL;
	}

	parser->lexer = lexer;
	parser->statements = statements;
	parser->identifiers = identifiers;

	return parser;
}

DynamicArray* parseBuffer(Parser* parser) {

	if (parser == NULL) {
		return NULL;
	}

	for (;;) {
		Statement* statement = parseStatement(parser);

		if (statement == NULL) {
			return NULL;
		}

		if (statement->type == E_O_F_STMT) {
			freeStatement(statement);
			break;
		}

		appendStmt(parser->statements, statement);
	}

	return parser->statements;
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
		fprintf(stderr, "Error: Failed to allocate memory for tokenstring\n");
		return NULL;
	}

	memcpy(tokenString, token->start, token->length);

	tokenString[token->length] = '\0';
	return tokenString;
}

Statement* parseStatement(Parser* parser) {

	if (parser == NULL) {
		return NULL;
	}

	Token* token = peekToken(parser->lexer);

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
			advanceToken(parser->lexer);
			statement->type = IF_STMT;
			statement->as.ifStmt = parseIfStmt(parser);

			if (statement->as.ifStmt == NULL) {
				goto error;
			}

			break;

		case WHILE:
			advanceToken(parser->lexer);
			statement->type = WHILE_STMT;
			statement->as.whileStmt = parseWhileStmt(parser);

			if (statement->as.whileStmt == NULL) {
				goto error;
			}

			break;
			
		case LCURL:
			advanceToken(parser->lexer);
			statement->type = BLOCK_STMT;
			statement->as.blockStmt = parseBlockStmt(parser);

			if (statement->as.blockStmt == NULL) {
				goto error;
			}
			
			break;

		case ID:
		case NUM:
		case FNUM:
		case LPAREN:
			statement->type = EXPRESSION_STMT;
			Expression* expression = parseExpression(parser, 0);
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
	fprintf(stderr, "Error: Failed to parse Statement\n");
	freeStatement(statement);
	return NULL;

}

Expression* parsePrimaryExpression(Parser* parser) {

	if (parser == NULL) {
		return NULL;
	}

	Token* token = peekToken(parser->lexer);

	if (token == NULL) {
		fprintf(stderr, "Error: Received invalid token\n");
		return NULL;
	}

	if (token->type == NOT || token->type == MINUS) {

		TokenType type = token->type;
		advanceToken(parser->lexer);

		Expression* right = parsePrimaryExpression(parser);

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

			advanceToken(parser->lexer);
			break;

		case ID:
			expression = calloc(1, sizeof(Expression));
			expression->type = VARIABLE_EXPR;
			expression->as.variable = parseVariable(token);

			if (expression->as.variable == NULL) {
				goto error;
			}

			advanceToken(parser->lexer);
			break;
		case LPAREN:
			advanceToken(parser->lexer);

			expression = parseExpression(parser, 0);

			if (expression == NULL) {
				return NULL;
			}

			if (peekToken(parser->lexer) == NULL || peekToken(parser->lexer)->type != RPAREN) {
				fprintf(stderr, "Error: Expected token ')'.\n");
				return NULL;
			}

			advanceToken(parser->lexer);
			break;
        	default:
			fprintf(stderr, "Error: Unexpected token type %d in primary expression.\n", peekToken(parser->lexer)->type);
			goto error;
	}

	return expression;

error:
	freeExpression(expression);
	return NULL;
}

Expression* parseExpression(Parser* parser, int minPrecedence) {

	if (parser == NULL) {
		return NULL;
	}

	Expression* leftExpression = parsePrimaryExpression(parser);

	if (leftExpression == NULL) {
		fprintf(stderr, "Error: Failed to allocate expression in parser\n");
		return NULL;
	}

	while (1) {
		Token* token = peekToken(parser->lexer);

		if (token == NULL || token->type == E_O_F) {
			break;
		}

		int precedence = getBinOpPrecedence(token->type);

		if (precedence == 0 || precedence < minPrecedence) {
			break;
		}

		TokenType type = token->type;

		advanceToken(parser->lexer);

		int nextMinPrecedence = (type == ASSIGN) ? precedence : precedence + 1;
		Expression* rightExpression = parseExpression(parser, nextMinPrecedence);

		if (rightExpression == NULL) {
			freeExpression(leftExpression);
			return NULL;
		}

		Expression* newLeftExpression = calloc(1, sizeof(Expression));

		if (newLeftExpression == NULL) {
			fprintf(stderr, "Error: Failed to allocate expression in parser\n");
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

			if (newLeftExpression->as.assignment == NULL) {
				freeExpression(leftExpression);
				freeExpression(rightExpression);
				freeExpression(newLeftExpression);
				return NULL;
			}

			free(leftExpression);
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

BlockStmt* parseBlockStmt(Parser* parser) {

	if (parser == NULL) {
		return NULL;
	}

	BlockStmt* blockStmt = calloc(1, sizeof(BlockStmt));

	if (blockStmt == NULL) {
		return NULL;
	}

	blockStmt->stmts = dynamicArray(2);

	if (blockStmt->stmts == NULL) {
		freeBlockStmt(blockStmt);
		return NULL;
	}

	while (peekToken(parser->lexer) != NULL && peekToken(parser->lexer)->type != RCURL) {

		Statement* statement = parseStatement(parser);

		if (statement == NULL) {
			freeBlockStmt(blockStmt);
			return NULL;
		}

		appendStmt(blockStmt->stmts, statement);
	}

	if (peekToken(parser->lexer) == NULL || peekToken(parser->lexer)->type != RCURL) {
		fprintf(stderr, "Error: Expected '}'\n");
		freeBlockStmt(blockStmt);
		return NULL;
	}

	advanceToken(parser->lexer);
	return blockStmt;
}

WhileStmt* parseWhileStmt(Parser* parser) {

	if (parser == NULL) {
		return NULL;
	}

	WhileStmt* whileStmt = calloc(1, sizeof(WhileStmt));

	if (whileStmt == NULL) {
		return NULL;
	}

	whileStmt->condition = parseExpression(parser, 0);
	
	if (whileStmt->condition == NULL) {
		freeWhileStmt(whileStmt);
		return NULL;
	}

	if (peekToken(parser->lexer) == NULL || peekToken(parser->lexer)->type != LCURL) {
		fprintf(stderr, "Error: Expected '{' after while condition\n");
		freeWhileStmt(whileStmt);
		return NULL;
	}

	advanceToken(parser->lexer);

	whileStmt->body = parseBlockStmt(parser);

	if (whileStmt->body == NULL) {
		freeWhileStmt(whileStmt);
		return NULL;
	}

	return whileStmt;

}

IfStmt* parseIfStmt(Parser* parser) {

	if (parser == NULL) {
		return NULL;
	}

	IfStmt* ifStmt = calloc(1, sizeof(IfStmt));

	if (ifStmt == NULL) {
		return NULL;
	}

	ifStmt->condition = parseExpression(parser, 0);
	
	if (ifStmt->condition == NULL) {
		freeIfStmt(ifStmt);
		return NULL;
	}

	if (peekToken(parser->lexer) == NULL || peekToken(parser->lexer)->type != LCURL) {
		fprintf(stderr, "Error: Expected '{' after if condition\n");
		fprintf(stderr, "token type was %d\n", peekToken(parser->lexer)->type);
		freeIfStmt(ifStmt);
		return NULL;
	}

	advanceToken(parser->lexer);

	ifStmt->trueBody = parseBlockStmt(parser);

	if (ifStmt->trueBody == NULL) {
		freeIfStmt(ifStmt);
		return NULL;
	}

	ifStmt->type = ONLYIF;

	if (peekToken(parser->lexer) != NULL && peekToken(parser->lexer)->type == ELSE) {
		advanceToken(parser->lexer);
		ifStmt->type = IF_ELSE;

		if (peekToken(parser->lexer) != NULL && peekToken(parser->lexer)->type == IF) {
			advanceToken(parser->lexer);

			ifStmt->type = IF_ELSE_IF;
			ifStmt->as.ifElseIf = parseIfStmt(parser);

			if (ifStmt->as.ifElseIf == NULL) {
				freeIfStmt(ifStmt);
				return NULL;
			}

			return ifStmt;
		}

		if (peekToken(parser->lexer) == NULL || peekToken(parser->lexer)->type != LCURL) {
			fprintf(stderr, "Error: Expected '{' after else\n");
			freeIfStmt(ifStmt);
			return NULL;
		}

		advanceToken(parser->lexer);

		ifStmt->as.ifElse = parseBlockStmt(parser);

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
		return NULL;
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

void freeParser(Parser* parser) {
	if (parser == NULL) {
		return;
	}
	freeArray(parser->statements);
	freeTable(parser->identifiers);
}
