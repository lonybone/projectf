#include <stdlib.h>
#include "parser.h"
#include "lexer.h"
#include "utils.h"

DynamicArray* statements;

void initializeParser() {
	// array for holding all statements one level beneath root
	statements = dynamicArray(2);
}

Statement* parseStatement() {

	Token* token = getToken();
	Statement* statement = malloc(sizeof(Statement));

	while (token != NULL) {

		if (token->type == E_O_F) {
			free(token);
			break;
		}

		

		free(token);
		token = getToken();
	}

	if (token == NULL) {
		return NULL;
	}
	
	return NULL;
}

void freeParser() {
	//TODO: free the entire AST recursively (oh god)
}
