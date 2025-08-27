#include "typeChecker.h"
#include "parser.h"
#include "utils.h"

int checkTypes(TypeChecker* typeChecker);
int checkStatement(TypeChecker* typeChecker, Statement* statement);
int checkBlockStmt(TypeChecker* typeChecker, BlockStmt* blockStmt);
int checkWhileStmt(TypeChecker* typeChecker, WhileStmt* whileStmt);
int checkIfStmt(TypeChecker* typeChecker, IfStmt* ifStmt);
int checkExpression(TypeChecker* typeChecker, Expression* expression);
int checkAssignment(TypeChecker* typeChecker, Assignment* assignment);
int checkBinOperation(TypeChecker* typeChecker, BinOperation* binOperation);
int checkUnaryOperation(TypeChecker* typeChecker, UnaryOperation* unaryOperation);
int checkVariable(TypeChecker* typeChecker, Variable* variable);
int checkValue(TypeChecker* typeChecker, Value* value);
int varInOneScope(TypeChecker* typeChecker, char* id);

int checkTypes(TypeChecker* typeChecker) {

	if (typeChecker == NULL) {
		return 0;
	}

	DynamicArray* typeScopes = dynamicArray(2);

	if (typeScopes == NULL) {
		return 0;
	}

	HashTable* typeTable = hashTable(256);

	if (typeTable == NULL) {
		return 0;
	}

	if (!pushItem(typeScopes, typeTable)) return 0;

	for (int i = 0; i < typeChecker->ast->size; i++) {
		if (!checkStatement(typeChecker, typeChecker->ast->array[i])) { 

			for (int i = 0; i < typeChecker->ast->size; i++) {
				HashTable* table = (HashTable*)typeChecker->typeScopes->array[i];
				freeTable(table);
			}

			freeArray(typeScopes);
			return 0;
		}
	}

	for (int i = 0; i < typeChecker->ast->size; i++) {
		HashTable* table = (HashTable*)typeChecker->typeScopes->array[i];
		freeTable(table);
	}

	freeArray(typeScopes);
	return 1;
}

int checkStatement(TypeChecker* typeChecker, Statement* statement) {
	
	if (typeChecker == NULL || statement == NULL) {
		return 0;
	}
	
	switch (statement->type) {
		case EXPRESSION_STMT:
			return checkExpression(typeChecker, statement->as.expression);
		case BLOCK_STMT:
			return checkBlockStmt(typeChecker, statement->as.blockStmt);
		case WHILE_STMT:
			return checkWhileStmt(typeChecker, statement->as.whileStmt);
		case IF_STMT:
			return checkIfStmt(typeChecker, statement->as.ifStmt);
		case DECLARATION_STMT:
		default:
			fprintf(stderr, "Error: unexpected Statement type in checkStatement");
			return 0;
	}
}

int checkExpression(TypeChecker* typeChecker, Expression* expression) {
	if (typeChecker == NULL || expression == NULL) {
		return 0;
	}

	switch (expression->type) {
		case EXPR_WRAPPER_EXPR:
			return checkExpression(typeChecker, expression->as.expWrap);
		case ASSIGN_EXPR:
			return checkAssignment(typeChecker, expression->as.assignment);
		case BINOP_EXPR:
			return checkBinOperation(typeChecker, expression->as.binop);
		case UNARY_EXPR:
			return checkUnaryOperation(typeChecker, expression->as.unop);
		case VARIABLE_EXPR:
			return checkVariable(typeChecker, expression->as.variable);
		case VALUE_EXPR:
			return checkValue(typeChecker, expression->as.value);
		default:
			fprintf(stderr, "Error: unexpected Expression type in checkExpression");
			return 0;
	}
}

// check if variable already has a type, if so then check wether expression is of same type
// if variable has no type then this is a declaration and the type shall be annotated
int checkAssignment(TypeChecker* typeChecker, Assignment* assignment) {
	if (typeChecker == NULL || assignment == NULL) {
		return 0;
	}

	return 1;
}
// check wether left and right expression satisfy the same type recursively
// annotate own type after checking if both children have the same type as that type
// if one expression has no type yet then this is a compile time error of attempting to access non initialized variable
int checkBinOperation(TypeChecker* typeChecker, BinOperation* binOperation) { 
	if (typeChecker == NULL || binOperation == NULL) {
		return 0;
	}

	return 1;
}
// check wether expression is of type boolean (! operator) or a number (- operator)
int checkUnaryOperation(TypeChecker* typeChecker, UnaryOperation* unaryOperation) {
	if (typeChecker == NULL || unaryOperation == NULL) {
		return 0;
	}

	return 1;
}
// if the variable has a type then just return
// if the variable has no type, then also return since its a declaration w/o initialization
int checkVariable(TypeChecker* typeChecker, Variable* variable) {
	if (typeChecker == NULL || variable == NULL) {
		return 0;
	}

	return 1;
}
// just return the value
int checkValue(TypeChecker* typeChecker, Value* value) {
	if (typeChecker == NULL || value == NULL) {
		return 0;
	}

	return 1;
}

int checkBlockStmt(TypeChecker* typeChecker, BlockStmt* blockStmt) {
	if (typeChecker == NULL || blockStmt == NULL) {
		return 0;
	}

	for (int i = 0; i < blockStmt->stmts->size; i++) {
		if(!checkStatement(typeChecker, blockStmt->stmts->array[i])) return 0;
	}

	return 1;
}

int checkWhileStmt(TypeChecker* typeChecker, WhileStmt* whileStmt) {
	if (typeChecker == NULL || whileStmt == NULL) {
		return 0;
	}

	Expression* condition = whileStmt->condition;
	if(!checkExpression(typeChecker, whileStmt->condition) || condition->valueType != BOOL_TYPE) return 0;
	if(!checkBlockStmt(typeChecker, whileStmt->body)) return 0;

	return 1;
}

int checkIfStmt(TypeChecker* typeChecker, IfStmt* ifStmt) {
	if (typeChecker == NULL || ifStmt == NULL) {
		return 0;
	}

	Expression* condition = ifStmt->condition;
	if(!checkExpression(typeChecker, ifStmt->condition) || condition->valueType != BOOL_TYPE) return 0;

	if (ifStmt->type == IF_ELSE) {
		if(!checkBlockStmt(typeChecker, ifStmt->as.ifElse)) return 0;
	}

	else if (ifStmt->type == IF_ELSE_IF) {
		if(!checkIfStmt(typeChecker, ifStmt->as.ifElseIf)) return 0;
	}

	return 1;
}

int varInOneScope(TypeChecker* typeChecker, char* id) {
	if (typeChecker == NULL || id == NULL) {
		return 0;
	}

	return 1;
}
