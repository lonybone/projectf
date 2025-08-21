#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

char* buff;
int idx;
Token* currentToken;

int initializeLexer(char* b) {
	buff = b;
	idx = 0;
	currentToken = getToken();

	if (currentToken == NULL) {
		return 1;
	}

	return 0;
}

void freeLexer() {
	free(currentToken);
}

Token* getToken() {

	while (buff[idx] == ' ' || buff[idx] == '\n' || buff[idx] == '\t' || buff[idx] == ' ' || buff[idx] == '\r') {
		idx++;
	}

	Token* token = malloc(sizeof(Token));

	if (token == NULL) {
		fprintf(stderr, "Error: failed to allocate token in lexer");
		return NULL;
	}

	token->start = &buff[idx];
	token->length = 1;

	switch (buff[idx]) {

		case '\0':
			token->type = E_O_F;
			return token;
		case ',':
			token->type = COLON;
			break;
		case ';':
			token->type = SEMICOLON;
			break;
		case '{':
			token->type = LCURL;
			break;
		case '}':
			token->type = RCURL;
			break;
		case '[':
			token->type = RSQUR;
			break;
		case ']':
			token->type = LSQUR;
			break;
		case '(':
			token->type = LPAREN;
			break;
		case ')':
			token->type = RPAREN;
			break;
		case '+':
			token->type = PLUS;
			break;
		case '-':
			token->type = MINUS;
			break;
		case '%':
			token->type = MODULUS;
			break;
		case '*':
			token->type = MUL;
			break;
		case '/':
			token->type = DIV;
			break;
		case '=':
			if (buff[idx+1] == '=') {
				token->length++;
				token->type = EQUALS;
			}
			else {
				token->type = ASSIGN;
			}
			break;
		case '!':
			if (buff[idx+1] == '=') {
				token->length++;
				token->type = NEQUALS;
			}
			else {
				token->type = NOT;
			}
			break;

		case '<':
			if (buff[idx+1] == '=') {
				token->length++;
				token->type = STE;
			}
			else {
				token->type = ST;
			}
			break;
		case '>':
			if (buff[idx+1] == '=') {
				token->length++;
				token->type = GTE;
			}
			else {
				token->type = GT;
			}
			break;
		case '"':
			token->type = DQUOTE;
			while (buff[idx + token->length] != '"' && buff[idx + token->length] != '\0') {
				token->length++;
			}
			token->length++; 
			break;
		case '\'':
			token->type = SQUOTE;
			while (buff[idx + token->length] != '\'' && buff[idx + token->length] != '\0') {
				token->length++;
			}
			token->length++;
			break;
		default:

			if (isdigit(buff[idx])) {
				token->type = NUM;
				bool isFloat = false;
				while (isdigit(buff[idx + token->length]) || buff[idx + token->length] == '.') {
					if (buff[idx + token->length] == '.') {
						if (isFloat) {
							fprintf(stderr, "INVALID NUMBER TOKEN WITH MORE THAN ONE FLOAT IDENTIFIER\n");
							free(token);
							return NULL;
						}
						isFloat = true;
						token->type = FNUM;
					}
					token->length++;
				}
			}

			else if (isalpha(buff[idx])) {

				if (buff[idx] == 'i' && buff[idx+1] == 'f' && !isalnum(buff[idx+2])) {
					token->type = IF;
					token->length = 2;
					break;
				}

				if (buff[idx] == 'e' && buff[idx+1] == 'l' && buff[idx+2] == 's' && buff[idx+3] == 'e' && !isalnum(buff[idx+4])) {
					token->type = ELSE;
					token->length = 4;
					break;
				}

				if (buff[idx] == 'w' && buff[idx+1] == 'h' && buff[idx+2] == 'i' && buff[idx+3] == 'l' && buff[idx+4] == 'e' && !isalnum(buff[idx+5])) {
					token->type = WHILE;
					token->length = 5;
					break;
				}

				if (buff[idx] == 'i' && buff[idx+1] == 'n' && buff[idx+2] == 't' && !isalnum(buff[idx+3])) {
					token->type = INT;
					token->length = 3;
					break;
				}

				if (buff[idx] == 'l' && buff[idx+1] == 'o' && buff[idx+2] == 'n' && buff[idx+3] == 'g' && !isalnum(buff[idx+4])) {
					token->type = LONG;
					token->length = 4;
					break;
				}
				
				if (buff[idx] == 'f' && buff[idx+1] == 'l' && buff[idx+2] == 'o' && buff[idx+3] == 'a' && buff[idx+4] == 't' && !isalnum(buff[idx+5])) {
					token->type = FLOAT;
					token->length = 5;
					break;
				}

				if (buff[idx] == 'd' && buff[idx+1] == 'o' && buff[idx+2] == 'u' && buff[idx+3] == 'b' && buff[idx+4] == 'l' && buff[idx+5] == 'e' && !isalnum(buff[idx+6])) {
					token->type = DOUBLE;
					token->length = 6;
					break;
				}

				if (buff[idx] == 'b' && buff[idx+1] == 'o' && buff[idx+2] == 'o' && buff[idx+3] == 'l' && !isalnum(buff[idx+4])) {
					token->type = BOOL;
					token->length = 4;
					break;
				}

				if (buff[idx] == 'c' && buff[idx+1] == 'h' && buff[idx+2] == 'a' && buff[idx+3] == 'r' && !isalnum(buff[idx+4])) {
					token->type = CHAR;
					token->length = 4;
					break;
				}

				if (buff[idx] == 's' && buff[idx+1] == 't' && buff[idx+2] == 'r' && !isalnum(buff[idx+3])) {
					token->type = STR;
					token->length = 3;
					break;
				}

				if (buff[idx] == 't' && buff[idx+1] == 'r' && buff[idx+2] == 'u' && buff[idx+3] == 'e' && !isalnum(buff[idx+4])) {
					token->type = TRUE;
					token->length = 4;
					break;
				}

				if (buff[idx] == 'f' && buff[idx+1] == 'a' && buff[idx+2] == 'l' && buff[idx+3] == 's' && buff[idx+4] == 'e' && !isalnum(buff[idx+5])) {
					token->type = FALSE;
					token->length = 5;
					break;
				}

				token->type = ID;
				while (isalnum(buff[idx + token->length])) {
					token->length++;
				}
			}
			
			else {
				fprintf(stderr, "Unexpected Token: %c\n", buff[idx]);
				free(token);
				return NULL;
			}
	}

	return token;
}

Token* peekToken() {
	return currentToken;
}

void advanceToken() {

	if (currentToken == NULL) {
		return;
	}

	idx += currentToken->length;

	free(currentToken);
	currentToken = getToken();
}
