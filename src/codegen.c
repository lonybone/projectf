#include "codegen.h"
#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int generate(Codegen* codegen);
int generateStackFrames(Codegen* codegen);
int generateGlobalStatement(Codegen* codegen, Statement* statement);
int generateGlobalAssignment(Codegen* codegen, Assignment* assignment);
Value* calculateGlobalExpression(Expression* expression);
Value* addValues(BinOperationType binOpType, Value* left, Value* right);
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
int getVariableOffset(Codegen* codegen, char* id);
HashTable* getScopeForVar(Codegen* codegen, char* id);
int getTypeSize(ValueType type);
const char* getRegisterFromSize(int typeSize);

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

	codegen->scopes = dynamicArray(2, freeTable);

	if (codegen->scopes == NULL) {
		freeCodegen(codegen);
		return NULL;
	}

	codegen->ast = ast;
	return codegen;
}

void writeToFile(Codegen* codegen, const char* filepath) {
	FILE* file = fopen(filepath, "wb"); 

	if (file == NULL) {
		fprintf(stderr, "Error: Cannot open file\n");
		return;
	}

	int written = fwrite(codegen->buffer, 1, codegen->idx, file);
	if (written != codegen->idx) {
		fprintf(stderr, "Error while writing to file\n");
		return;
	}

	fclose(file);
}

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
	codegen->buffer[codegen->idx] = '\0';

	return 1;
}

int generate(Codegen* codegen) {

	if (codegen == NULL) {
		return 0;
	}

	for (int i = 0; i < codegen->ast->size; i++) {
		int res;
		if (codegen->scopes->size == 0) {
			res = generateGlobalStatement(codegen, codegen->ast->array[i]);
		}
		else { 
			res = generateStatement(codegen, codegen->ast->array[i]);
		}

		if (!res) {

			freeArray(codegen->scopes);
			free(codegen->buffer);

			codegen->scopes = dynamicArray(2, freeTable);

			if (codegen->scopes == NULL) {
				freeCodegen(codegen);
				return 0;
			}
			
			codegen->buffer = malloc(1024);

			if (codegen->buffer == NULL) {
				freeCodegen(codegen);
				return 0;
			}

			HashTable* typeTable = hashTable(256, free);

			if (typeTable == NULL) {
				freeCodegen(codegen);
				return 0;
			}

			if (!pushItem(codegen->scopes, typeTable))  {
				freeCodegen(codegen);
				return 0;
			}

			return 0;
		}
	}

	return 1;
}

int generateStackFrames(Codegen* codegen);
int generateGlobalStatement(Codegen* codegen, Statement* statement) {
	if (codegen == NULL || statement == NULL) {
		return 0;
	}

	if (statement->type != EXPRESSION_STMT || statement->as.expression->type != ASSIGN_EXPR) {
		fprintf(stderr, "Error: Encountered non Assignment Expression in Global Data Section\n");
		return 0;
	}

	switch (statement->as.expression->valueType) {
		case BOOL_TYPE:
		case LONG_TYPE: {
			return generateGlobalAssignment(codegen, statement->as.expression->as.assignment);
		}
		case DOUBLE_TYPE:
			fprintf(stderr, "Error: Did not implement float Types yet :(\n");
			return 0;
		default:
			fprintf(stderr, "Error: Unrecognized Type in generateGlobalStatement\n");
			return 0;
	}
}

int generateGlobalAssignment(Codegen* codegen, Assignment* assignment) {
	if (codegen == NULL || assignment == NULL) {
		return 0;
	}

	ValueType type = assignment->variable->type;

	if (type == UNKNOWN) {
		fprintf(stderr, "Error: Declarations without Assingment not allowed in global Scope\n");
	}
	
	Value* value = calculateGlobalExpression(assignment->expression);

	if (value == NULL) {
		fprintf(stderr, "Error: Failed to Calculate Value in generateGlobalAssignment\n");
		return 0;
	}

	char* variableId = assignment->variable->id;

	switch (type) {
		char instr[256];
		case BOOL_TYPE: {
			bool val = value->as.b;
			snprintf(instr, sizeof(instr), "\tsection .data\n\tglobal %s\n\t.align 1\n\t.type %s, @object\n\t.size %s, 1\n%s:\n\tdb %d\n\n", variableId, variableId, variableId, variableId, val);
			free(value);
			return addToBuffer(codegen, instr);
		}
		case LONG_TYPE: {
			long long val = value->as.i_64;
			snprintf(instr, sizeof(instr), "\tsection .data\n\tglobal %s\n\t.align 8\n\t.type %s, @object\n\t.size %s, 8\n%s:\n\tdq %lld\n\n", variableId, variableId, variableId, variableId, val);
			free(value);
			return addToBuffer(codegen, instr);
		}
		case DOUBLE_TYPE:
			fprintf(stderr, "Error: Did not implement float Types yet :(\n");
			free(value);
			return 0;
		default:
			fprintf(stderr, "Error: Unrecognized Type in calculate Expression\n");
			free(value);
			return 0;
	}
}

