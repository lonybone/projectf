#include "codegen.h"
#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int generate(Codegen* codegen);
int generateGlobalStatement(Codegen* codegen, Statement* statement);
int generateGlobalAssignment(Codegen* codegen, Assignment* assignment);
Value* calculateGlobalExpression(Expression* expression);
Value* addValues(BinOperationType binOpType, Value* left, Value* right);
int generateStatement(Codegen* codegen, Statement* statement);
int generateFunctionStatement(Codegen* codegen, FunctionStmt* function);
int annotateCall(Codegen* codegen, Expression* expression);
int generateExpression(Codegen* codegen, Expression* expression);
int generateFunctionCall(Codegen* codegen, FunctionCall* function, int hasCall);
int generateBinOperation(Codegen* codegen, BinOperation* binOperation);
int generateUnaryOperation(Codegen* codegen, UnaryOperation* unaryOperationt);
int generateBlockStmt(Codegen* codegen, BlockStmt* blockStmt, HashTable* scope, DynamicArray* params, int paramC);
int generateWhileStmt(Codegen* codegen, WhileStmt* whileStmt);
int generateIfStmt(Codegen* codegen, IfStmt* ifStmt);
int generateReturnStmt(Codegen* codegen, ReturnStmt* returnStmt);
int generateAssignment(Codegen* codegen, Assignment* assignment);
int getVariableOffset(Codegen* codegen, char* id);
HashTable* getScopeForVar(Codegen* codegen, char* id);
int getTypeSize(ValueType type);
const char* getFunctionArgRegister(int reg, int typeSize);
const char* getCalleeSavedRegister(int reg, int typeSize);

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

	HashTable* globalScope = hashTable(256, NULL);

	if (globalScope == NULL) {
		freeCodegen(codegen);
		return NULL;
	}

	if (!pushItem(codegen->scopes, globalScope)) {
		freeCodegen(codegen);
		return NULL;
	}

	codegen->labelCounters = dynamicArray(2, free);

	if (codegen->labelCounters == NULL) {
		freeCodegen(codegen);
		return NULL;
	}

	int* globalCounter = malloc(sizeof(int));

	if (globalCounter == NULL) {
		freeCodegen(codegen);
		return NULL;
	}

	*globalCounter = 0;

	if (!pushItem(codegen->labelCounters, globalCounter)) {
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

	if (codegen->idx + token_len + 1 > codegen->maxSize) {

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

	if (!addToBuffer(codegen, "extern printf\n\n\tsection .data\n\tmessage db \"Result was: %d\", 10, 0\n\n")) return 0;

	for (int i = 0; i < codegen->ast->size; i++) {

		if (!generateStatement(codegen, (Statement*)codegen->ast->array[i])) {

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

void insertionSort(DynamicArray* arr) {
    int i;
    int j;
    Variable* key;
    for (i = 1; i < arr->size; i++) {
        key = (Variable*)arr->array[i];
        j = i - 1;
        while (j >= 0 && getTypeSize(((Variable*)arr->array[j])->type) < getTypeSize(key->type)) {
            arr->array[j + 1] = arr->array[j];
            j = j - 1;
        }
        arr->array[j + 1] = key;
    }
}

int generateGlobalStatement(Codegen* codegen, Statement* statement) {
	if (codegen == NULL || statement == NULL) {
		return 0;
	}

	if (statement->type == FUNCTION_STMT) {
		return generateFunctionStatement(codegen, statement->as.function);
	}

	if (statement->type != EXPRESSION_STMT || statement->as.expression->type != ASSIGN_EXPR) {
		fprintf(stderr, "Error: Encountered ILLEGAL statement in Global Data Section\n");
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
	if (!insertKeyPair((HashTable*)codegen->scopes->array[0], variableId, NULL)) {
		fprintf(stderr, "Error: Tried to redifine global variable \"%s\"", variableId);
		return 0;
	}

	switch (type) {
		char instr[256];
		case BOOL_TYPE: {
			bool val = value->as.b;
			snprintf(instr, sizeof(instr), "\tsection .data\n\tglobal %s\n\talign 1\n\t\n%s:\n\tdb %d\n\n", variableId, variableId, val);
			free(value);
			return addToBuffer(codegen, instr);
		}
		case LONG_TYPE: {
			long long val = value->as.i_64;
			snprintf(instr, sizeof(instr), "\tsection .data\n\tglobal %s\n\talign 4\n\t\n%s:\n\tdd %lld\n\n", variableId, variableId, val);
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

	if (codegen->scopes->size == 1) {
		return generateGlobalStatement(codegen, statement);
	}

	switch (statement->type) {
		case EXPRESSION_STMT:
			annotateCall(codegen, statement->as.expression);
			return generateExpression(codegen, statement->as.expression);
		case FUNCTION_STMT:
			for (int i = 0; i < statement->as.function->params->size; i++) {
				annotateCall(codegen, statement->as.function->params->array[i]);
			}
			return generateFunctionStatement(codegen, statement->as.function);
		case BLOCK_STMT:
			return generateBlockStmt(codegen, statement->as.blockStmt, NULL, NULL, 0);
		case WHILE_STMT:
			annotateCall(codegen, statement->as.whileStmt->condition);
			return generateWhileStmt(codegen, statement->as.whileStmt);
		case IF_STMT:
			annotateCall(codegen, statement->as.ifStmt->condition);
			return generateIfStmt(codegen, statement->as.ifStmt);
		case RETURN_STMT:
			annotateCall(codegen, statement->as.returnStmt->expression);
			return generateReturnStmt(codegen, statement->as.returnStmt);
		case DECLARATION_STMT:
		default:
			fprintf(stderr, "Error: Unexpected Statement type %d in generateStatement\n", statement->type);
			return 0;
	}
}

int generateFunctionStatement(Codegen* codegen, FunctionStmt* function) {
	if (codegen == NULL || function == NULL) {
		return 0;
	}

	codegen->currentFunction = function;

	HashTable* functionScope = hashTable(256, free);
	if (functionScope == NULL) {
		return 0;
	}

	DynamicArray* params = dynamicArray(2, NULL);
	if (params == NULL) {
		freeTable(functionScope);
		return 0;
	}

	int* labelCounter = malloc(sizeof(int));
	if (labelCounter == NULL) {
		freeTable(functionScope);
		freeArray(params);
		return 0;
	}
	*labelCounter = 0;

	if (!pushItem(codegen->labelCounters, labelCounter)) {
		free(labelCounter);
		freeTable(functionScope);
		freeArray(params);
		return 0;
	}

	// for now only i32s and 6 params allowed
	if (function->params->size > 6) {
		free(popItem(codegen->labelCounters));
		freeTable(functionScope);
		freeArray(params);
		fprintf(stderr, "Error: Only 6 Function Prameters allowed for now\n");
		return 0;
	}

	// add Params
	for (int i = 0; i < function->params->size; i++) {
		Variable* param = (Variable*)function->params->array[i];
		if (!insertKeyPair(functionScope, param->id, NULL)) {
			free(popItem(codegen->labelCounters));
			freeTable(functionScope);
			freeArray(params);
			return 0;
		}

		if (!pushItem(params, param)) {
			free(popItem(codegen->labelCounters));
			freeTable(functionScope);
			freeArray(params);
			return 0;
		}
	}

	function->toEmit = dynamicArray(16, free);
	if (function->toEmit == NULL) {
		free(popItem(codegen->labelCounters));
		freeTable(functionScope);
		return 0;
	}

	
	if (!generateBlockStmt(codegen, function->blockStmt, functionScope, params, function->params->size)) {
		free(popItem(codegen->labelCounters));
		freeArray(params);
		return 0;
	}
	freeArray(params);

	char instr[256];
	int stackAllocationSize = function->maxStack;

	if (function->maxCalleeSaved >= 0 && (function->maxCalleeSaved) % 2 == 0) {
	    stackAllocationSize += 8;
	}

	stackAllocationSize = (stackAllocationSize + 7) & ~7;

	// function prologue
	snprintf(instr, sizeof(instr), "; Start of Function \"%s\"\nsection .text\nglobal %s\n%s:\npush rbp\nmov rbp, rsp\n", function->id, function->id, function->id);
	if (!addToBuffer(codegen, instr)) {
		free(popItem(codegen->labelCounters));
		return 0;
	}
	
	// allocate space for local variables and align stack
	snprintf(instr, sizeof(instr), "sub rsp, %d\n", stackAllocationSize);
	if (!addToBuffer(codegen, instr)) {
		free(popItem(codegen->labelCounters));
		return 0;
	}

	// push callee saved registers
	if (function->maxCalleeSaved >= 0) {
		for (int i = 0; i <= function->maxCalleeSaved; i++) {
			const char* reg = getCalleeSavedRegister(i, 8);
			snprintf(instr, sizeof(instr), "push %s\n", reg);
			if (!addToBuffer(codegen, instr)) {
				free(popItem(codegen->labelCounters));
				return 0;
			}
		}
	}

	// emitting the function body
	for (int i = 0; i < function->toEmit->size; i++) {
		if (!addToBuffer(codegen, (char*)function->toEmit->array[i])) {
			free(popItem(codegen->labelCounters));
			return 0;
		}
	}

	// function epilogue
	const char* printEpilogue = (strcmp(function->id, "main") == 0) ? "\tmov rdi, message\n\tmov esi, eax\n\tmov rax, 0\n\tcall printf\n\tmov rax, 0\n" : "";
	if (!addToBuffer(codegen, printEpilogue)) {
		free(popItem(codegen->labelCounters));
		return 0;
	}

	snprintf(instr, sizeof(instr), "%s_return:\n", function->id);
	if (!addToBuffer(codegen, instr)) {
		free(popItem(codegen->labelCounters));
		return 0;
	}

	// pop callee-saved registers in reverse order
	if (function->maxCalleeSaved >= 0) {
		for (int i = function->maxCalleeSaved; i >= 0; i--) {
			const char* reg = getCalleeSavedRegister(i, 8);
			snprintf(instr, sizeof(instr), "pop %s\n", reg);
			if (!addToBuffer(codegen, instr)) {
				free(popItem(codegen->labelCounters));
				return 0;
			}
		}
	}

	// deallocate local variable space
	snprintf(instr, sizeof(instr), "add rsp, %d\n", stackAllocationSize);
	if (!addToBuffer(codegen, instr)) {
		free(popItem(codegen->labelCounters));
		return 0;
	}

	// restore base pointer and return
	snprintf(instr, sizeof(instr), "leave\nret\n; End of Function \"%s\"\n\n", function->id);
	if (!addToBuffer(codegen, instr)) return 0;

	free(popItem(codegen->labelCounters));

	codegen->currentFunction = NULL;
	return 1;
}

// needs to create a new scope at start and remove it at the end
// needs to handle correct increment/decrement of the global stack offset variable by e.g. remembering offset at start and resetting it at the end
int generateBlockStmt(Codegen* codegen, BlockStmt* blockStmt, HashTable* scope, DynamicArray* params, int paramC) {
	if (codegen == NULL || blockStmt == NULL) {
		return 0;
	}

	HashTable* blockScope;
	if (scope) {
		blockScope = scope;
	}
	else {
		blockScope = hashTable(256, free);
	}

	if (blockScope == NULL) {
		return 0;
	}

	if (!pushItem(codegen->scopes, blockScope)) {
		freeTable(blockScope);
		return 0;
	}

	DynamicArray* vars = dynamicArray(2, NULL);

	if (params) {
		for (int i = 0; i < params->size; i++) {
			if (!pushItem(vars, params->array[i])) {
				freeTable(scope);
				freeArray(vars);
				return 0;
			}
		}
	}

	if (vars == NULL) {
		freeTable(blockScope);
		return 0;
	}

	for (int i = 0; i < blockStmt->stmts->size; i++) {
		Statement* statement = ((Statement*)blockStmt->stmts->array[i]);

		if (statement->type == EXPRESSION_STMT && statement->as.expression->type == ASSIGN_EXPR) {
			Variable* variable = ((Variable*)(statement->as.expression->as.assignment->variable));
			HashTable* varScope = getScopeForVar(codegen, variable->id);
			if (containsKey(blockScope, variable->id) || varScope == codegen->scopes->array[i] || getVariableOffset(codegen, variable->id) != 0) {
				continue;
			}
			if (!insertKeyPair(blockScope, variable->id, NULL)) {
				freeArray(vars);
				freeTable(blockScope);
				return 0;
			}
			if (!pushItem(vars, variable)) {
				freeArray(vars);
				freeTable(blockScope);
				return 0;
			}
		}
	}

	insertionSort(vars);
	int currentMaxStackSize = codegen->currentFunction->maxStack;

	for (int i = 0; i < vars->size; i++) {
		Variable* variable = ((Variable*)(vars->array[i]));

		currentMaxStackSize += getTypeSize(variable->type);
		int* varOffset = malloc(sizeof(int));
		if (varOffset == NULL) {
			freeArray(vars);
			freeTable(blockScope);
			return 0;
		}
		*varOffset = -(int)currentMaxStackSize;
		if (!updateKeyPair(blockScope, variable->id, varOffset)) {
			free(varOffset);
			freeArray(vars);
			freeTable(blockScope);
			return 0;
		}
	}

	// push params on the stack
	if (params != NULL) {
		for (int i = 0; i < paramC; i++) {
			Variable* param = ((Variable*)params->array[i]);
			const char* reg = getFunctionArgRegister(i+1, getTypeSize(param->type));
			int paramOffset = *(int*)getValue(blockScope, param->id);
			char instr[64];
			snprintf(instr, sizeof(instr), "mov [rbp%+d], %s\n", paramOffset, reg);
			if (!pushItem(codegen->currentFunction->toEmit, strdup(instr))) {
				freeArray(vars);
				freeTable((HashTable*)popItem(codegen->scopes));
				return 0;
			}
		}
	}

	freeArray(vars);
	currentMaxStackSize = (currentMaxStackSize + 15) & ~15;
	codegen->currentFunction->maxStack = currentMaxStackSize;

	int maxDiff = 0;
	for (int i = 0; i < blockStmt->stmts->size; i++) {
		Statement* statement = (Statement*)blockStmt->stmts->array[i];

		if (!generateStatement(codegen, statement)) {
			freeTable((HashTable*)popItem(codegen->scopes));
			return 0;
		}

		// we encountered another blockStmt that needed variable space
		if (codegen->currentFunction->maxStack != currentMaxStackSize) {
			int currentDiff = codegen->currentFunction->maxStack - currentMaxStackSize;
			// new block is a new max
			if (currentDiff > maxDiff) {
				maxDiff = currentDiff;
			}
			codegen->currentFunction->maxStack = currentMaxStackSize;
		}
	}

	codegen->currentFunction->maxStack = currentMaxStackSize + maxDiff;

	freeTable((HashTable*)popItem(codegen->scopes));
	return 1;
}

int generateWhileStmt(Codegen* codegen, WhileStmt* whileStmt) {
	if (codegen == NULL || whileStmt == NULL) {
		return 0;
	}

	int* counterPtr = (int*)peekArray(codegen->labelCounters);
	int labelCounter = *counterPtr;
	(*counterPtr)++;
	char instr[64];
	const char* functionID = codegen->currentFunction->id;

	snprintf(instr, sizeof(instr), "%s_start_while_%d:\n", functionID, labelCounter);
	if (!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;

	if (!generateExpression(codegen, whileStmt->condition)) return 0;

	const char* testReg;
	if (whileStmt->condition->hasCall) {
		testReg = "rbx";
	}
	else {
		testReg = "rax";
	}

	snprintf(instr, sizeof(instr), "\ttest %s, %s\n\tjz %s_end_while_%d\n", testReg, testReg, functionID, labelCounter);
	if (!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;

	if (!generateBlockStmt(codegen, whileStmt->body, NULL, NULL, 0)) return 0;

	snprintf(instr, sizeof(instr), "\tjmp %s_start_while_%d\n", functionID, labelCounter);
	if (!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;

	snprintf(instr, sizeof(instr), "%s_end_while_%d:\n", functionID, labelCounter);
	if (!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;

	return 1;
}

int generateIfStmt(Codegen* codegen, IfStmt* ifStmt) {
	if (codegen == NULL || ifStmt == NULL) {
		return 0;
	}

	int* counterPtr = (int*)peekArray(codegen->labelCounters);
	int labelCounter = *counterPtr;
	(*counterPtr)++;
	char instr[64];
	const char* functionId = codegen->currentFunction->id;
	const char* testReg;
	if (ifStmt->condition->hasCall) {
		testReg = "rbx";
	}
	else {
		testReg = "rax";
	}

	if (!generateExpression(codegen, ifStmt->condition)) return 0;

	if (ifStmt->type == ONLYIF) {
		snprintf(instr, sizeof(instr), "\ttest %s, %s\n\tjz %s_end_if_%d\n", testReg, testReg, functionId, labelCounter);
		if (!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;

		if (!generateBlockStmt(codegen, ifStmt->trueBody, NULL, NULL, 0)) {
			return 0;
		}
		snprintf(instr, sizeof(instr), "%s_end_if_%d:\n", functionId, labelCounter);
		if (!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;

		return 1;
	}
	else {
		snprintf(instr, sizeof(instr), "\ttest %s, %s\n\tjz %s_else_%d\n", testReg, testReg, functionId, labelCounter);
		if (!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;

		int prevMaxStack = codegen->currentFunction->maxStack;
		if (!generateBlockStmt(codegen, ifStmt->trueBody, NULL, NULL, 0)) {
			return 0;
		}

		int trueBodyDiff = codegen->currentFunction->maxStack - prevMaxStack;

		snprintf(instr, sizeof(instr), "\tjmp %s_end_if_%d\n", functionId, labelCounter);
		if (!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;
		snprintf(instr, sizeof(instr), "%s_else_%d:\n", functionId, labelCounter);
		if (!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;

		codegen->currentFunction->maxStack = prevMaxStack;
		if (ifStmt->type == IF_ELSE) {
			if (!generateBlockStmt(codegen, ifStmt->as.ifElse, NULL, NULL, 0)) return 0;
		} else {
			if (!generateIfStmt(codegen, ifStmt->as.ifElseIf)) return 0;
		}

		int blockDiff = codegen->currentFunction->maxStack - prevMaxStack;

		if (blockDiff < trueBodyDiff) {
			codegen->currentFunction->maxStack = prevMaxStack + trueBodyDiff;
		}

		snprintf(instr, sizeof(instr), "%s_end_if_%d:\n", functionId, labelCounter);
		if (!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;

		return 1;
	}
}

int generateReturnStmt(Codegen* codegen, ReturnStmt* returnStmt) {
	if (codegen == NULL || returnStmt == NULL) {
		return 0;
	}

	const char* functionId = codegen->currentFunction->id;
	if (!generateExpression(codegen, returnStmt->expression)) return 0;
	if (strcmp(functionId, "main") == 0) {
		return 1;
	}
	char instr[64];
	int typeSize = getTypeSize(returnStmt->expression->valueType);
	const char* src;
	const char* dst = getFunctionArgRegister(0, typeSize);
	if (returnStmt->expression->hasCall) {
		src = getCalleeSavedRegister(0, typeSize);
		snprintf(instr, sizeof(instr), "\tmov %s, %s\n", dst, src);
		if (!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;
	}
	else {
		src = getFunctionArgRegister(0, typeSize);
	}
	snprintf(instr, sizeof(instr), "\tjmp %s_return\n", functionId);
	return pushItem(codegen->currentFunction->toEmit, strdup(instr));
}

static bool findCallsRecursive(Expression* expression);
static void propagateCallsDownRecursive(Expression* expression, bool parentHasCall);

int annotateCall(Codegen* codegen, Expression* expression) {
	if (codegen == NULL || expression == NULL) {
		return 0;
	}

	findCallsRecursive(expression);
	propagateCallsDownRecursive(expression, false);
	return expression->hasCall;
}

static bool findCallsRecursive(Expression* expression) {
	if (expression == NULL) {
		return false;
	}

	bool foundCall = false;
	switch (expression->type) {
		case FUNCTIONCALL_EXPR:
			foundCall = true;
			break;
		case EXPR_WRAPPER_EXPR:
			foundCall = findCallsRecursive(expression->as.expWrap);
			break;
		case ASSIGN_EXPR:
			foundCall = findCallsRecursive(expression->as.assignment->expression);
			break;
		case UNARY_EXPR:
			foundCall = findCallsRecursive(expression->as.unop->right);
			break;
		case BINOP_EXPR: {
			bool leftHasCall = findCallsRecursive(expression->as.binop->left);
			bool rightHasCall = findCallsRecursive(expression->as.binop->right);
			foundCall = leftHasCall || rightHasCall;
			break;
		}
		case VARIABLE_EXPR:
		case VALUE_EXPR:
			foundCall = false;
			break;
		default:
			foundCall = false;
			break;
	}

	expression->hasCall = foundCall;
	return foundCall;
}

static void propagateCallsDownRecursive(Expression* expression, bool parentHasCall) {
	if (expression == NULL) {
		return;
	}

	expression->hasCall = parentHasCall || expression->hasCall;

	switch (expression->type) {
		case FUNCTIONCALL_EXPR:
			for (int i = 0; i < expression->as.functionCall->params->size; i++) {
				propagateCallsDownRecursive(expression->as.functionCall->params->array[i], expression->hasCall);
			}
			break;
		case EXPR_WRAPPER_EXPR:
			propagateCallsDownRecursive(expression->as.expWrap, expression->hasCall);
			break;
		case ASSIGN_EXPR:
			propagateCallsDownRecursive(expression->as.assignment->expression, expression->hasCall);
			break;
		case UNARY_EXPR:
			propagateCallsDownRecursive(expression->as.unop->right, expression->hasCall);
			break;
		case BINOP_EXPR:
			propagateCallsDownRecursive(expression->as.binop->left, expression->hasCall);
			propagateCallsDownRecursive(expression->as.binop->right, expression->hasCall);
			break;
		case VARIABLE_EXPR:
		case VALUE_EXPR:
			break;
	}
}
	

int generateExpression(Codegen* codegen, Expression* expression) {
	if (codegen == NULL || expression == NULL) {
		return 0;
	}

	switch (expression->type) {
		case EXPR_WRAPPER_EXPR:
			return generateExpression(codegen, expression->as.expWrap);
		case FUNCTIONCALL_EXPR:
			return generateFunctionCall(codegen, expression->as.functionCall, expression->hasCall);
		case ASSIGN_EXPR:
			return generateAssignment(codegen, expression->as.assignment);
		case UNARY_EXPR:
			return generateUnaryOperation(codegen, expression->as.unop);
		case BINOP_EXPR:
			return generateBinOperation(codegen, expression->as.binop);
		case VARIABLE_EXPR: {
			char instr[128];
			const char* destReg;
			int typeSize = getTypeSize(expression->as.variable->type);

			if (expression->hasCall) {
				destReg = getCalleeSavedRegister(codegen->currentFunction->calleeSaved, typeSize);
			}
			else {
				destReg = getFunctionArgRegister(codegen->currentFunction->callerSaved, typeSize);
			}

			char varLocation[64];
			HashTable* varScope = getScopeForVar(codegen, expression->as.variable->id);
			if ((HashTable*)codegen->scopes->array[0] == varScope) {
				snprintf(varLocation, sizeof(varLocation), "[%s]", expression->as.variable->id);
			}
			else {
				int variableOffset = getVariableOffset(codegen, expression->as.variable->id);
				snprintf(varLocation, sizeof(varLocation), "[rbp%d]", variableOffset);
			}

			snprintf(instr, sizeof(instr), "\tmov %s, %s\n", destReg, varLocation);
			return pushItem(codegen->currentFunction->toEmit, strdup(instr));
		}
		case VALUE_EXPR: {
			char instr[128];
			switch (expression->as.value->type) {
				case LONG_TYPE: {
					const char* destReg;
					int typeSize = getTypeSize(LONG_TYPE);

					if (expression->hasCall) {
						destReg = getCalleeSavedRegister(codegen->currentFunction->calleeSaved, typeSize);
					}
					else {
						destReg = getFunctionArgRegister(codegen->currentFunction->callerSaved, typeSize);
					}

					snprintf(instr, sizeof(instr), "\tmov %s, %ld\n", destReg, expression->as.value->as.i_32);
					return pushItem(codegen->currentFunction->toEmit, strdup(instr));
				}
				case DOUBLE_TYPE:
					fprintf(stderr, "Error: Did not implement float Types yet :(\n");
					return 0;
				case BOOL_TYPE: {
					const char* destReg;

					if (expression->hasCall) {
						destReg = getCalleeSavedRegister(codegen->currentFunction->calleeSaved, 8);
					}
					else {
						destReg = getFunctionArgRegister(codegen->currentFunction->callerSaved, 8);
					}

					snprintf(instr, sizeof(instr), "\tmov %s, %d\n", destReg, expression->as.value->as.b);
					return pushItem(codegen->currentFunction->toEmit, strdup(instr));
				}
				default:
					fprintf(stderr, "Error: Unregognized Value Type in generateValue\n");
					return 0;
			}
		}
		default:
			fprintf(stderr, "Error: Unexpected Expression type in generate Expression\n");
			return 0;
	}
}

int generateFunctionCall(Codegen* codegen, FunctionCall* function, int hasCall) {
	if (codegen == NULL || function == NULL) {
		return 0;
	}

	char instr[64];

	for (int i = 0; i < function->params->size; i++) {
		findCallsRecursive(function->params->array[i]);
	}

	// calculate and store args
	for (int i = 0; i < function->params->size; i++) {
		Expression* arg = (Expression*)function->params->array[i];

		if (codegen->currentFunction->calleeSaved+1 > codegen->currentFunction->maxCalleeSaved) {
			if (codegen->currentFunction->calleeSaved > 4) {
				fprintf(stderr, "Error: ran out of Registers when trying to evaluate Function Arguments\n");
				return 0;
			}
			codegen->currentFunction->maxCalleeSaved = codegen->currentFunction->calleeSaved;
		}

		if (!generateExpression(codegen, arg)) return 0;

		int argSize = getTypeSize(arg->valueType);
		const char* opcode = (argSize == 1 || argSize == 2) ? "movzx" : "mov";

		const char* dstReg = getCalleeSavedRegister(codegen->currentFunction->calleeSaved, (argSize >= 4) ? argSize : 8);
		const char* srcReg;
		
		if (arg->hasCall) {
			srcReg = getCalleeSavedRegister(codegen->currentFunction->calleeSaved, argSize);
		}
		else {
			srcReg = getFunctionArgRegister(codegen->currentFunction->callerSaved, argSize);
		}

		snprintf(instr, sizeof(instr), "\t%s %s, %s\n", opcode, dstReg, srcReg);
		codegen->currentFunction->calleeSaved++;
		if (!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;
	}

	// put args into correct registers
	for (int i = 0; i < function->params->size; i++) {
		Expression* arg = (Expression*)function->params->array[i];
		int argSize = getTypeSize(arg->valueType);
		const char* argSrc;
		const char* argDst;
		
		argSrc = getCalleeSavedRegister(codegen->currentFunction->calleeSaved-(function->params->size-i), argSize);

		argDst = getFunctionArgRegister(i+1, argSize);
		snprintf(instr, sizeof(instr), "\tmov %s, %s\n", argDst, argSrc);
		if (!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;
	}

	int retTypeSize = getTypeSize(function->returnType);
	const char* raxReg = getFunctionArgRegister(0, retTypeSize);
	const char* dstReg;

	if (hasCall) {
		dstReg = getCalleeSavedRegister(codegen->currentFunction->calleeSaved-function->params->size, retTypeSize);
	}
	else {
		dstReg = getFunctionArgRegister(codegen->currentFunction->callerSaved, retTypeSize);
	}

	snprintf(instr, sizeof(instr), "\tcall %s\n\tmov %s, %s\n", function->id, dstReg, raxReg);
	codegen->currentFunction->calleeSaved -= function->params->size;
	return pushItem(codegen->currentFunction->toEmit, strdup(instr));
}

int generateBinOperation(Codegen* codegen, BinOperation* binOperation) {
	if (codegen == NULL || binOperation == NULL) {
		return 0;
	}

	const char* leftReg;
	const char* rightReg;
	int typeSize = getTypeSize(binOperation->left->valueType);


	int hasCall = binOperation->left->hasCall;

	if (hasCall) {
		leftReg = getCalleeSavedRegister(codegen->currentFunction->calleeSaved, typeSize);

		if (codegen->currentFunction->calleeSaved + 1 > codegen->currentFunction->maxCalleeSaved) {
			if (codegen->currentFunction->calleeSaved > 4) {
				fprintf(stderr, "Error: Ran out of Callee-Saved Registers when evaluating Expression\n");
				return 0;
			}
			codegen->currentFunction->maxCalleeSaved = codegen->currentFunction->calleeSaved;
		}
		rightReg = getCalleeSavedRegister(codegen->currentFunction->calleeSaved + 1, typeSize);

		if (!generateExpression(codegen, binOperation->left)) return 0;
		codegen->currentFunction->calleeSaved++;
		if (!generateExpression(codegen, binOperation->right)) return 0;
		codegen->currentFunction->calleeSaved--;
	}
	else {
		// dont use rdx for now, as it may be used in integer division as special register for overflow, also for mod operations later
		int originalCallerSaved = codegen->currentFunction->callerSaved;

		int leftIdx = originalCallerSaved;
		if (leftIdx == 3) {
			leftIdx++;
		}
		
		int rightIdx = leftIdx + 1;
		if (rightIdx == 3) {
			rightIdx++;
		}
		
		if (rightIdx > codegen->currentFunction->maxCallerSaved) {
			if (rightIdx > 6) {
				fprintf(stderr, "Error: Ran out of Caller-Saved Registers when evaluating Expression\n");
				return 0;
			}
			codegen->currentFunction->maxCallerSaved = rightIdx;
		}
		
		leftReg = getFunctionArgRegister(leftIdx, typeSize);
		rightReg = getFunctionArgRegister(rightIdx, typeSize);
		
		codegen->currentFunction->callerSaved = leftIdx;
		if (!generateExpression(codegen, binOperation->left)) codegen->currentFunction->callerSaved = originalCallerSaved;
		
		codegen->currentFunction->callerSaved = rightIdx;
		if (!generateExpression(codegen, binOperation->right)) return 0;
		
		codegen->currentFunction->callerSaved = originalCallerSaved;
	}

	char instr[64];
	switch (binOperation->type) {
		case ADD_OP:
			snprintf(instr, sizeof(instr), "\tadd %s, %s\n", leftReg, rightReg);
			return pushItem(codegen->currentFunction->toEmit, strdup(instr));
		case SUB_OP:
			snprintf(instr, sizeof(instr), "\tsub %s, %s\n", leftReg, rightReg);
			return pushItem(codegen->currentFunction->toEmit, strdup(instr));
		case MUL_OP: {
			snprintf(instr, sizeof(instr), "\timul %s, %s\n", leftReg, rightReg);
			return pushItem(codegen->currentFunction->toEmit, strdup(instr));
		}
		case DIV_OP: {
			const char* raxReg = getFunctionArgRegister(0, typeSize);
			snprintf(instr, sizeof(instr), "\tmov %s, %s\n\tidiv %s\n", raxReg, rightReg, leftReg);
			return pushItem(codegen->currentFunction->toEmit, strdup(instr));
		}
		case MOD_OP:
			fprintf(stderr, "Error: Operator Modulus not implemented yet\n");
			return 0;
		case ST_OP:
		case STE_OP:
		case GT_OP:
		case GTE_OP:
		case EQ_OP:
		case NEQ_OP:
			snprintf(instr, sizeof(instr), "\tcmp %s, %s\n", leftReg, rightReg);
			if (!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;

			const char* destBoolReg;
			if (hasCall) {
				destBoolReg = getCalleeSavedRegister(codegen->currentFunction->calleeSaved, 1);
			}
			else {
				destBoolReg = getFunctionArgRegister(codegen->currentFunction->callerSaved, 1);
			}
			switch (binOperation->type) {
				case ST_OP:
					snprintf(instr, sizeof(instr), "\tsetl %s\n", destBoolReg);
					if(!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;
					break;
				case STE_OP:
					snprintf(instr, sizeof(instr), "\tsetle %s\n", destBoolReg);
					if(!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;
					break;
				case GT_OP:
					snprintf(instr, sizeof(instr), "\tsetg %s\n", destBoolReg);
					if(!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;
					break;
				case GTE_OP:
					snprintf(instr, sizeof(instr), "\tsetge %s\n", destBoolReg);
					if(!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;
					break;
				case EQ_OP:
					snprintf(instr, sizeof(instr), "\tsete %s\n", destBoolReg);
					if(!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;
					break;
				case NEQ_OP:
					snprintf(instr, sizeof(instr), "\tsetne %s\n", destBoolReg);
					if(!pushItem(codegen->currentFunction->toEmit, strdup(instr))) return 0;
					break;
				default:
					fprintf(stderr, "Error: Encountered illegal Operand in generateBinOperation\n");
					return 0;
			}

			const char* fullDestReg;
			if (hasCall) {
				fullDestReg = getCalleeSavedRegister(codegen->currentFunction->calleeSaved, 8);
			}
			else {
				fullDestReg = getFunctionArgRegister(codegen->currentFunction->callerSaved, 8);
			}

			snprintf(instr, sizeof(instr), "\tmovzx %s, %s\n", fullDestReg, destBoolReg);
			return pushItem(codegen->currentFunction->toEmit, strdup(instr));
		default:
			fprintf(stderr, "Error: Encountered illegal Operand generateBinOperation\n");
			return 0;
	}
}

int generateUnaryOperation(Codegen* codegen, UnaryOperation* unaryOperation) {
	if (codegen == NULL || unaryOperation == NULL) {
		return 0;
	}

	const char* testReg;

	char instr[128];

	if (unaryOperation->type == NOT) {
		const char* fullReg;
		const char* dstReg;
		if (unaryOperation->right->hasCall) {
			testReg = getCalleeSavedRegister(codegen->currentFunction->calleeSaved, 1);
			dstReg = getCalleeSavedRegister(codegen->currentFunction->calleeSaved, 1);
			fullReg = getCalleeSavedRegister(codegen->currentFunction->calleeSaved, 8);
		}
		else {
			testReg = getFunctionArgRegister(codegen->currentFunction->callerSaved, 1);
			dstReg = getFunctionArgRegister(codegen->currentFunction->callerSaved, 1);
			fullReg = getFunctionArgRegister(codegen->currentFunction->callerSaved, 8);
		}
		if (!generateExpression(codegen, unaryOperation->right)) return 0;
		snprintf(instr, sizeof(instr), "\tmovzx %s, %s\n\ttest %s, %s\n\tsetz %s\n", fullReg, dstReg, testReg, testReg, testReg);
		return pushItem(codegen->currentFunction->toEmit, strdup(instr));
	}
	else if (unaryOperation->type == MINUS) {
		int typeSize = getTypeSize(unaryOperation->right->valueType);

		if (unaryOperation->right->hasCall) {
			testReg = getCalleeSavedRegister(codegen->currentFunction->calleeSaved, typeSize);
		}
		else {
			testReg = getFunctionArgRegister(codegen->currentFunction->callerSaved, typeSize);
		}
		if (!generateExpression(codegen, unaryOperation->right)) return 0;
		snprintf(instr, sizeof(instr), "\tneg %s\n", testReg);
		return pushItem(codegen->currentFunction->toEmit, strdup(instr));
	}

	fprintf(stderr, "Error: Unexpected Unary Operation Operand encountered in generateUnaryOperation: %d\n", unaryOperation->type);
	return 0;
}

int generateAssignment(Codegen* codegen, Assignment* assignment) {
	if (codegen == NULL || assignment == NULL) {
		return 0;
	}

	if (!generateExpression(codegen, assignment->expression)) return 0;
	int variableOffset = getVariableOffset(codegen, assignment->variable->id);

	ValueType variableType = assignment->variable->type;
	int typeSize = getTypeSize(variableType);

	const char* reg;
	if (assignment->expression->hasCall) {
		reg = getCalleeSavedRegister(0, typeSize);
	}
	else {
		reg = getFunctionArgRegister(0, typeSize);
	}

	HashTable* varScope = getScopeForVar(codegen, assignment->variable->id);
	char varLocation[64];
	// global variable
	if ((HashTable*)codegen->scopes->array[0] == varScope) {
		snprintf(varLocation, sizeof(varLocation), "[%s]", assignment->variable->id);
	}
	else {
		snprintf(varLocation, sizeof(varLocation), "[rbp%d]", variableOffset);
	}

	char lastInstruction[128];
	snprintf(lastInstruction, sizeof(lastInstruction), "\tmov %s, %s\n", varLocation, reg);

	return pushItem(codegen->currentFunction->toEmit, strdup(lastInstruction));
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
			return 4;
		case DOUBLE_TYPE:
			fprintf(stderr, "Error: Did not implement float Types yet :(\n");
			return 0;
		case BOOL_TYPE:
			return 1;
		default:
			fprintf(stderr, "Error: Unrecognized Type %d, cannot get type Offset\n", type);
			return 0;
			
	}
}

const char* getFunctionArgRegister(int reg, int typeSize) {
	static const char* register_names[7][4] = {
		{"al",  "ax",  "eax", "rax"},
		{"dil", "di",  "edi", "rdi"},
		{"sil", "si",  "esi", "rsi"},
		{"dl",  "dx",  "edx", "rdx"},
		{"cl",  "cx",  "ecx", "rcx"},
		{"r8b", "r8w", "r8d", "r8" },
		{"r9b", "r9w", "r9d", "r9" }
	};

	if (reg > 6 || typeSize < 0) {
		return NULL;
	}

	int size_index;
	switch (typeSize) {
		case 1: size_index = 0; break;
		case 2: size_index = 1; break;
		case 4: size_index = 2; break;
		case 8: size_index = 3; break;
		default:
			return NULL;
	}

	return register_names[reg][size_index];
}

const char* getCalleeSavedRegister(int reg, int typeSize) {
	static const char* register_names[5][4] = {
		{"bl",  "bx",   "ebx",  "rbx"},
		{"r12b","r12w", "r12d", "r12"},
		{"r13b","r13w", "r13d", "r13"},
		{"r14b","r14w", "r14d", "r14"},
		{"r15b","r15w", "r15d", "r15"}
	};

	if (reg < 0 || reg > 4) {
		return NULL;
	}

	int size_index;
	switch (typeSize) {
		case 1: size_index = 0; break;
		case 2: size_index = 1; break;
		case 4: size_index = 2; break;
		case 8: size_index = 3; break;
		default:
			return NULL;
	}

	return register_names[reg][size_index];
}

void freeCodegen(Codegen* codegen) {
	if (codegen == NULL) {
		return;
	}
	freeArray(codegen->labelCounters);
	freeArray(codegen->scopes);
	free(codegen->buffer);
	free(codegen);
}
