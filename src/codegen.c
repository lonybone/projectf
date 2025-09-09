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
int generateStatement(Codegen* codegen, Statement* statement, DynamicArray* toEmit);
int generateFunctionStatement(Codegen* codegen, FunctionStmt* function);
int generateExpression(Codegen* codegen, Expression* expression, DynamicArray* toEmit);
int generateBinOperation(Codegen* codegen, BinOperation* binOperation, DynamicArray* toEmit);
int generateUnaryOperation(Codegen* codegen, UnaryOperation* unaryOperation, DynamicArray* toEmit);
int generateBlockStmt(Codegen* codegen, BlockStmt* blockStmt, HashTable* scope, DynamicArray* params, int paramC, int prevStackSize, DynamicArray* toEmit);
int generateWhileStmt(Codegen* codegen, WhileStmt* whileStmt, int prevStackSize, DynamicArray* toEmit);
int generateIfStmt(Codegen* codegen, IfStmt* ifStmt, int prevStackSize, DynamicArray* toEmit);
int generateReturnStmt(Codegen* codegen, ReturnStmt* returnStmt, DynamicArray* toEmit);
int generateAssignment(Codegen* codegen, Assignment* assignment, DynamicArray* toEmit);
int generateValue(Codegen* codegen, Value* value, DynamicArray* toEmit);
int generateVariable(Codegen* codegen, Variable* variable, DynamicArray* toEmit);
int getVariableOffset(Codegen* codegen, char* id);
HashTable* getScopeForVar(Codegen* codegen, char* id);
int getTypeSize(ValueType type);
const char* getRegister(int reg, int typeSize);

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

		if (!generateStatement(codegen, (Statement*)codegen->ast->array[i], NULL)) {

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
			snprintf(instr, sizeof(instr), "\tsection .data\n\tglobal %s\n\t.align 1\n\t.type %s, @object\n\t.size %s, 1\n%s:\n\tdb %d\n\n", variableId, variableId, variableId, variableId, val);
			free(value);
			return addToBuffer(codegen, instr);
		}
		case LONG_TYPE: {
			long long val = value->as.i_64;
			snprintf(instr, sizeof(instr), "\tsection .data\n\tglobal %s\n\t.align 4\n\t.type %s, @object\n\t.size %s, 4\n%s:\n\tdq %lld\n\n", variableId, variableId, variableId, variableId, val);
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

int generateStatement(Codegen* codegen, Statement* statement, DynamicArray* toEmit) {
	if (codegen == NULL || statement == NULL) {
		return 0;
	}

	if (codegen->scopes->size == 1) {
		return generateGlobalStatement(codegen, statement);
	}

	switch (statement->type) {
		case EXPRESSION_STMT:
			return generateExpression(codegen, statement->as.expression, toEmit);
		case FUNCTION_STMT:
			return generateFunctionStatement(codegen, statement->as.function);
		case BLOCK_STMT:
			return generateBlockStmt(codegen, statement->as.blockStmt, NULL, NULL, 0, 0, toEmit);
		case WHILE_STMT:
			return generateWhileStmt(codegen, statement->as.whileStmt, 0, toEmit);
		case IF_STMT:
			return generateIfStmt(codegen, statement->as.ifStmt, 0, toEmit);
		case RETURN_STMT:
			return generateReturnStmt(codegen, statement->as.returnStmt, toEmit);
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
			return -1;
		}

		if (!pushItem(params, param)) {
			free(popItem(codegen->labelCounters));
			freeTable(functionScope);
			freeArray(params);
			return -1;
		}
	}

	DynamicArray* toEmit = dynamicArray(16, free);
	if (toEmit == NULL) {
		free(popItem(codegen->labelCounters));
		freeTable(functionScope);
		freeArray(params);
		return 0;
	}

	int rawStackFrameSize = generateBlockStmt(codegen, function->blockStmt, functionScope, params, function->params->size, 0, toEmit);
	if (rawStackFrameSize == -1) {
		free(popItem(codegen->labelCounters));
		freeArray(toEmit);
		return 0;
	}

	int stackFrameSize = (rawStackFrameSize + 15) & ~15;
	if (stackFrameSize == 0) {
		stackFrameSize = 16;
	}

	// function prologue
	char prologue[256];
	snprintf(prologue, sizeof(prologue), "%s:\n\tpush ebp\n\tmov ebp, esp\n\tsub esp, %d\n", function->id, stackFrameSize);
	if (!addToBuffer(codegen, prologue)) {
		free(popItem(codegen->labelCounters));
		freeArray(toEmit);
		return 0;
	}

	// emitting the whole block
	for (int i = 0; i < toEmit->size; i++) {
		if (!addToBuffer(codegen, (char*)toEmit->array[i])) {
			free(popItem(codegen->labelCounters));
			freeArray(toEmit);
			return 0;
		}
	}

	freeArray(toEmit);
	free(popItem(codegen->labelCounters));

	// function epilogue
	char epilogue[256];
	snprintf(epilogue, sizeof(epilogue), "\tmov esp, ebp\n\tpop ebp\n\tadd esp, %d\n\tret", stackFrameSize);
	addToBuffer(codegen, epilogue);
	return 1;
}