Value* calculateGlobalExpression(Expression* expression) {
	if (expression == NULL) {
		return NULL;
	}

	switch (expression->type) {
		case EXPR_WRAPPER_EXPR:
			return calculateGlobalExpression(expression->as.expWrap);
		case ASSIGN_EXPR:
			fprintf(stderr, "Error: Unexpected Assignment in calculateGlobalExpression\n");
			return NULL;
		case UNARY_EXPR: {
			if (expression->as.unop->type == MINUS) {
				Value* value = calculateGlobalExpression(expression->as.unop->right);
				if (value->type == LONG_TYPE) {
					value->as.i_64 = -value->as.i_64;
					return value;
				}
				free(value);
				fprintf(stderr, "Error: Unexpected type for Unary Operand -\n");
				return NULL;
			}
			if (expression->as.unop->type == NOT) {
				Value* value = calculateGlobalExpression(expression->as.unop->right);
				if (value->type == BOOL_TYPE) {
					value->as.b = 1 ? value->as.b == 0 : 0;
					return value;
				}
				fprintf(stderr, "Error: Unexpected type for Unary Operand !\n");
				free(value);
				return NULL;
			}
		}
		case BINOP_EXPR: {
			Value* left = calculateGlobalExpression(expression->as.binop->left);
			Value* right = calculateGlobalExpression(expression->as.binop->right);
			Value* newValue = addValues(expression->as.binop->type, left, right);
			if (newValue == NULL) {
				return NULL;
			}
			return newValue;
		}
		case VARIABLE_EXPR:
			fprintf(stderr, "Error: Unexpected Variable in Global Expression\n");
			return NULL;
		case VALUE_EXPR: {
			Value* copiedValue = calloc(1, sizeof(Value));
			memcpy(copiedValue, expression->as.value, sizeof(Value));
			return copiedValue;
		}
		default:
			fprintf(stderr, "Error: Unexpected Expression type in generate Expression\n");
			return 0;
	}
}

