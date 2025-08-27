#include "codegen.h"
#include "parser.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

int generateStatement(Codegen* codegen, Statement* statement);
int generateExpression(Codegen* codegen, Expression* expression);
int generateBinOperation(Codegen* codegen, BinOperation* binOperation);
int generateUnaryOperation(Codegen* codegen, UnaryOperation* unaryOperation);
int generateBlockStmt(Codegen* codegen, BlockStmt* blockStmt);
int generateWhileStmt(Codegen* codegen, WhileStmt* whileStmt);
int generateIfStmt(Codegen* codegen, IfStmt* ifStmt);
int generateAssignment(Codegen* codegen, Assignment* assignment);
int generateValue(Codegen* codegen, Value* value);
int generateVariable(Codegen* codegen, Variable* variable);

Codegen* initializeCodegen(DynamicArray* ast) {
	
	if (ast == NULL) {
		return NULL;
	}

	Codegen* codegen = calloc(1, sizeof(Codegen));

	if (codegen == NULL) {
		return NULL;
	}

	codegen->idx = 0;
	codegen->maxSize = 1024;
	codegen->buffer = malloc(1024);

	if (codegen->buffer == NULL) {
		freeCodegen(codegen);
		return NULL;
	}

	codegen->scopes = dynamicArray(2);

	if (codegen->scopes == NULL) {
		freeCodegen(codegen);
		return NULL;
	}

	codegen->ast = ast;

	return codegen;
}

// TODO:
int generate(Codegen* codegen);

int addToBuffer(Codegen* codegen, const char* token) {

	if (codegen == NULL || token == NULL) {
		return 0;
	}

	int token_len = strlen(token);

	if (codegen->idx + token_len > codegen->maxSize) {

		int new_size = codegen->maxSize * 2;

		while (new_size < codegen->idx + token_len) {
			new_size *= 2;
		}

		char* new_buffer = realloc(codegen->buffer, new_size);

		if (new_buffer == NULL) {
			return 0;
		}

		codegen->buffer = new_buffer;
		codegen->maxSize = new_size;
	}

	memcpy(codegen->buffer + codegen->idx, token, token_len);
	codegen->idx += token_len;

	return 1;
}

int generateStatement(Codegen* codegen, Statement* statement) { return 1; }
int generateUnaryOperation(Codegen* codegen, UnaryOperation* unaryOperation) { return 1; }
int generateBlockStmt(Codegen* codegen, BlockStmt* blockStmt) { return 1; }
int generateWhileStmt(Codegen* codegen, WhileStmt* whileStmt) { return 1; }
int generateIfStmt(Codegen* codegen, IfStmt* ifStmt) { return 1; }
int generateAssignment(Codegen* codegen, Assignment* assignment) { return 1; }
int generateValue(Codegen* codegen, Value* value) { return 1; }
int generateVariable(Codegen* codegen, Variable* variable) { return 1; }

/* 
 Algo:
 eval(right);
 "push rax\n"
 eval(left)
 "pop rdx\n"
 [here the operation due to operation type will be performed:
 	add -> add rax, rdx
	sub -> sub rax, rdx
	mul -> mul rdx
	div -> div rdx
	lt, lte, gt, gte, eq, neq -> cmp rax, rdx\n jl/jle/jg/jge/je/jne label
	unary -> TODO:
 ]

 binops will just do this again
 unops will first call this on their expression member and then logical not on rax
 variables will be looked up and just move their value into rax
 values will just move their value into rax
*/
int generateExpression(Codegen* codegen, Expression* expression) {

	if (codegen == NULL || expression == NULL) {
		return 0;
	}

	switch (expression->type) {
		case EXPR_WRAPPER_EXPR:
			return generateExpression(codegen, expression->as.expWrap);
		case ASSIGN_EXPR:
			return generateAssignment(codegen, expression->as.assignment);
		case UNARY_EXPR:
			return generateUnaryOperation(codegen, expression->as.unop);
		case BINOP_EXPR:
			return generateBinOperation(codegen, expression->as.binop);
		case VARIABLE_EXPR:
			return generateVariable(codegen, expression->as.variable);
		case VALUE_EXPR:
			return generateValue(codegen, expression->as.value);
		default:
			fprintf(stderr, "Error: Unexpected Expression type in generate Expression");
			return 0;
	}
}

int generateBinOperation(Codegen* codegen, BinOperation* binOperation) {
	
	if (codegen == NULL || binOperation == NULL) {
		return 0;
	}

	if (!generateExpression(codegen, binOperation->right)) return 0;
	if (!addToBuffer(codegen, "push rax\n")) return 0;
	if (!generateExpression(codegen, binOperation->left)) return 0;
	if (!addToBuffer(codegen, "pop rdx\n")) return 0;

	switch (binOperation->type) {
		case ADD_OP:
			return addToBuffer(codegen, "add rax, rdx\n");
		case SUB_OP:
			return addToBuffer(codegen, "sub rax, rdx\n");
		case MUL_OP:
			return addToBuffer(codegen, "mul rdx\n");
		case DIV_OP:
			return addToBuffer(codegen, "div rdx\n");
		case MOD_OP:
			fprintf(stderr, "Error: Operator Modulus not implemented yet");
			return 0;
		case ST_OP:
			// addToBuffer(codegen, "cmp rax, rdx\njl label");
			fprintf(stderr, "Error: Operator jl not implemented yet");
			return 0;
		case STE_OP:
			// addToBuffer(codegen, "cmp rax, rdx\njle label");
			fprintf(stderr, "Error: Operator jle not implemented yet");
			return 0;
		case GT_OP:
			// addToBuffer(codegen, "cmp rax, rdx\njg label");
			fprintf(stderr, "Error: Operator jg not implemented yet");
			return 0;
		case GTE_OP:
			// addToBuffer(codegen, "cmp rax, rdx\njge label");
			fprintf(stderr, "Error: Operator jge not implemented yet");
			return 0;
		case EQ_OP:
			// addToBuffer(codegen, "cmp rax, rdx\nje label");
			fprintf(stderr, "Error: Operator je not implemented yet");
			return 0;
		case NEQ_OP:
			// addToBuffer(codegen, "cmp rax, rdx\njne label");
			fprintf(stderr, "Error: Operator jne not implemented yet");
			return 0;
		default:
			fprintf(stderr, "Error: Encountered illegal operand");
			return 0;
	}
}

void freeCodegen(Codegen* codegen) {
	if (codegen == NULL) {
		return;
	}

	for (int i = 0; i < codegen->scopes->size; i++) {
		HashTable* table = (HashTable*)codegen->scopes->array[i];
		freeTable(table);
	}
	
	freeArray(codegen->scopes);;
	free(codegen->buffer);
	free(codegen);
}
