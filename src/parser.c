#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "lexer.h"
#include "utils.h"

DynamicArray* statements;
HashTable* identifiers;

void initializeParser() {
	// array for holding all statements one level beneath root
	statements = dynamicArray(2);
	identifiers = hashTable(256);
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

	Token* token = getToken();
	if (token == NULL) {
		fprintf(stderr, "Error: received no token in parser");
		return NULL;
	}

	Statement* statement = malloc(sizeof(Statement));
	if (statement == NULL) {
		fprintf(stderr, "Error: failed to allocate statement in parser");
		free(token);
		return NULL;
	}

	if (token->type == E_O_F) {
		free(token);
		statement->type = E_O_F_STMT;
		return statement;
	}

	switch (token->type) {

		case LCURL:
			statement->type = BLOCK_STMT;
			statement->as.blockStmt = parseBlockStmt();

			if (statement->as.blockStmt == NULL) {
				goto error;
			}
			
			break;

		case ID: {
			char* tokenString = parseToken(token);

			if (tokenString == NULL) {
				goto error;
			}

			if (insertKeyPair(identifiers,tokenString, NULL) == SUCCESS) {
				statement->type = DECLARATION_STMT;
				statement->as.declaration = parseDeclaration();
				if (statement->as.declaration == NULL) {
					free(tokenString);
					goto error;
				}
			}

			else {
				statement->type = EXPRESSION_STMT;
				statement->as.expression = parseExpression();

				if (statement->as.expression == NULL) {
					free(tokenString);
					goto error;
				}
			}
			
			free(tokenString);
			break;
		}
		case WHILE:
			statement->type = WHILE_STMT;
			statement->as.whileStmt = parseWhileStmt();

			if (statement->as.whileStmt == NULL) {
				goto error;
			}

			break;

		case IF:
			statement->type = IF_STMT;
			statement->as.ifStmt = parseIfStmt();

			if (statement->as.ifStmt == NULL) {
				goto error;
			}

			break;
			
		default: {
			char* tokenString = parseToken(token);

			if (tokenString == NULL) {
				goto error;
			}

			fprintf(stderr,"Unexpected token: %s in parser", tokenString);
			free(tokenString);
			goto error;
		}

	}

	free(token);	
	return statement;

error:
	fprintf(stderr, "Error: failed to parse Statement");
	free(token);
	free(statement);
	return NULL;	

}

void freeParser() {
	//TODO: free the entire AST recursively (oh god)
}
