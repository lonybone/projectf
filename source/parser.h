#ifndef PARSER_H
#define PARSER_H

typedef struct Expression Expression;

typedef enum {
	INT,
	LONG,
	LONG_LONG,
	FLOAT,
	DOUBLE,
	CHAR,
	STR
} ValueType;

typedef struct { 
	ValueType type;
	union {
		int i_16;
		long i_32;
		long long i_64;
		float fl;
		double dfl;
		char ch;
		char* str;
	} vals;
} Value;

typedef enum {
	ADD,
	SUB,
	MUL,
	DIV,
	EQ,
	NEQ,
} OperationType;

typedef struct {
	char* id;
	Value* val;
} Variable;

typedef struct {
	Variable* var;
	Expression* value;
} Assign;

typedef struct {
	Variable* variable;
	Assign* assign;
} Declaration;

typedef struct {
	OperationType type;
	Expression* left;
	Expression* right;

} BinOperation;

typedef enum {
	ASSIGN,
	BINOP,
	VALUE,
	DECLARATION
} ExpressionType;

struct Expression {
	ExpressionType type;
	union {
		Assign* assign;
		BinOperation* binop;
		Value* value;
		Declaration* declaration;
	} next;
};

#endif