Value* addValues(BinOperationType binOpType, Value* left, Value* right) {
	if (left == NULL || right == NULL) {
		return NULL;
	}

	Value* res = calloc(1, sizeof(Value));

	if (res == NULL) return NULL;

	switch (binOpType) {
		case ADD_OP:
			switch (left->type) {
				case BOOL_TYPE:
					fprintf(stderr, "Error: Operand + not allowed for type boolean in addValues\n");
					return NULL;
				case LONG_TYPE:
					res->as.i_64 = left->as.i_64 + right->as.i_64;
					res->type = LONG_TYPE;
					break;
				case DOUBLE_TYPE:
					fprintf(stderr, "Error: Float type not implemented yet :(\n");
					return NULL;
				default:
					fprintf(stderr, "Error: Unregognized type in addValues\n");
					return NULL;
			}
			break;
		case SUB_OP:
			switch (left->type) {
				case BOOL_TYPE:
					fprintf(stderr, "Error: Operand - not allowed for type boolean in addValues\n");
					return NULL;
				case LONG_TYPE:
					res->as.i_64 = left->as.i_64 + right->as.i_64;
					res->type = LONG_TYPE;
					break;
				case DOUBLE_TYPE:
					fprintf(stderr, "Error: Float type not implemented yet :(\n");
					return NULL;
				default:
					fprintf(stderr, "Error: Unregognized type in addValues\n");
					return NULL;
			}
			break;
		case MUL_OP:
			switch (left->type) {
				case BOOL_TYPE:
					fprintf(stderr, "Error: Operand * not allowed for type boolean in addValues\n");
					return NULL;
				case LONG_TYPE:
					res->as.i_64 = left->as.i_64 * right->as.i_64;
					res->type = LONG_TYPE;
					break;
				case DOUBLE_TYPE:
					fprintf(stderr, "Error: Float type not implemented yet :(\n");
					return NULL;
				default:
					fprintf(stderr, "Error: Unregognized type in addValues\n");
					return NULL;
			}
			break;
		case DIV_OP:
			switch (left->type) {
				case BOOL_TYPE:
					fprintf(stderr, "Error: Operand / not allowed for type boolean in addValues\n");
					return NULL;
				case LONG_TYPE:
					res->as.i_64 = left->as.i_64 / right->as.i_64;
					res->type = LONG_TYPE;
					break;
				case DOUBLE_TYPE:
					fprintf(stderr, "Error: Float type not implemented yet :(\n");
					return NULL;
				default:
					fprintf(stderr, "Error: Unregognized type in addValues\n");
					return NULL;
			}
			break;
		case MOD_OP:
			fprintf(stderr, "Error: Operator Modulus not implemented yet\n");
			return NULL;
		case ST_OP:
			switch (left->type) {
				case BOOL_TYPE:
					fprintf(stderr, "Error: Operand < not allowed for type boolean in addValues\n");
					return NULL;
				case LONG_TYPE:
					res->as.i_64 = left->as.i_64 < right->as.i_64;
					res->type = BOOL_TYPE;
					break;
				case DOUBLE_TYPE:
					fprintf(stderr, "Error: Float type not implemented yet :(\n");
					return NULL;
				default:
					fprintf(stderr, "Error: Unregognized type in addValues\n");
					return NULL;
			}
			break;
		case STE_OP:
			switch (left->type) {
				case BOOL_TYPE:
					fprintf(stderr, "Error: Operand <= not allowed for type boolean in addValues\n");
					return NULL;
				case LONG_TYPE:
					res->as.i_64 = left->as.i_64 <= right->as.i_64;
					res->type = BOOL_TYPE;
					break;
				case DOUBLE_TYPE:
					fprintf(stderr, "Error: Float type not implemented yet :(\n");
					return NULL;
				default:
					fprintf(stderr, "Error: Unregognized type in addValues\n");
					return NULL;
			}
			break;
		case GT_OP:
			switch (left->type) {
				case BOOL_TYPE:
					fprintf(stderr, "Error: Operand > not allowed for type boolean in addValues\n");
					return NULL;
				case LONG_TYPE:
					res->as.i_64 = left->as.i_64 > right->as.i_64;
					res->type = BOOL_TYPE;
					break;
				case DOUBLE_TYPE:
					fprintf(stderr, "Error: Float type not implemented yet :(\n");
					return NULL;
				default:
					fprintf(stderr, "Error: Unregognized type in addValues\n");
					return NULL;
			}
			break;
		case GTE_OP:
			switch (left->type) {
				case BOOL_TYPE:
					fprintf(stderr, "Error: Operand >= not allowed for type boolean in addValues\n");
					return NULL;
				case LONG_TYPE:
					res->as.i_64 = left->as.i_64 >= right->as.i_64;
					res->type = BOOL_TYPE;
					break;
				case DOUBLE_TYPE:
					fprintf(stderr, "Error: Float type not implemented yet :(\n");
					return NULL;
				default:
					fprintf(stderr, "Error: Unregognized type in addValues\n");
					return NULL;
			}
			break;
		case EQ_OP:
			switch (left->type) {
				case BOOL_TYPE:
					fprintf(stderr, "Error: Operand == not allowed for type boolean in addValues\n");
					return NULL;
				case LONG_TYPE:
					res->as.i_64 = left->as.i_64 == right->as.i_64;
					res->type = BOOL_TYPE;
					break;
				case DOUBLE_TYPE:
					fprintf(stderr, "Error: Float type not implemented yet :(\n");
					return NULL;
				default:
					fprintf(stderr, "Error: Unregognized type in addValues\n");
					return NULL;
			}
			break;
		case NEQ_OP:
			switch (left->type) {
				case BOOL_TYPE:
					fprintf(stderr, "Error: Operand != not allowed for type boolean in addValues\n");
					return NULL;
				case LONG_TYPE:
					res->as.i_64 = left->as.i_64 != right->as.i_64;
					res->type = BOOL_TYPE;
					break;
				case DOUBLE_TYPE:
					fprintf(stderr, "Error: Float type not implemented yet :(\n");
					return NULL;
				default:
					fprintf(stderr, "Error: Unregognized type in addValues\n");
					return NULL;
			}
			break;
		default:
			fprintf(stderr, "Error: Encountered illegal Operand in addValues\n");
			return 0;
	}

	free(right);
	free(left);
	return res;
}

