#include <stdio.h>

#ifndef LEXER_H
#define LEXER_H

typedef struct Lexer Lexer;
typedef struct Token Token;

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
	STE,
	ST,
	GTE,
	GT,
	DQUOTE,
	SQUOTE,
	NUM,
	FNUM,
	IF,
	ELSE,
	WHILE,
	INT,
	LONG,
	FLOAT,
	DOUBLE,
	BOOL,
	CHAR,
	STR,
	TRUE,
	FALSE,
	ID,
} TokenType;

struct Lexer {
	char* buff;
	int idx;
	Token* currentToken;
};

struct Token {
	TokenType type;
	const char* start;
	int length;
};

Lexer* initializeLexer(char* b);
void freeLexer(Lexer* lexer);
int setLexerBuffer(Lexer* lexer, char* buffer);
Token* getToken(Lexer* lexer);
Token* peekToken(Lexer* lexer);
void advanceToken(Lexer* lexer);

#endif
