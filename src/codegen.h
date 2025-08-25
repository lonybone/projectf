#include "utils.h"

#ifndef CODEGEN_H
#define CODEGEN_H

typedef struct Codegen Codegen;

struct Codegen {
	int idx;
	int maxSize;
	char* buffer;
	DynamicArray* statements;
	HashTable* idTable;
};

Codegen* initializeCodegen(DynamicArray* statements);
int generate(Codegen* codegen);
void freeCodegen(Codegen* codegen);

#endif
