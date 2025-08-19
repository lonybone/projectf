#ifndef PARSER_H
#define PARSER_H

struct DynamicArray;

typedef struct BlockStmt BlockStmt;
typedef struct Statement Statement;
typedef struct Expression Expression;
typedef struct BinOperation BinOperation;
typedef struct WhileStmt WhileStmt;
typedef struct IfStmt IfStmt;
typedef struct Assignment Assignment;
typedef struct Declaration Declaration;
typedef struct Variable Variable;
typedef struct Value Value;

typedef enum {
	EXPRESSION_STMT,
	ASSIGN_STMT,
	DECLARATION_EXPR
} StatementType;

typedef enum {
	ASSIGN_EXPR,
	BINOP_EXPR,
	VALUE_EXPR,
} ExpressionType;

typedef enum {
	ADD_OP,
	SUB_OP,
	MUL_OP,
	DIV_OP,
	EQ_OP,
	NEQ_OP
} BinOperationType;

typedef enum {
	INT_TYPE,
	LONG_TYPE,
	LONG_LONG_TYPE,
	FLOAT_TYPE,
	DOUBLE_TYPE,
	CHAR_TYPE,
	STR_TYPE
} ValueType;

struct BlockStmt {
	struct DynamicArray* stmts;
};

struct Statement {
	StatementType type;
	union {
		Expression* expression;
		Assignment* assignment;
		Declaration* declaration;
	} as;
};

struct Expression {
	ExpressionType type;
	union {
		BinOperation* binop;
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

struct WhileStmt {
	Expression* condition;
	BlockStmt* body;
};

struct IfStmt {
	Expression* condition;
	BlockStmt* trueBody;
	BlockStmt* falseBody;
};

struct Assignment {
	Variable* var;
	Expression* value;
};

struct Declaration {
	char* name;
	ValueType type;
	Expression* initializer;
};

struct Variable {
	char* id;
};

struct Value {
	ValueType* type;
	union {
		int i_16;
		long i_32;
		long long i_64;
		float f;
		double df;
		char ch;
		char* str;
	} as;
};

#endif