// needs to create a new scope at start and remove it at the end
// needs to handle correct increment/decrement of the global stack offset variable by e.g. remembering offset at start and resetting it at the end
int generateBlockStmt(Codegen* codegen, BlockStmt* blockStmt, HashTable* scope, DynamicArray* params, int paramC, int prevStackSize, DynamicArray* toEmit) {
	if (codegen == NULL || blockStmt == NULL) {
		return -1;
	}

	HashTable* blockScope;
	if (scope) {
		blockScope = scope;
	}
	else {
		blockScope = hashTable(256, free);
	}

	if (blockScope == NULL) {
		return -1;
	}

	if (!pushItem(codegen->scopes, blockScope)) {
		freeTable(blockScope);
		return -1;
	}

	DynamicArray* vars;
	if (params) {
		vars = params;
	}
	else {
		vars = dynamicArray(2, NULL);
	}

	if (vars == NULL) {
		freeTable(blockScope);
		return -1;
	}

	for (int i = 0; i < blockStmt->stmts->size; i++) {
		Statement* statement = ((Statement*)blockStmt->stmts->array[i]);

		if (statement->type == EXPRESSION_STMT && statement->as.expression->type == ASSIGN_EXPR) {
			Variable* variable = ((Variable*)(statement->as.expression->as.assignment->variable));
			if (containsKey(blockScope, variable->id) || getVariableOffset(codegen, variable->id) != 0) {
				continue;
			}
			if (!insertKeyPair(blockScope, variable->id, NULL)) {
				freeArray(vars);
				freeTable(blockScope);
				return -1;
			}
			if (!pushItem(vars, variable)) {
				freeArray(vars);
				freeTable(blockScope);
				return -1;
			}
		}
	}

	insertionSort(vars);
	int currentMaxStackSize = prevStackSize;

	for (int i = 0; i < vars->size; i++) {
		Variable* variable = ((Variable*)(vars->array[i]));

		currentMaxStackSize += getTypeSize(variable->type);
		int* varOffset = malloc(sizeof(int));
		if (varOffset == NULL) {
			freeArray(vars);
			freeTable(blockScope);
			return -1;
		}
		*varOffset = -(int)currentMaxStackSize;
		if (!updateKeyPair(blockScope, variable->id, varOffset)) {
			free(varOffset);
			freeArray(vars);
			freeTable(blockScope);
			return -1;
		}
	}

	// push params on the stack
	if (params != NULL) {
		for (int i = 0; i < paramC; i++) {
			Variable* param = ((Variable*)params->array[i]);
			const char* reg = getRegister(i+1, getTypeSize(param->type));
			int paramOffset = *(int*)getValue(blockScope, param->id);
			char instr[64];
			snprintf(instr, sizeof(instr), "mov [rbp%+d], %s\n", paramOffset, reg);
			if (!pushItem(toEmit, strdup(instr))) {
				freeArray(vars);
				freeTable((HashTable*)popItem(codegen->scopes));
				return -1;
			}
		}
	}

	freeArray(vars);

	for (int i = 0; i < blockStmt->stmts->size; i++) {
		Statement* statement = (Statement*)blockStmt->stmts->array[i];

		int nesttingStackSize = currentMaxStackSize;
		switch (statement->type) {
			case BLOCK_STMT: {
				int tmp = generateBlockStmt(codegen, statement->as.blockStmt, NULL, NULL, 0, nesttingStackSize, toEmit);
				if (tmp == -1) {
					freeTable((HashTable*)popItem(codegen->scopes));
					return -1;
				}
				if (tmp > currentMaxStackSize) {
					currentMaxStackSize = tmp;
				}
				break;
			}
			case IF_STMT: {
				int tmp = generateIfStmt(codegen, statement->as.ifStmt, nesttingStackSize, toEmit);
				if (tmp == -1) {
					freeTable((HashTable*)popItem(codegen->scopes));
					return -1;
				}
				if (tmp > currentMaxStackSize) {
					currentMaxStackSize = tmp;
				}
				break;
			}
			case WHILE_STMT: {
				int tmp = generateWhileStmt(codegen, statement->as.whileStmt, nesttingStackSize, toEmit);
				if (tmp == -1) {
					freeTable((HashTable*)popItem(codegen->scopes));
					return -1;
				}
				if (tmp > currentMaxStackSize) {
					currentMaxStackSize = tmp;
				}
				break;
			}
			default:
				if (!generateStatement(codegen, statement, toEmit)) {
					freeTable((HashTable*)popItem(codegen->scopes));
					return -1;
				}
				break;
		}
	}

	freeTable((HashTable*)popItem(codegen->scopes));
	return currentMaxStackSize;
}

