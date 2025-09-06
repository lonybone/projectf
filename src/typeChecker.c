#include "typeChecker.h"
#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

int checkTypes(TypeChecker* typeChecker);
int checkStatement(TypeChecker* typeChecker, Statement* statement);
int checkFunctionStmt(TypeChecker* typeChecker, FunctionStmt* function);
int checkBlockStmt(TypeChecker* typeChecker, BlockStmt* blockStmt);
int checkWhileStmt(TypeChecker* typeChecker, WhileStmt* whileStmt);
int checkIfStmt(TypeChecker* typeChecker, IfStmt* ifStmt);
int checkExpression(TypeChecker* typeChecker, Expression* expression);
ValueType checkAssignment(TypeChecker* typeChecker, Assignment* assignment);
ValueType checkBinOperation(TypeChecker* typeChecker, BinOperation* binOperation);
ValueType checkUnaryOperation(TypeChecker* typeChecker, UnaryOperation* unaryOperation);
ValueType checkVariable(TypeChecker* typeChecker, Variable* variable);
ValueType checkValue(TypeChecker* typeChecker, Value* value);
ValueType getVarTypeFromScopes(TypeChecker* typeChecker, char* id);
HashTable* getVarScope(TypeChecker* typeChecker, char* id);
void freeChecker(TypeChecker* typeChecker);

TypeChecker* initializeChecker (DynamicArray* ast) {
	
	if (ast == NULL) {
		return NULL;
	}

	TypeChecker* typeChecker = calloc(1, sizeof(TypeChecker));

	if (typeChecker == NULL) {
		return NULL;
	}

	typeChecker->ast = ast;
	typeChecker->typeScopes = dynamicArray(2, freeTable);

	if (typeChecker->typeScopes == NULL) {
		freeChecker(typeChecker);
		return NULL;
	}

	HashTable* typeTable = hashTable(256, free);

	if (typeTable == NULL) {
		freeChecker(typeChecker);
		return NULL;
	}

	if (!pushItem(typeChecker->typeScopes, typeTable))  {
		freeChecker(typeChecker);
		return NULL;
	}

	return typeChecker;
}

int checkTypes(TypeChecker* typeChecker) {

	if (typeChecker == NULL) {
		return 0;
	}

	for (int i = 0; i < typeChecker->ast->size; i++) {
		if (!checkStatement(typeChecker, typeChecker->ast->array[i])) { 
			for (int i = 0; i < typeChecker->typeScopes->size; i++) {
				HashTable* table = (HashTable*)typeChecker->typeScopes->array[i];
				freeTable(table);
			}

			freeArray(typeChecker->typeScopes);

			typeChecker->typeScopes = dynamicArray(2, freeTable);

			if (typeChecker->typeScopes == NULL) {
				freeChecker(typeChecker);
				return 0;
			}

			HashTable* typeTable = hashTable(256, free);

			if (typeTable == NULL) {
				freeChecker(typeChecker);
				return 0;
			}

			if (!pushItem(typeChecker->typeScopes, typeTable))  {
				freeChecker(typeChecker);
				return 0;
			}

			return 0;
		}
	}

	return 1;
}

int checkStatement(TypeChecker* typeChecker, Statement* statement) {
	
	if (typeChecker == NULL || statement == NULL) {
		return 0;
	}
	
	switch (statement->type) {
		case EXPRESSION_STMT: {
			// implicit declaration handling
			Expression* expression = statement->as.expression;
			if (expression->type == VARIABLE_EXPR) {
				Variable* variable = expression->as.variable;
				ValueType valueType = getVarTypeFromScopes(typeChecker, variable->id);
				// variable is not in any scope
				if (valueType == -1) {
					HashTable* currentScope = (HashTable*)peekArray(typeChecker->typeScopes);
					int* unknownValue = malloc(sizeof(int));
					if (unknownValue == NULL) return 0;
					*unknownValue = (int)UNKNOWN;
					if (!insertKeyPair(currentScope, variable->id, unknownValue)) {
						fprintf(stderr, "Error: Could not declare variable %s\n", variable->id);
						return 0;
					}
					expression->valueType = UNKNOWN;
					return 1;
				}
			}
			return checkExpression(typeChecker, statement->as.expression);
		}
		case FUNCTION_STMT:
			return checkFunctionStmt(typeChecker, statement->as.function);
		case BLOCK_STMT:
			return checkBlockStmt(typeChecker, statement->as.blockStmt);
		case WHILE_STMT:
			return checkWhileStmt(typeChecker, statement->as.whileStmt);
		case IF_STMT:
			return checkIfStmt(typeChecker, statement->as.ifStmt);
		case DECLARATION_STMT:
		default:
			fprintf(stderr, "Error: unexpected Statement type in checkStatement\n");
			return 0;
	}
}

