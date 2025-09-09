#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "parser.h"
#include "utils.h"

Statement* parseStatement(Parser* parser);
FunctionStmt* parseFunctionStmt(Parser* parser, Token* typeToken, Token* idToken);
BlockStmt* parseBlockStmt(Parser* parser);
WhileStmt* parseWhileStmt(Parser* parser);
IfStmt* parseIfStmt(Parser* parser);
Expression* parseExpression(Parser* parser, int minPrecedence);
Expression* parsePrimaryExpression(Parser* parser);
Expression* parseExpression_V2(Parser* parser);
Expression* parseExpressionToAst(Parser* parser);
Expression* descent(Expression* this);
FunctionCall* parseFunctionCall(Parser* parser, Token* idToken);
BinOperation* parseBinOperation(BinOperationType type, Expression* left, Expression* right);
UnaryOperation* parseUnaryOperation(TokenType type, Expression* right);
Assignment* parseAssignment(Variable* variable, Expression* expression);
Declaration* parseDeclaration(); // not implemented
Value* parseValue(Token* token);
Variable* parseVariable(Token* token);

void freeStatement(void* statement);
void freeFunctionStmt(void* functionStmt);
void freeExpression(void* expression);
void freeBinOperation(BinOperation* binOperation);
void freeUnaryOperation(UnaryOperation* unaryOperation);
void freeFunctionCall(FunctionCall* functionCall);
void freeBlockStmt(BlockStmt* blockStmt);
void freeWhileStmt(WhileStmt* whileStmt);
void freeIfStmt(IfStmt* ifStmt);
void freeAssignment(Assignment* assignment);
void freeReturnStmt(ReturnStmt* returnStmt);
void freeDeclaration(Declaration* declaration);
void freeVariable(void* variable);
void freeValue(Value* value);

int VERSION = V2_CUSTOM;

Parser* initializeParser(char* buffer) {

	Lexer* lexer = initializeLexer(buffer);

	if (lexer == NULL) {
		return NULL;
	}

	Parser* parser = calloc(1, sizeof(Parser));

	if (parser == NULL) {
		return NULL;
	}
	
	DynamicArray* statements = dynamicArray(2, freeStatement);

	if (statements == NULL ) {
		freeParser(parser);
		return NULL;
	}

	parser->lexer = lexer;
	parser->statements = statements;
	parser->version = VERSION;

	return parser;
}

DynamicArray* parseBuffer(Parser* parser) {

	if (parser == NULL) {
		return NULL;
	}

	for (;;) {
		Statement* statement = parseStatement(parser);

		if (statement == NULL) {
			freeArray(parser->statements);
			parser->statements = dynamicArray(2, freeStatement);
			return NULL;
		}

		if (statement->type == E_O_F_STMT) {
			freeStatement(statement);
			break;
		}

		pushItem(parser->statements, statement);
	}

	DynamicArray* ret = parser->statements;
	parser->statements = dynamicArray(2, freeStatement);

	if (parser->statements == NULL) {
		return NULL;
	}

	return ret;
}