int generateWhileStmt(Codegen* codegen, WhileStmt* whileStmt, int prevStackSize, DynamicArray* toEmit) { return 0; }
int generateIfStmt(Codegen* codegen, IfStmt* ifStmt, int prevStackSize, DynamicArray* toEmit) {
	if (codegen == NULL || ifStmt == NULL) {
		return -1;
	}

	int* counterPtr = (int*)peekArray(codegen->labelCounters);
	int labelCounter = *counterPtr;
	(*counterPtr)++;
	char instr[64];
	prevStackSize = (prevStackSize + 15) & ~15;

	if (!generateExpression(codegen, ifStmt->condition, toEmit)) return -1;

	if (ifStmt->type == ONLYIF) {
		snprintf(instr, sizeof(instr), "\tjz .end_if_%d\n", labelCounter);
		if (!pushItem(toEmit, strdup(instr))) return -1;

		int trueBranchSize = generateBlockStmt(codegen, ifStmt->trueBody, NULL, NULL, 0, prevStackSize, toEmit);
		if (trueBranchSize == -1) return -1;
		snprintf(instr, sizeof(instr), "\t.end_if_%d:\n", labelCounter);
		if (!pushItem(toEmit, strdup(instr))) return -1;

		return trueBranchSize;
	}
	else {
		snprintf(instr, sizeof(instr), "\tjz .else_%d\n", labelCounter);
		if (!pushItem(toEmit, strdup(instr))) return -1;

		int trueBranchSize = generateBlockStmt(codegen, ifStmt->trueBody, NULL, NULL, 0, prevStackSize, toEmit);
		if (trueBranchSize == -1) return -1;

		snprintf(instr, sizeof(instr), "\tjmp .end_if_%d\n", labelCounter);
		if (!pushItem(toEmit, strdup(instr))) return -1;
		snprintf(instr, sizeof(instr), "\t.else_%d:\n", labelCounter);
		if (!pushItem(toEmit, strdup(instr))) return -1;

		int falseBranchSize = 0;
		if (ifStmt->type == IF_ELSE) {
			falseBranchSize = generateBlockStmt(codegen, ifStmt->as.ifElse, NULL, NULL, 0, prevStackSize, toEmit);
		} else {
			falseBranchSize = generateIfStmt(codegen, ifStmt->as.ifElseIf, prevStackSize, toEmit);
		}
		if (falseBranchSize == -1) return -1;
		snprintf(instr, sizeof(instr), "\t.end_if_%d:\n", labelCounter);
		if (!pushItem(toEmit, strdup(instr))) return -1;

		return (trueBranchSize > falseBranchSize) ? trueBranchSize : falseBranchSize;
	}
}