int generateStatement(Codegen* codegen, Statement* statement) {

	if (codegen == NULL || statement == NULL) {
		return 0;
	}
	
	switch (statement->type) {
		case EXPRESSION_STMT:
			return generateExpression(codegen, statement->as.expression);
		case BLOCK_STMT:
			return generateBlockStmt(codegen, statement->as.blockStmt);
		case WHILE_STMT:
			return generateWhileStmt(codegen, statement->as.whileStmt);
		case IF_STMT:
			return generateIfStmt(codegen, statement->as.ifStmt);
		case DECLARATION_STMT:
		default:
			fprintf(stderr, "Error: Unexpected Statement type in generateStatement\n");
			return 0;
	}
}

// needs to create a new scope at start and remove it at the end
// needs to handle correct increment/decrement of the global stack offset variable by e.g. remembering offset at start and resetting it at the end
int generateBlockStmt(Codegen* codegen, BlockStmt* blockStmt) { return 1; }
int generateWhileStmt(Codegen* codegen, WhileStmt* whileStmt) { return 1; }
int generateIfStmt(Codegen* codegen, IfStmt* ifStmt) { return 1; }

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
			fprintf(stderr, "Error: Unexpected Expression type in generate Expression\n");
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
			fprintf(stderr, "Error: Operator Modulus not implemented yet\n");
			return 0;
		case ST_OP:
		case STE_OP:
		case GT_OP:
		case GTE_OP:
		case EQ_OP:
		case NEQ_OP:
			if (!addToBuffer(codegen, "cmp rax, rdx\n")) return 0;
			switch (binOperation->type) {
				case ST_OP:
					if(!addToBuffer(codegen, "setl al\n")) return 0;
					break;
				case STE_OP:
					if(!addToBuffer(codegen, "setle al\n")) return 0;
					break;
				case GT_OP:
					if(!addToBuffer(codegen, "setg al\n")) return 0;
					break;
				case GTE_OP:
					if(!addToBuffer(codegen, "setge al\n")) return 0;
					break;
				case EQ_OP:
					if(!addToBuffer(codegen, "sete al\n")) return 0;
					break;
				case NEQ_OP:
					if(!addToBuffer(codegen, "setne al\n")) return 0;
					break;
				default:
					fprintf(stderr, "Error: Encountered illegal Operand in generateBinOperation\n");
					return 0;
			}
			return addToBuffer(codegen, "movzx rax, al\n");
		default:
			fprintf(stderr, "Error: Encountered illegal Operand generateBinOperation\n");
			return 0;
	}
}

int generateUnaryOperation(Codegen* codegen, UnaryOperation* unaryOperation) {
	if (codegen == NULL || unaryOperation == NULL) {
		return 0;
	}

	if (unaryOperation->type == NOT) {
		if (!generateExpression(codegen, unaryOperation->right)) return 0;
		return addToBuffer(codegen, "test rax, rax\nsetz al\nmovzx rax, al\n");
	}
	else if (unaryOperation->type == MINUS) {
		if (!generateExpression(codegen, unaryOperation->right)) return 0;
		return addToBuffer(codegen, "neg rax\n");
	}

	fprintf(stderr, "Error: Unexpected Unary Operation Operand encountered in generateUnaryOperation: %d\n", unaryOperation->type);
	return 0;
}