int checkExpression(TypeChecker* typeChecker, Expression* expression) {
	if (typeChecker == NULL || expression == NULL) {
		return 0;
	}

	ValueType typeResult;
	switch (expression->type) {
		case EXPR_WRAPPER_EXPR:
			typeResult = checkExpression(typeChecker, expression->as.expWrap);
			break;
		case ASSIGN_EXPR:
			typeResult = checkAssignment(typeChecker, expression->as.assignment);
			break;
		case BINOP_EXPR:
			typeResult = checkBinOperation(typeChecker, expression->as.binop);
			break;
		case UNARY_EXPR:
			typeResult = checkUnaryOperation(typeChecker, expression->as.unop);
			break;
		case VARIABLE_EXPR:
			typeResult = checkVariable(typeChecker, expression->as.variable);
			break;
		case VALUE_EXPR:
			typeResult = checkValue(typeChecker, expression->as.value);
			break;
		default:
			fprintf(stderr, "Error: unexpected Expression type in checkExpression\n");
			return 0;
	}

	// variable expressions are allowed to be declared on their own
	if (typeResult == -1 && expression->type != VARIABLE_EXPR) {
		return 0;
	}
	expression->valueType = typeResult;
	return 1;
}

// check if variable already has a type, if so then check wether expression is of same type
// if variable has no type then this is a declaration and the type shall be annotated
ValueType checkAssignment(TypeChecker* typeChecker, Assignment* assignment) {
	if (typeChecker == NULL || assignment == NULL) {
		return -1;
	}

	if (!checkExpression(typeChecker, assignment->expression)) return -1;
	ValueType expressionType = assignment->expression->valueType;

	Variable* variable = assignment->variable;
	ValueType variableType = getVarTypeFromScopes(typeChecker, variable->id);

	// is not in scope, can be added to current scope
	if (variableType == -1) {
		HashTable* currentScope = (HashTable*)peekArray(typeChecker->typeScopes);

		variable->type = expressionType;
		int* varType = malloc(sizeof(int));
		if (varType == NULL) return -1;
		*varType = (int)variable->type;
		if(!insertKeyPair(currentScope, variable->id, varType)) return -1;
	}

	// is in one scope
	else { 
		HashTable* varScope = getVarScope(typeChecker, variable->id);
		// not initialized
		if (variableType == UNKNOWN) {

			variable->type = expressionType;
			int* varType = malloc(sizeof(int));
			if (varType == NULL) return -1;
			*varType = (int)variable->type;
			if(!updateKeyPair(varScope, variable->id, varType)) return -1;
		}
	
		// type mismatch
		else if (variableType != expressionType) {
			fprintf(stderr, "Error: Tried to Assign value of type %d to Variable %s of type %d\n", expressionType, variable->id, variableType);
			return -1;
		}
	}

	return expressionType;
}
// check wether left and right expression satisfy the same type recursively
// annotate own type after checking if both children have the same type as that type
// if one expression has no type yet then this is a compile time error of attempting to access non initialized variable
ValueType checkBinOperation(TypeChecker* typeChecker, BinOperation* binOperation) { 
	if (typeChecker == NULL || binOperation == NULL) {
		return -1;
	}

	if(!checkExpression(typeChecker, binOperation->left)) return -1;
	if(!checkExpression(typeChecker, binOperation->right)) return -1;

	switch (binOperation->type) {
		case ADD_OP:
		case SUB_OP:
		case MUL_OP:
		case DIV_OP:
		case MOD_OP:
			// forwarded error from accessing variable with unknown value type
			if (binOperation->left->valueType == UNKNOWN || binOperation->right->valueType == UNKNOWN) {
				return -1;
			}
			if (binOperation->left->valueType == binOperation->right->valueType) {
				return binOperation->left->valueType;
			}
			else {
				fprintf(stderr, "Error: Type Mismatch between %d and %d in checkBinOperation with Operand %d\n", binOperation->left->valueType, binOperation->right->valueType, binOperation->type); 
				return -1;
			}
		case ST_OP:
		case STE_OP:
		case GT_OP:
		case GTE_OP:
		case EQ_OP:
		case NEQ_OP:
			// forwarded error from accessing variable with unknown value type
			if (binOperation->left->valueType == UNKNOWN || binOperation->right->valueType == UNKNOWN) {
				return -1;
			}
			if (binOperation->left->valueType == binOperation->right->valueType) {
				return BOOL_TYPE;
			}
			else {
				fprintf(stderr, "Error: Type Mismatch between %d and %d in checkBinOperation with Operand %d\n", binOperation->left->valueType, binOperation->right->valueType, binOperation->type); 
				return -1;
			}
			
		default:
			fprintf(stderr, "Error: Encountered unknown Binary Operation Type in checkBinOperation in typeChecker\n");
			return -1;
	}
}
// check wether expression is of type boolean (! operator) or a number (- operator)
ValueType checkUnaryOperation(TypeChecker* typeChecker, UnaryOperation* unaryOperation) {
	if (typeChecker == NULL || unaryOperation == NULL) {
		return -1;
	}

	if (!checkExpression(typeChecker, unaryOperation->right)) return -1;

	if (unaryOperation->type == NOT) {
		if (unaryOperation->right->valueType == BOOL_TYPE) {
			return BOOL_TYPE;
		}
	}
	if (unaryOperation->type == MINUS) {
		if (unaryOperation->right->valueType == LONG_TYPE) {
			return LONG_TYPE;
		}
		if (unaryOperation->right->valueType == DOUBLE_TYPE) {
			return DOUBLE_TYPE;
		}
	}
	fprintf(stderr, "Error: Tried To apply Operator %d on Expression on type %d\n", MINUS, unaryOperation->right->valueType);
	return -1;
}
// if the variable has a type then just return
// if the variable has no type, then throw a compile time error since this function will only get called inside an expression (as of currently)
ValueType checkVariable(TypeChecker* typeChecker, Variable* variable) {
	if (typeChecker == NULL || variable == NULL) {
		return -1;
	}

	ValueType type = getVarTypeFromScopes(typeChecker, variable->id);
	if (type == -1) {
		if (variable->type != UNKNOWN) {
			HashTable* currentScope = (HashTable*)peekArray(typeChecker->typeScopes);

			int* varType = malloc(sizeof(int));
			if (varType == NULL) return -1;
			*varType = (int)variable->type;
			if(!insertKeyPair(currentScope, variable->id, varType)) return -1;
		}
		return -1;
	}
	variable->type = type;
	return type;
	
}
// just set the value type according to its member
ValueType checkValue(TypeChecker* typeChecker, Value* value) {
	if (typeChecker == NULL || value == NULL) {
		return -1;
	}

	return value->type;
}

