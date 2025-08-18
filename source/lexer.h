#include <stdio.h>

#ifndef LEXER_H
#define LEXER_H

typedef enum {
	E_O_F,
	SEMICOLON,
	LCURL,
	RCURL,
	LPAREN,
	RPAREN,
	PLUS,
	MINUS,
	MODULUS,
	MUL,
	DIV,
	EQUALS,
	ASSIGN,
	DQUOTE,
	SQUOTE,
	NUM,
	ID,
} TokenType;

typedef struct {
	TokenType type;
	const char* start;
	int length;
} Token;

void initializeLexer(char *b);
Token *getToken();

#endif
