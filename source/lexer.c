#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

char *buff;
int idx;

void initializeLexer(char *b) {
	buff = b;
	idx = 0;
}

Token *getToken() {

	while (buff[idx] == ' ' || buff[idx] == '\n' || buff[idx] == ' ' || buff[idx] == '\r') {
		idx++;
	}

	Token *token = malloc(sizeof(Token));
	token->start = &buff[idx];
	token->length = 1;

	switch (buff[idx]) {

		case '\0':
			token->type = E_O_F;
			return token;
		case ';':
			token->type = SEMICOLON;
			break;
		case '{':
			token->type = LCURL;
			break;
		case '}':
			token->type = RCURL;
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
		case '"':
			token->type = DQUOTE;
			while (buff[idx + token->length] != '"' && buff[idx] + token->length != '\0') {
				token->length++;
			}
			token->length++; 
			break;
		case '\'':
			token->type = SQUOTE;
			while (buff[idx + token->length] != '\'' && buff[idx] + token->length != '\0') {
				token->length++;
			}
			token->length++;
			break;
		default:

			if (isdigit(buff[idx])) {
				token->type = NUM;
				while (isdigit(buff[idx + token->length])) {
					token->length++;
				}
			}

			else if (isalpha(buff[idx])) {
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

	// Prepare for next Token
	idx += token->length;
	return token;
}