// if this assignment is a declaration+initialization, then increment some global offset variable by the variables inferred type size in bytes and add it to scopes at highest scope ofc
// if it is not a declaration then just grab the stack location of that variable from the scopes
// global stack offset must also be so that it gets decremented so that it matches the types length in byte
int generateAssignment(Codegen* codegen, Assignment* assignment) {
	if (codegen == NULL || assignment == NULL) {
		return 0;
	}

	if (!generateExpression(codegen, assignment->expression)) return 0;
	int variableOffset = getVariableOffset(codegen, assignment->variable->id);

	ValueType variableType = assignment->variable->type;
	int typeSize = getTypeSize(variableType);
	const char* reg = getRegisterFromSize(typeSize);

	// variable is not in any scope yet, this is a declaration
	if (!variableOffset) {
		int alignment = typeSize;
		int remainder = abs(codegen->stackOffset) % alignment;
		int padding = (remainder == 0) ? 0 : alignment - remainder;

		codegen->stackOffset -= (typeSize + padding);
		variableOffset = codegen->stackOffset;

		HashTable* currentScope = peekArray(codegen->scopes);
		int* varOffset = malloc(sizeof(int));
		if (varOffset == NULL) return -1;
		*varOffset = (int)variableOffset;

		if (!insertKeyPair(currentScope, assignment->variable->id, varOffset)) return 0;
	}

	char lastInstruction[64];
	snprintf(lastInstruction, sizeof(lastInstruction), "mov [rbp%d], %s\n", variableOffset, reg);

	return addToBuffer(codegen, lastInstruction);
}

int generateVariable(Codegen* codegen, Variable* variable) {
	if (codegen == NULL || variable == NULL) {
		return 0;
	}

	ValueType variableType = variable->type;

	if (variableType == UNKNOWN) {
		return 1;
	}

	int variableOffset = getVariableOffset(codegen, variable->id);
	int typeSize = getTypeSize(variableType);
	char instr[64];

	switch (typeSize) {
		case 1:
			snprintf(instr, sizeof(instr), "movzx rax, BYTE [rbp%d]\n", variableOffset);
			break;
		case 4:
			snprintf(instr, sizeof(instr), "mov eax, [rbp%d]\n", variableOffset);
			break;
		case 8:
			snprintf(instr, sizeof(instr), "mov rax, [rbp%d]\n", variableOffset);
			break;
		default:
			fprintf(stderr, "Error: Unsupported type size in generateVariable\n");
			return 0;
	}
	
	return addToBuffer(codegen, instr);
}

int generateValue(Codegen* codegen, Value* value) {
	if (codegen == NULL || value == NULL) {
		return 0;
	}
	char instr[64];
	switch (value->type) {
		case LONG_TYPE:
			snprintf(instr, sizeof(instr), "mov rax, %lld\n", value->as.i_64);
			return addToBuffer(codegen, instr);
		case DOUBLE_TYPE:
			fprintf(stderr, "Error: Did not implement float Types yet :(\n");
			return 0;
		case BOOL_TYPE:
			snprintf(instr, sizeof(instr), "mov al, %d\n", value->as.b);
			return addToBuffer(codegen, instr);
		default:
			fprintf(stderr, "Error: Unregognized Value Type in generateValue\n");
			return 0;
	}
}

int getVariableOffset(Codegen* codegen, char* id) {
	if (codegen == NULL || id == NULL) {
		return 0;
	}

	for (int i = 0; i < codegen->scopes->size; i++) {
		if (containsKey(codegen->scopes->array[i], id)) {
			void* value = getValue((HashTable*)codegen->scopes->array[i], id);
			if (value == NULL) return 0;
			return (ValueType)*(int*)value;
		}
	}

	return 0;
}

HashTable* getScopeForVar(Codegen* codegen, char* id) {
	if (codegen == NULL || id == NULL) {
		return NULL;
	}

	for (int i = 0; i < codegen->scopes->size; i++) {
		if (containsKey(codegen->scopes->array[i], id)) {
			return (HashTable*)codegen->scopes->array[i];
		}
	}

	return NULL;
}

int getTypeSize(ValueType type) {
	switch(type) {
		case LONG_TYPE:
			return 8;
		case DOUBLE_TYPE:
			fprintf(stderr, "Error: Did not implement float Types yet :(\n");
			return 0;
		case BOOL_TYPE:
			return 1;
		default:
			fprintf(stderr, "Error: Unrecognized Type, cannot get type Offset\n");
			return 0;
			
	}
}

const char* getRegisterFromSize(int typeSize) {
	switch (typeSize) {
		case 1:
			return "al";
		case 2:
			return "ax";
		case 4:
			return "eax";
		case 8:
			return "rax";
		default:
			fprintf(stderr, "Error: Encountered invalid typeSize in getRegisterFromSize: %d\n", typeSize);
			return NULL;
	}
}

void freeCodegen(Codegen* codegen) {
	if (codegen == NULL) {
		return;
	}
	freeArray(codegen->scopes);
	free(codegen->buffer);
	free(codegen);
}