int generateReturnStmt(Codegen* codegen, ReturnStmt* returnStmt, DynamicArray* toEmit) {
	if (codegen == NULL || returnStmt == NULL) {
		return 0;
	}

	return generateExpression(codegen, returnStmt->expression, toEmit);
}

int generateExpression(Codegen* codegen, Expression* expression, DynamicArray* toEmit) {
	if (codegen == NULL || expression == NULL) {
		return 0;
	}

	switch (expression->type) {
		case EXPR_WRAPPER_EXPR:
			return generateExpression(codegen, expression->as.expWrap, toEmit);
		case ASSIGN_EXPR:
			return generateAssignment(codegen, expression->as.assignment, toEmit);
		case UNARY_EXPR:
			return generateUnaryOperation(codegen, expression->as.unop, toEmit);
		case BINOP_EXPR:
			return generateBinOperation(codegen, expression->as.binop, toEmit);
		case VARIABLE_EXPR:
			return generateVariable(codegen, expression->as.variable, toEmit);
		case VALUE_EXPR:
			return generateValue(codegen, expression->as.value, toEmit);
		default:
			fprintf(stderr, "Error: Unexpected Expression type in generate Expression\n");
			return 0;
	}
}

int generateBinOperation(Codegen* codegen, BinOperation* binOperation, DynamicArray* toEmit) {
	if (codegen == NULL || binOperation == NULL) {
		return 0;
	}

	if (!generateExpression(codegen, binOperation->right, toEmit)) return 0;
	if (!pushItem(toEmit, strdup("push rax\n"))) return 0;
	if (!generateExpression(codegen, binOperation->left, toEmit)) return 0;
	if (!pushItem(toEmit, strdup("pop rdx\n"))) return 0;

	switch (binOperation->type) {
		case ADD_OP:
			return pushItem(toEmit, strdup("add rax, rdx\n"));
		case SUB_OP:
			return pushItem(toEmit, strdup("sub rax, rdx\n"));
		case MUL_OP:
			return pushItem(toEmit, strdup("mul rdx\n"));
		case DIV_OP:
			return pushItem(toEmit, strdup("div rdx\n"));
		case MOD_OP:
			fprintf(stderr, "Error: Operator Modulus not implemented yet\n");
			return 0;
		case ST_OP:
		case STE_OP:
		case GT_OP:
		case GTE_OP:
		case EQ_OP:
		case NEQ_OP:
			if (!pushItem(toEmit, strdup("cmp rax, rdx\n"))) return 0;
			switch (binOperation->type) {
				case ST_OP:
					if(!pushItem(toEmit, strdup("setl al\n"))) return 0;
					break;
				case STE_OP:
					if(!pushItem(toEmit, strdup("setle al\n"))) return 0;
					break;
				case GT_OP:
					if(!pushItem(toEmit, strdup("setg al\n"))) return 0;
					break;
				case GTE_OP:
					if(!pushItem(toEmit, strdup("setge al\n"))) return 0;
					break;
				case EQ_OP:
					if(!pushItem(toEmit, strdup("sete al\n"))) return 0;
					break;
				case NEQ_OP:
					if(!pushItem(toEmit, strdup("setne al\n"))) return 0;
					break;
				default:
					fprintf(stderr, "Error: Encountered illegal Operand in generateBinOperation\n");
					return 0;
			}
			return pushItem(toEmit, strdup("movzx rax, al\n"));
		default:
			fprintf(stderr, "Error: Encountered illegal Operand generateBinOperation\n");
			return 0;
	}
}

