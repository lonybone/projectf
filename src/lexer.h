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
	BOOL,
	I16,
	I32,
	I64,
	F32,
	F64,
	CHAR,
	STR,
	TRUE,
	FALSE,
	RETURN,
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
