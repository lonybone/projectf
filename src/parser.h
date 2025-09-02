#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "utils.h"
#include <stdbool.h>

struct DynamicArray;

typedef struct Parser Parser;
typedef struct Statement Statement;
typedef struct BlockStmt BlockStmt;
typedef struct Expression Expression;
typedef struct BinOperation BinOperation;
typedef struct UnaryOperation UnaryOperation;
typedef struct WhileStmt WhileStmt;
typedef struct IfStmt IfStmt;
typedef struct Assignment Assignment;
typedef struct Declaration Declaration;
typedef struct Variable Variable;
typedef struct Value Value;

typedef enum {
	V1_PRATT,
	V2_CUSTOM
} ParserVersion;

typedef enum {
	EXPRESSION_STMT,
	BLOCK_STMT,
	WHILE_STMT,
	IF_STMT,
	DECLARATION_STMT,
	E_O_F_STMT
} StatementType;

typedef enum {
	EXPR_WRAPPER_EXPR,
	ASSIGN_EXPR,
	BINOP_EXPR,
	UNARY_EXPR,
	VARIABLE_EXPR,
	VALUE_EXPR
} ExpressionType;

typedef enum {
	ADD_OP,
	SUB_OP,
	MUL_OP,
	DIV_OP,
	MOD_OP,
	ST_OP,
	STE_OP,
	GT_OP,
	GTE_OP,
	EQ_OP,
	NEQ_OP
} BinOperationType;

typedef enum {
	ONLYIF,
	IF_ELSE,
	IF_ELSE_IF
} IfStmtOperationType;

typedef enum {
	UNKNOWN,
	BOOL_TYPE,
	INT_TYPE,
	LONG_TYPE,
	LONG_LONG_TYPE,
	FLOAT_TYPE,
	DOUBLE_TYPE,
	CHAR_TYPE,
	STR_TYPE
} ValueType;

struct Parser {
	Lexer* lexer;
	DynamicArray* statements;
	ParserVersion version;
};

struct Statement {
	StatementType type;
	union {
		Expression* expression;
		BlockStmt* blockStmt;
		WhileStmt* whileStmt;
		IfStmt* ifStmt;
		Declaration* declaration;
	} as;
};

struct BlockStmt {
	struct DynamicArray* stmts;
};

struct Expression {
	ExpressionType type;
	ValueType valueType;
	union {
		Expression* expWrap;
		BinOperation* binop;
		UnaryOperation* unop;
		Assignment* assignment;
		Variable* variable;
		Value* value;
	} as;
};

struct BinOperation {
	BinOperationType type;
	Expression* left;
	Expression* right;

};

struct UnaryOperation {
	TokenType type;
	Expression* right;

};

struct WhileStmt {
	Expression* condition;
	BlockStmt* body;
};

struct IfStmt {
	IfStmtOperationType type;
	Expression* condition;
	BlockStmt* trueBody;
	union {
		BlockStmt* ifElse;
		IfStmt* ifElseIf;
	} as;
};

struct Assignment {
	Variable* variable;
	Expression* expression;
};

struct Declaration {
	Variable* variable;
	ValueType type;
	Expression* initializer;
};

struct Variable {
	ValueType type;
	char* id;
};

struct Value {
	ValueType type;
	union {
		bool b;
		int i_16;
		long i_32;
		long long i_64;
		float f;
		double df;
		char ch;
		char* str;
	} as;
};

Parser* initializeParser(char* buffer);
DynamicArray* parseBuffer(Parser* parser);
int setParserBuffer(Parser* parser, char* buffer);
void freeParser(Parser* parser);
void freeStatement(Statement* statement);

#endif
