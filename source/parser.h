#ifndef PARSER_H
#define PARSER_H

typedef struct Expression Expression;

typedef enum {
	INT_TYPE,
	LONG_TYPE,
	LONG_LONG_TYPE,
	FLOAT_TYPE,
	DOUBLE_TYPE,
	CHAR_TYPE,
	STR_TYPE
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

typedef enum {
	ADD_OP,
	SUB_OP,
	MUL_OP,
	DIV_OP,
	EQ_OP,
	NEQ_OP
} BinOperationType;

typedef struct {
	BinOperationType type;
	Expression* left;
	Expression* right;

} BinOperation;

typedef enum {
	ASSIGN_EXPR,
	BINOP_EXPR,
	VALUE_EXPR,
	DECLARATIO_EXPR
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
