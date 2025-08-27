#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include "utils.h"
typedef struct TypeChecker TypeChecker;

struct TypeChecker {
	DynamicArray* ast;
	DynamicArray* typeScopes;
};

TypeChecker* initializeChecker (DynamicArray* ast);
int checkTypes(TypeChecker* typeChecker);
void freeChecker(TypeChecker* typeChecker);

#endif
