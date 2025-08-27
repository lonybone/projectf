#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include "utils.h"
typedef struct TypeChecker TypeChecker;

struct TypeChecker {
	DynamicArray* ast;
	DynamicArray* typeScopes;
};

#endif
