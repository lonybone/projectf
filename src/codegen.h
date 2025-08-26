#include "utils.h"

#ifndef CODEGEN_H
#define CODEGEN_H

typedef struct Codegen Codegen;

struct Codegen {
	int idx;
	int maxSize;
	char* buffer;
	DynamicArray* scopes;
	DynamicArray* ast;
	HashTable* idTable;
};

Codegen* initializeCodegen(DynamicArray* ast);
int generate(Codegen* codegen);
void freeCodegen(Codegen* codegen);

#endif
