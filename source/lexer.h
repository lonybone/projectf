#include <stdio.h>

#ifndef LEXER_H
#define LEXER_H

typedef enum {
	E_O_F,
	SEMICOLON,
	COLON,
	LCURL,
	RCURL,
	LSQUR,
	RSQUR,
	LPAREN,
	RPAREN,
	PLUS,
	MINUS,
	MODULUS,
	MUL,
	DIV,
	EQUALS,
	ASSIGN,
	NEQUALS,
	NOT,
	DQUOTE,
	SQUOTE,
	NUM,
	FLOAT,
	IF,
	ELSE,
	WHILE,
	INT,
	LONG,
	BOOL,
	CHAR,
	STR,
	TRUE,
	FALSE,
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
