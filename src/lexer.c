#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

Lexer* initializeLexer(char* b) {
	Lexer* lexer = calloc(1, sizeof(Lexer));

	if (lexer == NULL) {
		return NULL;
	}

	lexer->buff= b;
	lexer->idx = 0;
	lexer->currentToken = getToken(lexer);

	if (lexer->currentToken == NULL) {
		freeLexer(lexer);
		return NULL;
	}

	return lexer;
}

void freeLexer(Lexer* lexer) {
	if (lexer == NULL) {
		return;
	}
	free(lexer->currentToken);
	free(lexer);
}

int setLexerBuffer(Lexer* lexer, char* buffer) {
	if (lexer == NULL || buffer == NULL) {
		return 0;
	}

	lexer->buff = buffer;
	lexer->idx = 0;
	lexer->currentToken = getToken(lexer);

	if (lexer->currentToken == NULL) {
		freeLexer(lexer);
		return 0;
	}

	return 1;
}

Token* getToken(Lexer* lexer) {

	if (lexer == NULL) {
		return NULL;
	}

	char* buff = lexer->buff;

	while (buff[lexer->idx] == ' ' || buff[lexer->idx] == '\n' || buff[lexer->idx] == '\t' || buff[lexer->idx] == '\r') {
		lexer->idx++;
	}

	Token* token = malloc(sizeof(Token));

	if (token == NULL) {
		fprintf(stderr, "Error: Failed to allocate token in lexer");
		return NULL;
	}

	token->start = &buff[lexer->idx];
	token->length = 1;

	switch (buff[lexer->idx]) {

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
			token->type = LSQUR;
			break;
		case ']':
			token->type = RSQUR;
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
			if (buff[lexer->idx+1] == '=') {
				token->length++;
				token->type = EQUALS;
			}
			else {
				token->type = ASSIGN;
			}
			break;
		case '!':
			if (buff[lexer->idx+1] == '=') {
				token->length++;
				token->type = NEQUALS;
			}
			else {
				token->type = NOT;
			}
			break;

		case '<':
			if (buff[lexer->idx+1] == '=') {
				token->length++;
				token->type = STE;
			}
			else {
				token->type = ST;
			}
			break;
		case '>':
			if (buff[lexer->idx+1] == '=') {
				token->length++;
				token->type = GTE;
			}
			else {
				token->type = GT;
			}
			break;
		case '"':
			token->type = DQUOTE;
			while (buff[lexer->idx + token->length] != '"' && buff[lexer->idx + token->length] != '\0') {
				token->length++;
			}
			token->length++; 
			break;
		case '\'':
			token->type = SQUOTE;
			while (buff[lexer->idx + token->length] != '\'' && buff[lexer->idx + token->length] != '\0') {
				token->length++;
			}
			token->length++;
			break;
		default:

			if (isdigit(buff[lexer->idx])) {
				token->type = NUM;
				bool isFloat = false;
				while (isdigit(buff[lexer->idx + token->length]) || buff[lexer->idx + token->length] == '.') {
					if (buff[lexer->idx + token->length] == '.') {
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

			else if (isalpha(buff[lexer->idx])) {

				if (buff[lexer->idx] == 'i' && buff[lexer->idx+1] == 'f' && !isalnum(buff[lexer->idx+2])) {
					token->type = IF;
					token->length = 2;
					break;
				}

				if (buff[lexer->idx] == 'e' && buff[lexer->idx+1] == 'l' && buff[lexer->idx+2] == 's' && buff[lexer->idx+3] == 'e' && !isalnum(buff[lexer->idx+4])) {
					token->type = ELSE;
					token->length = 4;
					break;
				}

				if (buff[lexer->idx] == 'w' && buff[lexer->idx+1] == 'h' && buff[lexer->idx+2] == 'i' && buff[lexer->idx+3] == 'l' && buff[lexer->idx+4] == 'e' && !isalnum(buff[lexer->idx+5])) {
					token->type = WHILE;
					token->length = 5;
					break;
				}

				if (buff[lexer->idx] == 'b' && buff[lexer->idx+1] == 'o' && buff[lexer->idx+2] == 'o' && buff[lexer->idx+3] == 'l' && !isalnum(buff[lexer->idx+4])) {
					token->type = BOOL;
					token->length = 4;
					break;
				}

				if (buff[lexer->idx] == 'i' && buff[lexer->idx+1] == '1' && buff[lexer->idx+2] == '6') {
					token->type = I16;
					token->length = 3;
					break;
				}

				if (buff[lexer->idx] == 'i' && buff[lexer->idx+1] == '3' && buff[lexer->idx+2] == '2') {
					token->type = I32;
					token->length = 3;
					break;
				}

				if (buff[lexer->idx] == 'i' && buff[lexer->idx+1] == '6' && buff[lexer->idx+2] == '4') {
					token->type = I64;
					token->length = 3;
					break;
				}

				if (buff[lexer->idx] == 'f' && buff[lexer->idx+1] == '3' && buff[lexer->idx+2] == '2') {
					token->type = F32;
					token->length = 3;
					break;
				}

				if (buff[lexer->idx] == 'f' && buff[lexer->idx+1] == '6' && buff[lexer->idx+2] == '4') {
					token->type = F64;
					token->length = 3;
					break;
				}

				if (buff[lexer->idx] == 'c' && buff[lexer->idx+1] == 'h' && buff[lexer->idx+2] == 'a' && buff[lexer->idx+3] == 'r' && !isalnum(buff[lexer->idx+4])) {
					token->type = CHAR;
					token->length = 4;
					break;
				}

				if (buff[lexer->idx] == 's' && buff[lexer->idx+1] == 't' && buff[lexer->idx+2] == 'r' && !isalnum(buff[lexer->idx+3])) {
					token->type = STR;
					token->length = 3;
					break;
				}

				if (buff[lexer->idx] == 'r' && buff[lexer->idx+1] == 'e' && buff[lexer->idx+2] == 't' && buff[lexer->idx+3] == 'u' && buff[lexer->idx+4] == 'r' && buff[lexer->idx+5] == 'n' && !isalnum(buff[lexer->idx+6])) {
					token->type = RETURN;
					token->length = 6;
					break;
				}

				if (buff[lexer->idx] == 't' && buff[lexer->idx+1] == 'r' && buff[lexer->idx+2] == 'u' && buff[lexer->idx+3] == 'e' && !isalnum(buff[lexer->idx+4])) {
					token->type = TRUE;
					token->length = 4;
					break;
				}

				if (buff[lexer->idx] == 'f' && buff[lexer->idx+1] == 'a' && buff[lexer->idx+2] == 'l' && buff[lexer->idx+3] == 's' && buff[lexer->idx+4] == 'e' && !isalnum(buff[lexer->idx+5])) {
					token->type = FALSE;
					token->length = 5;
					break;
				}

				token->type = ID;
				while (isalnum(buff[lexer->idx + token->length])) {
					token->length++;
				}
			}
			
			else {
				fprintf(stderr, "Unexpected Token: %c\n", buff[lexer->idx]);
				free(token);
				return NULL;
			}
	}

	return token;
}

Token* peekToken(Lexer* lexer) {
	if (lexer == NULL) {
		return NULL;
	}
	return lexer->currentToken;
}

void advanceToken(Lexer* lexer) {

	if (lexer == NULL) {
		return;
	}

	if (lexer->currentToken == NULL) {
		return;
	}

	lexer->idx += lexer->currentToken->length;

	free(lexer->currentToken);
	lexer->currentToken = getToken(lexer);
}
