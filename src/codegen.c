#include "codegen.h"
#include "parser.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

int checkTypes(Codegen* codegen);
int checkStatement(DynamicArray* typeScopes, Statement* statement);
int checkBlockStmt(DynamicArray* typeScopes, BlockStmt* blockStmt);
int checkWhileStmt(DynamicArray* typeScopes, WhileStmt* whileStmt);
int checkIfStmt(DynamicArray* typeScopes, IfStmt* ifStmt);
int checkExpression(DynamicArray* typeScopes, Expression* expression);
int checkAssignment(DynamicArray* typeScopes, Assignment* assignment);
int checkBinOperation(DynamicArray* typeScopes, BinOperation* binOperation);
int checkUnaryOperation(DynamicArray* typeScopes, UnaryOperation* unaryOperation);
int checkVariable(DynamicArray* typeScopes, Variable* variable);
int checkValue(DynamicArray* typeScopes, Value* value);

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
	codegen->idTable = hashTable(256);

	if (codegen->idTable == NULL) {
		freeCodegen(codegen);
		return NULL;
	}

	return codegen;
}

int checkTypes(Codegen* codegen) {

	if (codegen == NULL) {
		return 1;
	}

	DynamicArray* typeScopes = dynamicArray(2);

	if (typeScopes == NULL) {
		return 1;
	}

	HashTable* typeTable = hashTable(256);

	if (typeTable == NULL) {
		return 1;
	}

	if (pushItem(typeScopes, typeTable) == 1) return 1;

	for (int i = 0; i < codegen->ast->size; i++) {
		if (checkStatement(typeScopes, codegen->ast->array[i]) == 1) { 

			for (int i = 0; i < codegen->ast->size; i++) {
				HashTable* table = (HashTable*)codegen->ast->array[i];
				freeTable(table);
			}

			freeArray(typeScopes);
			return 1;
		}
	}

	for (int i = 0; i < codegen->ast->size; i++) {
		HashTable* table = (HashTable*)codegen->ast->array[i];
		freeTable(table);
	}

	freeArray(typeScopes);
	return 0;
}

int checkStatement(DynamicArray* typeScopes, Statement* statement) {
	
	if (statement == NULL) {
		return 1;
	}
	
	switch (statement->type) {
		case EXPRESSION_STMT:
			return checkExpression(typeScopes, statement->as.expression);
		case BLOCK_STMT:
			return checkBlockStmt(typeScopes, statement->as.blockStmt);
		case WHILE_STMT:
			return checkWhileStmt(typeScopes, statement->as.whileStmt);
		case IF_STMT:
			return checkIfStmt(typeScopes, statement->as.ifStmt);
		case DECLARATION_STMT:
		default:
			fprintf(stderr, "Error: unexpected Statement type in checkStatement");
			return 1;
	}
}

int checkExpression(DynamicArray* typeScopes, Expression* expression) {
	if (expression == NULL) {
		return 1;
	}

	switch (expression->type) {
		case EXPR_WRAPPER_EXPR:
			return checkExpression(typeScopes, expression->as.expWrap);
		case ASSIGN_EXPR:
			return checkAssignment(typeScopes, expression->as.assignment);
		case BINOP_EXPR:
			return checkBinOperation(typeScopes, expression->as.binop);
		case UNARY_EXPR:
			return checkUnaryOperation(typeScopes, expression->as.unop);
		case VARIABLE_EXPR:
			return checkVariable(typeScopes, expression->as.variable);
		case VALUE_EXPR:
			return checkValue(typeScopes, expression->as.value);
		default:
			fprintf(stderr, "Error: unexpected Expression type in checkExpression");
			return 1;
	}
}

// check if variable already has a type, if so then check wether expression is of same type
// if variable has no type then this is a declaration and the type shall be annotated
int checkAssignment(DynamicArray* typeScopes, Assignment* assignment) { return 0; }
// check wether left and right expression satisfy the same type recursively
// annotate own type after checking if both children have the same type as that type
// if one expression has no type yet then this is a compile time error of attempting to access non initialized variable
int checkBinOperation(DynamicArray* typeScopes, BinOperation* binOperation) { return 0; }
// check wether expression is of type boolean (! operator) or a number (- operator)
int checkUnaryOperation(DynamicArray* typeScopes, UnaryOperation* unaryOperation) { return 0; }
// if the variable has a type then just return
// if the variable has no type, then also return since its a declaration w/o initialization
int checkVariable(DynamicArray* typeScopes, Variable* variable) { return 0; }
// just return the value
int checkValue(DynamicArray* typeScopes, Value* value) { return 0; }