int generateUnaryOperation(Codegen* codegen, UnaryOperation* unaryOperation, DynamicArray* toEmit) {
	if (codegen == NULL || unaryOperation == NULL) {
		return 0;
	}

	if (unaryOperation->type == NOT) {
		if (!generateExpression(codegen, unaryOperation->right, toEmit)) return 0;
		return pushItem(toEmit, strdup("test rax, rax\nsetz al\nmovzx rax, al\n"));
	}
	else if (unaryOperation->type == MINUS) {
		if (!generateExpression(codegen, unaryOperation->right, toEmit)) return 0;
		return pushItem(toEmit, strdup("neg rax\n"));
	}

	fprintf(stderr, "Error: Unexpected Unary Operation Operand encountered in generateUnaryOperation: %d\n", unaryOperation->type);
	return 0;
}

int generateAssignment(Codegen* codegen, Assignment* assignment, DynamicArray* toEmit) {
	if (codegen == NULL || assignment == NULL) {
		return 0;
	}

	if (!generateExpression(codegen, assignment->expression, toEmit)) return 0;
	int variableOffset = getVariableOffset(codegen, assignment->variable->id);

	ValueType variableType = assignment->variable->type;
	int typeSize = getTypeSize(variableType);
	const char* reg = getRegister(0, typeSize);

	char lastInstruction[64];
	snprintf(lastInstruction, sizeof(lastInstruction), "mov [rbp%d], %s\n", variableOffset, reg);

	return pushItem(toEmit, strdup(lastInstruction));
}

int generateVariable(Codegen* codegen, Variable* variable, DynamicArray* toEmit) {
	if (codegen == NULL || variable == NULL) {
		return 0;
	}

	ValueType variableType = variable->type;

	if (variableType == UNKNOWN) {
		return 1;
	}

	int variableOffset = getVariableOffset(codegen, variable->id);
	HashTable* varScope = getScopeForVar(codegen, variable->id);
	int typeSize = getTypeSize(variableType);
	char instr[128];
	char varLocation[64];

	// global variable
	if ((HashTable*)codegen->scopes->array[0] == varScope) {
		snprintf(varLocation, sizeof(varLocation), "%s", variable->id);
	}
	else {
		snprintf(varLocation, sizeof(varLocation), "[rbp%d]", variableOffset);
	}

	switch (typeSize) {
		case 1:
			snprintf(instr, sizeof(instr), "movzx rax, BYTE %s\n", varLocation);
			break;
		case 4:
			snprintf(instr, sizeof(instr), "mov eax, %s\n", varLocation);
			break;
		case 8:
			snprintf(instr, sizeof(instr), "mov rax, %s\n", varLocation);
			break;
		default:
			fprintf(stderr, "Error: Unsupported type size in generateVariable\n");
			return 0;
	}
	return pushItem(toEmit, strdup(instr));
}

int generateValue(Codegen* codegen, Value* value, DynamicArray* toEmit) {
	if (codegen == NULL || value == NULL) {
		return 0;
	}
	char instr[64];
	switch (value->type) {
		case LONG_TYPE:
			snprintf(instr, sizeof(instr), "mov rax, %lld\n", value->as.i_64);
			return pushItem(toEmit, strdup(instr));
		case DOUBLE_TYPE:
			fprintf(stderr, "Error: Did not implement float Types yet :(\n");
			return 0;
		case BOOL_TYPE:
			snprintf(instr, sizeof(instr), "mov al, %d\n", value->as.b);
			return pushItem(toEmit, strdup(instr));
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

const char* getRegister(int reg, int typeSize) {
	static const char* register_names[7][4] = {
		{"al",  "ax",  "eax", "rax"},
		{"dil", "di",  "edi", "rdi"},
		{"sil", "si",  "esi", "rsi"},
		{"dl",  "dx",  "edx", "rdx"},
		{"cl",  "cx",  "ecx", "rcx"},
		{"r8b", "r8w", "r8d", "r8" },
		{"r9b", "r9w", "r9d", "r9" }
	};

	if (reg < 0 || typeSize > 6) {
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