int checkFunctionStmt(TypeChecker* typeChecker, FunctionStmt* function) {
	if (typeChecker == NULL || function == NULL) {
		return 0;
	}
	return 0;
}

int checkBlockStmt(TypeChecker* typeChecker, BlockStmt* blockStmt) {
	if (typeChecker == NULL || blockStmt == NULL) {
		return 0;
	}

	HashTable* blockScope = hashTable(256, free);

	if (blockScope == NULL) return 0;

	if (!pushItem(typeChecker->typeScopes, blockScope)) {
		freeTable(blockScope);
		return 0;
	}

	for (int i = 0; i < blockStmt->stmts->size; i++) {
		if(!checkStatement(typeChecker, blockStmt->stmts->array[i])) return 0;
	}

	freeTable(popItem(typeChecker->typeScopes));
	return 1;
}

int checkWhileStmt(TypeChecker* typeChecker, WhileStmt* whileStmt) {
	if (typeChecker == NULL || whileStmt == NULL) {
		return 0;
	}

	Expression* condition = whileStmt->condition;
	if(!checkExpression(typeChecker, whileStmt->condition)) return 0; 
	if (condition->valueType != BOOL_TYPE) {
		fprintf(stderr, "Error: Condition of WhileStatement is not of type boolean\n");
		return 0;
	}
	if(!checkBlockStmt(typeChecker, whileStmt->body)) return 0;

	return 1;
}

int checkIfStmt(TypeChecker* typeChecker, IfStmt* ifStmt) {
	if (typeChecker == NULL || ifStmt == NULL) {
		return 0;
	}

	Expression* condition = ifStmt->condition;
	if(!checkExpression(typeChecker, ifStmt->condition)) return 0; 
	if (condition->valueType != BOOL_TYPE) {
		fprintf(stderr, "Error: Condition of IfStatement is not of type boolean\n");
		return 0;
	}

	if(!checkBlockStmt(typeChecker, ifStmt->trueBody)) return 0;

	if (ifStmt->type == IF_ELSE) {
		if(!checkBlockStmt(typeChecker, ifStmt->as.ifElse)) return 0;
	}

	else if (ifStmt->type == IF_ELSE_IF) {
		if(!checkIfStmt(typeChecker, ifStmt->as.ifElseIf)) return 0;
	}

	return 1;
}

ValueType getVarTypeFromScopes(TypeChecker* typeChecker, char* id) {
	if (typeChecker == NULL || id == NULL) {
		return -1;
	}

	for (int i = 0; i < typeChecker->typeScopes->size; i++) {
		if (containsKey(typeChecker->typeScopes->array[i], id)) {
			void* value = getValue((HashTable*)typeChecker->typeScopes->array[i], id);
			if (value == NULL) return -1;
			return (ValueType)*(int*)(value);
		}
	}

	return -1;
}

HashTable* getVarScope(TypeChecker* typeChecker, char* id) {
	if (typeChecker == NULL || id == NULL) {
		return NULL;
	}

	for (int i = 0; i < typeChecker->typeScopes->size; i++) {
		if (containsKey(typeChecker->typeScopes->array[i], id)) {
			return (HashTable*)typeChecker->typeScopes->array[i];
		}
	}

	return NULL;
}

void freeChecker(TypeChecker* typeChecker) {
	if (typeChecker == NULL) return;
	freeArray(typeChecker->typeScopes);
	free(typeChecker);
}