int checkBlockStmt(DynamicArray* typeScopes, BlockStmt* blockStmt) {
	if (blockStmt == NULL) {
		return 1;
	}

	for (int i = 0; i < blockStmt->stmts->size; i++) {
		if(checkStatement(typeScopes, blockStmt->stmts->array[i]) == 1) return 1;
	}

	return 0;
}

int checkWhileStmt(DynamicArray* typeScopes, WhileStmt* whileStmt) {
	if (whileStmt == NULL) {
		return 1;
	}

	Expression* condition = whileStmt->condition;
	if(checkExpression(typeScopes, whileStmt->condition) == 1 || condition->valueType != BOOL_TYPE) return 1;
	if(checkBlockStmt(typeScopes, whileStmt->body) == 1) return 1;

	return 0;
}

int checkIfStmt(DynamicArray* typeScopes, IfStmt* ifStmt) {
	if (ifStmt == NULL) {
		return 1;
	}

	Expression* condition = ifStmt->condition;
	if(checkExpression(typeScopes, ifStmt->condition) == 1 || condition->valueType != BOOL_TYPE) return 1;

	if (ifStmt->type == IF_ELSE) {
		if(checkBlockStmt(typeScopes, ifStmt->as.ifElse) == 1) return 1;
	}

	else if (ifStmt->type == IF_ELSE_IF) {
		if(checkIfStmt(typeScopes, ifStmt->as.ifElseIf) == 1) return 1;
	}

	return 0;
}
// TODO:
int generate(Codegen* codegen);

int addToBuffer(Codegen* codegen, const char* token) {

	if (codegen == NULL || token == NULL) {
		return 1;
	}

	int token_len = strlen(token);

	if (codegen->idx + token_len > codegen->maxSize) {

		int new_size = codegen->maxSize * 2;

		while (new_size < codegen->idx + token_len) {
			new_size *= 2;
		}

		char* new_buffer = realloc(codegen->buffer, new_size);

		if (new_buffer == NULL) {
			return 1;
		}

		codegen->buffer = new_buffer;
		codegen->maxSize = new_size;
	}

	memcpy(codegen->buffer + codegen->idx, token, token_len);
	codegen->idx += token_len;

	return 0;
}

int generateStatement(Codegen* codegen, Statement* statement) { return 0; }
int generateUnaryOperation(Codegen* codegen, UnaryOperation* unaryOperation) { return 0; }
int generateBlockStmt(Codegen* codegen, BlockStmt* blockStmt) { return 0; }
int generateWhileStmt(Codegen* codegen, WhileStmt* whileStmt) { return 0; }
int generateIfStmt(Codegen* codegen, IfStmt* ifStmt) { return 0; }
int generateAssignment(Codegen* codegen, Assignment* assignment) { return 0; }
int generateValue(Codegen* codegen, Value* value) { return 0; }
int generateVariable(Codegen* codegen, Variable* variable) { return 0; }

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
		return 1;
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
			return 1;
	}
}

int generateBinOperation(Codegen* codegen, BinOperation* binOperation) {
	
	if (codegen == NULL || binOperation == NULL) {
		return 1;
	}

	if (generateExpression(codegen, binOperation->right) == 1) return 1;
	if (addToBuffer(codegen, "push rax\n") == 1) return 1;
	if (generateExpression(codegen, binOperation->left) == 1) return 1;
	if (addToBuffer(codegen, "pop rdx\n") == 1) return 1;

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
			return 1;
		case ST_OP:
			// addToBuffer(codegen, "cmp rax, rdx\njl label");
			fprintf(stderr, "Error: Operator jl not implemented yet");
			return 1;
		case STE_OP:
			// addToBuffer(codegen, "cmp rax, rdx\njle label");
			fprintf(stderr, "Error: Operator jle not implemented yet");
			return 1;
		case GT_OP:
			// addToBuffer(codegen, "cmp rax, rdx\njg label");
			fprintf(stderr, "Error: Operator jg not implemented yet");
			return 1;
		case GTE_OP:
			// addToBuffer(codegen, "cmp rax, rdx\njge label");
			fprintf(stderr, "Error: Operator jge not implemented yet");
			return 1;
		case EQ_OP:
			// addToBuffer(codegen, "cmp rax, rdx\nje label");
			fprintf(stderr, "Error: Operator je not implemented yet");
			return 1;
		case NEQ_OP:
			// addToBuffer(codegen, "cmp rax, rdx\njne label");
			fprintf(stderr, "Error: Operator jne not implemented yet");
			return 1;
		default:
			return 1;
	}
}

void freeCodegen(Codegen* codegen) {
	if (codegen == NULL) {
		return;
	}
	free(codegen->buffer);
	freeTable(codegen->idTable);
	free(codegen);
}