int setParserBuffer(Parser* parser, char* buffer) {
	if (parser == NULL || buffer == NULL) {
		return 0;
	}
	
	if (!setLexerBuffer(parser->lexer, buffer)) {
		return 0;
	}

	return 1;
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

static ValueType getTypeFromToken(Token* token) {
	switch (token->type) {
		case BOOL:
			return BOOL_TYPE;
			break;
		case I16:
			return INT_TYPE;
			break;
		case I32:
			return LONG_TYPE;
			break;
		case I64:
			return LONG_LONG_TYPE;
			break;
		case F32:
			return FLOAT_TYPE;
			break;
		case F64:
			return DOUBLE_TYPE;
			break;
		case CHAR:
			return CHAR_TYPE;
			break;
		case STR:
			return STR_TYPE;
			break;
		default:
			return -1;
	}
}

static int getExpressionPrecedence(Expression* expression) {

	if (expression == NULL) {
		return 0;
	}

	switch (expression->type) {

		case BINOP_EXPR:
			switch (expression->as.binop->type) {
				case ADD_OP:
					return getBinOpPrecedence(PLUS);
				case SUB_OP:
					return getBinOpPrecedence(MINUS);
				case MUL_OP:
					return getBinOpPrecedence(MUL);
				case DIV_OP:
					return getBinOpPrecedence(DIV);
				case MOD_OP:
					return getBinOpPrecedence(MODULUS);
				case ST_OP:
					return getBinOpPrecedence(ST);
				case STE_OP:
					return getBinOpPrecedence(STE);
				case GT_OP:
					return getBinOpPrecedence(GT);
				case GTE_OP:
					return getBinOpPrecedence(GTE);
				case EQ_OP:
					return getBinOpPrecedence(EQUALS);
				case NEQ_OP:
					return getBinOpPrecedence(NEQUALS);
				default:
					break;
			}
		case VARIABLE_EXPR:
		case VALUE_EXPR:
			return 5;
		default:
			return 0;


	}
}

static BinOperationType getBinOpType(TokenType type) {
	switch (type) {
		case PLUS:
			return ADD_OP;
		case MINUS:
			return SUB_OP;
		case MUL:
			return MUL_OP;
		case DIV:
			return DIV_OP;
		case MODULUS:
			return MOD_OP;
		case ST:
			return ST_OP;
		case STE:
			return STE_OP;
		case GT:
			return GT_OP;
		case GTE:
			return GTE_OP;
		case EQUALS:
			return EQ_OP;
		case NEQUALS:
			return NEQ_OP;
		default:
			return -1;
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

		case BOOL:
		case I16:
		case I32:
		case I64:
		case F32:
		case F64:
		case CHAR:
		case STR: {
			Token* typeToken = calloc(1, sizeof(Token));
			if (typeToken == NULL) return NULL;
			memcpy(typeToken, token, sizeof(Token));

			advanceToken(parser->lexer);

			Token* idToken = calloc(1, sizeof(Token));
			if (idToken == NULL) return NULL;
			memcpy(idToken, peekToken(parser->lexer), sizeof(Token));

			advanceToken(parser->lexer);

			if (idToken->type != ID) {
				fprintf(stderr, "Error: Unexpected Token %d after Type Annotation\n", idToken->type);
				free(idToken);
				free(typeToken);
				goto error;
			}

			Token* nextToken = calloc(1, sizeof(Token));
			if (nextToken == NULL) {
				free(idToken);
				free(typeToken);
				free(nextToken);
				goto error;
			}
			memcpy(nextToken, peekToken(parser->lexer), sizeof(Token));

			if (nextToken->type == LPAREN) {
				advanceToken(parser->lexer);

				statement->type = FUNCTION_STMT;
				statement->as.function = parseFunctionStmt(parser, typeToken, idToken);

				if (statement->as.function == NULL) {
					free(idToken);
					free(typeToken);
					free(nextToken);
					goto error;
				}
			}

			else if (nextToken->type == ASSIGN) {
				advanceToken(parser->lexer);

				statement->type = EXPRESSION_STMT;

				statement->as.expression = calloc(1, sizeof(Expression));

				if (statement->as.expression == NULL) {
					return NULL;
				}

				statement->as.expression->type = ASSIGN_EXPR;
				Variable* variable = parseVariable(idToken);

				if (variable == NULL) {
					free(idToken);
					free(typeToken);
					free(nextToken);
					goto error;
				}

				variable->type = getTypeFromToken(typeToken);

				Expression* expression;

				if (parser->version == V1_PRATT) {
					expression = parseExpression(parser, 0);
				}
				else {
					expression = parseExpression_V2(parser);
				}

				if (expression == NULL) {
					freeVariable(variable);
					free(idToken);
					free(typeToken);
					free(nextToken);
					goto error;
				}

				statement->as.expression->as.assignment = parseAssignment(variable, expression); 
				
				if (statement->as.expression->as.assignment == NULL) {
					freeVariable(variable);
					freeExpression(expression);
					free(idToken);
					free(typeToken);
					free(nextToken);
					goto error;
				}

			}

			// annotated declaration without assignment
			else {
				statement->type = EXPRESSION_STMT;

				statement->as.expression = calloc(1, sizeof(Expression));

				if (statement->as.expression == NULL) {
					return NULL;
				}

				statement->as.expression->type = VARIABLE_EXPR;
				Variable* variable = parseVariable(idToken);

				if (variable == NULL) {
					free(idToken);
					free(typeToken);
					free(nextToken);
					goto error;
				}

				statement->as.expression->as.variable = variable;
				variable->type = getTypeFromToken(typeToken);
			}

			free(idToken);
			free(typeToken);
			free(nextToken);
			break;
		}
		case ID:
		case NUM:
		case FNUM:
		case TRUE:
		case FALSE:
		case LPAREN: {
			statement->type = EXPRESSION_STMT;
			Expression* expression;

			if (parser->version == V1_PRATT) {
				expression = parseExpression(parser, 0);
			}
			else {
				expression = parseExpression_V2(parser);
			}

			if (expression == NULL) {
				goto error;
			}

			expression->valueType = UNKNOWN;
			statement->as.expression = expression;
			break;
		}
		case RETURN: {
			advanceToken(parser->lexer);

			statement->as.returnStmt = calloc(1, sizeof(ReturnStmt));
			statement->type = RETURN_STMT;

			if (statement->as.returnStmt == NULL) {
				goto error;
			}

			Expression* expression;

			if (parser->version == V1_PRATT) {
				expression = parseExpression(parser, 0);
			}
			else {
				expression = parseExpression_V2(parser);
			}

			if (expression == NULL) {
				goto error;
			}

			expression->valueType = UNKNOWN;
			statement->as.returnStmt->expression = expression;
			break;
		}
		default: {
			fprintf(stderr,"Error: Unexpected Token in Statement of type %d\n", token->type);
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

		right->valueType = UNKNOWN;

		Expression* unaryExpression = calloc(1, sizeof(Expression));

		if (unaryExpression == NULL) {
			freeExpression(right);
			return NULL;
		}

		unaryExpression->valueType = UNKNOWN;
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
		case TRUE:
		case FALSE:
		case NUM:
		case FNUM:
			expression = calloc(1, sizeof(Expression));

			if (expression == NULL) {
				goto error;
			}

			expression->type = VALUE_EXPR;
			expression->valueType = UNKNOWN;
			expression->as.value = parseValue(token);

			if (expression->as.value == NULL) {
				goto error;
			}

			advanceToken(parser->lexer);
			break;

		case ID:
			expression = calloc(1, sizeof(Expression));

			if (expression == NULL) {
				goto error;
			}
			expression->valueType = UNKNOWN;

			Token* idToken = calloc(1, sizeof(Token));
			if (idToken == NULL) {
				freeExpression(expression);
				goto error;
			}
			memcpy(idToken, peekToken(parser->lexer), sizeof(Token));

			advanceToken(parser->lexer);

			if (peekToken(parser->lexer)->type == LPAREN) {
				advanceToken(parser->lexer);
				expression->type = FUNCTIONCALL_EXPR;
				expression->as.functionCall = parseFunctionCall(parser, idToken);
				free(idToken);

				if (expression->as.functionCall == NULL) {
					freeExpression(expression);
					goto error;
				}
			}
			else {
				expression->type = VARIABLE_EXPR;
				expression->as.variable = parseVariable(idToken);
				free(idToken);

				if (expression->as.variable == NULL) {
					freeExpression(expression);
					goto error;
				}
			}
			break;

		case LPAREN:
			advanceToken(parser->lexer);

			if (parser->version == V1_PRATT) {
				expression = parseExpression(parser, 0);
			}
			else {
				expression = parseExpression_V2(parser);
			}

			if (expression == NULL) {
				return NULL;
			}

			expression->valueType = UNKNOWN;

			if (peekToken(parser->lexer) == NULL || peekToken(parser->lexer)->type != RPAREN) {
				fprintf(stderr, "Error: Expected token ')'.\n");
				goto error;
			}
			
			if (parser->version == V2_CUSTOM) {
				Expression* wrapper = calloc(1, sizeof(Expression));

				if (wrapper == NULL) {
					goto error;
				}

				wrapper->type = EXPR_WRAPPER_EXPR;
				wrapper->valueType = UNKNOWN;
				wrapper->as.expWrap = expression;
				expression = wrapper;
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

		rightExpression->valueType = UNKNOWN;

		Expression* newLeftExpression = calloc(1, sizeof(Expression));

		if (newLeftExpression == NULL) {
			fprintf(stderr, "Error: Failed to allocate expression in parser\n");
			freeExpression(leftExpression);
			freeExpression(rightExpression);
			return NULL;
		}

		newLeftExpression->valueType = UNKNOWN;

		if (type == ASSIGN) {
			if (leftExpression->type != VARIABLE_EXPR) {
				fprintf(stderr, "Error: Invalid target for assignment. Must be a variable.\n");
				freeExpression(leftExpression);
				freeExpression(rightExpression);
				freeExpression(newLeftExpression);
				return NULL;
			}

			if (rightExpression->type == ASSIGN_EXPR) {
				fprintf(stderr, "Error: Invalid Expression for assignment. Cannot Assign an Assignment to a Variable.\n");
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
			BinOperationType binOpType = getBinOpType(type);

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

Expression* parseExpression_V2(Parser* parser) {
	Expression* expression = parseExpressionToAst(parser);

	if (expression == NULL) {
		return NULL;
	}

	if (expression->type == ASSIGN_EXPR) {
		expression->as.assignment->expression = descent(expression->as.assignment->expression);

		if (expression->as.assignment->expression == NULL) {
			freeExpression(expression);
			return NULL;
		}

		return expression;
	}

	expression = descent(expression);

	if (expression == NULL) {
		freeExpression(expression);
		return NULL;
	}

	return expression;
}

Expression* parseExpressionToAst(Parser* parser) {

	if (parser == NULL) {
		return NULL;
	}

	Expression* leftExpression = parsePrimaryExpression(parser);

	if (leftExpression == NULL) {
		fprintf(stderr, "Error: Failed to allocate expression in parser\n");
		return NULL;
	}

	Token* token = peekToken(parser->lexer);

	if (token == NULL || token->type == E_O_F) {
		return leftExpression;
	}

	int precedence = getBinOpPrecedence(token->type);

	if (precedence == 0) {
		return leftExpression;
	}

	TokenType type = token->type;
	advanceToken(parser->lexer);

	if (type == ASSIGN) {

		if (leftExpression->type != VARIABLE_EXPR) {
			fprintf(stderr, "Error: Invalid target for assignment. Must be a variable.\n");
			freeExpression(leftExpression);
			return NULL;
		}

		Expression* assignment = calloc(1, sizeof(Expression));

		if (assignment == NULL) {
			freeExpression(leftExpression);
			return NULL;
		}

		assignment->type = ASSIGN_EXPR;

		Expression* rightExpression = parseExpressionToAst(parser);

		if (rightExpression == NULL) {
			freeExpression(leftExpression);
			freeExpression(assignment);
			return NULL;
		}

		if (rightExpression->type == ASSIGN_EXPR) {
			fprintf(stderr, "Error: Invalid Expression for assignment. Cannot Assign an Assignment to a Variable.\n");
			freeExpression(leftExpression);
			return NULL;
		}

		assignment->as.assignment = parseAssignment(leftExpression->as.variable, rightExpression);

		if (assignment->as.assignment == NULL) {
			freeExpression(assignment);
			freeExpression(leftExpression);
			freeExpression(rightExpression);
			return NULL;
		}

		free(leftExpression);
		return assignment;
	}

	else {
		Expression* binOperation = calloc(1, sizeof(Expression));

		if (binOperation == NULL) {
			freeExpression(leftExpression);
			return NULL;
		}

		binOperation->type = BINOP_EXPR;
		BinOperationType binOpType = getBinOpType(type);

		Expression* rightExpression = parseExpressionToAst(parser);

		if (rightExpression == NULL) {
			freeExpression(leftExpression);
			freeExpression(binOperation);
			return NULL;
		}

		binOperation->as.binop = parseBinOperation(binOpType, leftExpression, rightExpression);

		if (binOperation->as.binop == NULL || binOperation->as.binop->right == NULL) {
			freeExpression(binOperation);
			freeExpression(leftExpression);
			freeExpression(rightExpression);
			return NULL;
		}

		return binOperation;
	}
}

Expression* descent(Expression* this) {

	if (this == NULL || this->type != BINOP_EXPR) {
		return this;
	}

	while (this->as.binop->right != NULL && this->as.binop->right->type == BINOP_EXPR && getExpressionPrecedence(this->as.binop->right) <= getExpressionPrecedence(this)) {

		Expression* prevThis = this;
		Expression* prevRLCH = this->as.binop->right->as.binop->left;
		this = this->as.binop->right;

		this->as.binop->left = prevThis;
		prevThis->as.binop->right = prevRLCH;
	}

	this->as.binop->right = descent(this->as.binop->right);
	return this;
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

FunctionStmt* parseFunctionStmt(Parser* parser, Token* typeToken, Token* idToken) {
	if (parser == NULL || typeToken == NULL || idToken == NULL) {
		return NULL;
	}

	FunctionStmt* function = calloc(1, sizeof(FunctionStmt));

	if (function == NULL) {
		return NULL;
	}

	function->returnType = getTypeFromToken(typeToken);
	function->id = parseToken(idToken);

	if (function->id == NULL) {
		freeFunctionStmt(function);
		return NULL;
	}

	function->params = dynamicArray(2, freeVariable);

	if (function->params == NULL) {
		freeFunctionStmt(function);
		return NULL;
	}

	while (1) {
		if (peekToken(parser->lexer)->type == RPAREN) {
			advanceToken(parser->lexer);
			break;
		}

		Token* paramType = calloc(1, sizeof(Token));
		if (paramType == NULL) return NULL;
		memcpy(paramType, peekToken(parser->lexer), sizeof(Token));

		if (getTypeFromToken(paramType) == -1) {
			fprintf(stderr, "Error: Expected Type Token but got %d instead\n", paramType->type);
			free(paramType);
			freeFunctionStmt(function);
			return NULL;
		}

		advanceToken(parser->lexer);

		Token* paramId = calloc(1, sizeof(Token));
		if (paramId == NULL) return NULL;
		memcpy(paramId, peekToken(parser->lexer), sizeof(Token));

		advanceToken(parser->lexer);

		if (paramId->type != ID) {
			fprintf(stderr, "Error: Type Token isn't followed by ID Token\n");
			free(paramType);
			free(paramId);
			freeFunctionStmt(function);
			return NULL;
		}

		Variable* param = parseVariable(paramId);
		param->type = getTypeFromToken(paramType);
		pushItem(function->params, param);

		free(paramType);
		free(paramId);

		if (peekToken(parser->lexer)->type == RPAREN) {
			advanceToken(parser->lexer);
			break;
		}

		if (peekToken(parser->lexer)->type != COLON) {
			fprintf(stderr, "Error: Expected Token ',' after a Function Parameter\n");
			freeFunctionStmt(function);
			return NULL;
		}

		advanceToken(parser->lexer);
	}

	if (peekToken(parser->lexer)->type != LCURL) {
		fprintf(stderr, "Error: Expected Token '{' after a Function Parameters\n");
		freeFunctionStmt(function);
		return NULL;
	}
	advanceToken(parser->lexer);

	function->blockStmt = parseBlockStmt(parser);
	if (function->blockStmt == NULL) {
		freeFunctionStmt(function);
		return NULL;
	}

	return function;
}

BlockStmt* parseBlockStmt(Parser* parser) {

	if (parser == NULL) {
		return NULL;
	}

	BlockStmt* blockStmt = calloc(1, sizeof(BlockStmt));

	if (blockStmt == NULL) {
		return NULL;
	}

	blockStmt->stmts = dynamicArray(2, freeStatement);

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

		pushItem(blockStmt->stmts, statement);
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

	if (parser->version == V1_PRATT) {
		whileStmt->condition = parseExpression(parser, 0);
	}
	else {
		whileStmt->condition = parseExpression_V2(parser);
	}
	
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

	if (parser->version == V1_PRATT) {
		ifStmt->condition = parseExpression(parser, 0);
	}
	else {
		ifStmt->condition = parseExpression_V2(parser);
	}
	
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

FunctionCall* parseFunctionCall(Parser* parser, Token* idToken) {
	if (parser == NULL || idToken == NULL) {
		return NULL;
	}

	FunctionCall* function = calloc(1, sizeof(FunctionCall));

	if (function == NULL) {
		return NULL;
	}

	function->id = parseToken(idToken);

	if (function->id == NULL) {
		freeFunctionCall(function);
		return NULL;
	}

	function->params = dynamicArray(2, freeExpression);
	
	if (function->params == NULL) {
		freeFunctionCall(function);
		return NULL;
	}

	while (1) {
		if (peekToken(parser->lexer)->type == RPAREN) {
			advanceToken(parser->lexer);
			break;
		}

		Expression* param;

		if (parser->version == V1_PRATT) {
			param = parseExpression(parser, 0);
		}
		else {
			param = parseExpression_V2(parser);
		}

		if (param == NULL) {
			freeFunctionCall(function);
			return NULL;
		}

		pushItem(function->params, param);

		if (peekToken(parser->lexer)->type == RPAREN) {
			advanceToken(parser->lexer);
			break;
		}

		if (peekToken(parser->lexer)->type != COLON) {
			fprintf(stderr, "Error: Expected Token ',' after a Function Argument\n");
			freeFunctionCall(function);
			return NULL;
		}

		advanceToken(parser->lexer);
	}

	return function;
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

	variable->type = UNKNOWN;
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
			value->as.i_32 = 0;
			for (int i = 0; i < token->length; ++i) {
				char c = token->start[i];
				if (c >= '0' && c <= '9') {
					value->as.i_32 = value->as.i_32 * 10 + (c - '0');
				}
			}
			break;
		case FNUM:
			value->type = DOUBLE_TYPE;
			value->as.df = strtod(token->start, NULL);
			break;
		case TRUE:
			value->type = BOOL_TYPE;
			value->as.b = 1;
			break;
		case FALSE:
			value->type = BOOL_TYPE;
			value->as.b = 0;
			break;
		default:
			fprintf(stderr, "Error: Type not implemented yet\n");
			return NULL;
	}

	return value;

}

void freeStatement(void* statement) {

	if (statement == NULL) {
		return;
	}

	Statement* stmt = (Statement*) statement;

	switch (stmt->type) {
		case EXPRESSION_STMT:
			freeExpression(stmt->as.expression);
			break;
		case FUNCTION_STMT:
			freeFunctionStmt(stmt->as.function);
			break;
		case BLOCK_STMT:
			freeBlockStmt(stmt->as.blockStmt);
			break;
		case WHILE_STMT:
			freeWhileStmt(stmt->as.whileStmt);
			break;
		case IF_STMT:
			freeIfStmt(stmt->as.ifStmt);
			break;
		case DECLARATION_STMT:
			freeDeclaration(stmt->as.declaration);
			break;
		case RETURN_STMT:
			freeReturnStmt(stmt->as.returnStmt);
			break;
		case E_O_F_STMT:
			break;
	}
	
	free(stmt);
}

void freeBlockStmt(BlockStmt* blockStmt) {

	if (blockStmt == NULL) {
		return;
	}

	freeArray(blockStmt->stmts);
	free(blockStmt);
}

void freeExpression(void* expression) {

	Expression* expr = (Expression*)expression;

	if (expression == NULL) {
		return;
	}

	switch (expr->type) {
		case EXPR_WRAPPER_EXPR:
			freeExpression(expr->as.expWrap);
			break;
		case FUNCTIONCALL_EXPR:
			freeFunctionCall(expr->as.functionCall);
			break;
		case BINOP_EXPR:
			freeBinOperation(expr->as.binop);
			break;
		case UNARY_EXPR:
			freeUnaryOperation(expr->as.unop);
			break;
		case ASSIGN_EXPR:
			freeAssignment(expr->as.assignment);
			break;
		case VARIABLE_EXPR:
			freeVariable(expr->as.variable);
			break;
		case VALUE_EXPR:
			freeValue(expr->as.value);
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

void freeFunctionStmt(void* functionStmt) {

	FunctionStmt* func = (FunctionStmt*) functionStmt;

	if (functionStmt == NULL) {
		return;
	}

	free(func->id);
	freeArray(func->params);
	freeBlockStmt(func->blockStmt);
	free(func);
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

void freeReturnStmt(ReturnStmt* returnStmt) {
	if (returnStmt == NULL) {
		return;
	}
	freeExpression(returnStmt->expression);
	free(returnStmt);
}

void freeFunctionCall(FunctionCall* functionCall) {
	if (functionCall == NULL) {
		return;
	}
	free(functionCall->id);
	freeArray(functionCall->params);
	free(functionCall);
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

void freeVariable(void* variable) {

	Variable* var = (Variable*) variable;

	if (var == NULL) {
		return;
	}

	free(var->id);
	free(var);
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
	freeLexer(parser->lexer);
	freeArray(parser->statements);
	free(parser);
}
