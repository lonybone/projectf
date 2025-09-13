#include "parser.h"
#include "utils.h"

#ifndef CODEGEN_H
#define CODEGEN_H

typedef struct Codegen Codegen;

struct Codegen {
	int idx;
	int maxSize;
	char* buffer;
	int stackOffset;
	FunctionStmt* currentFunction;
	DynamicArray* labelCounters;
	DynamicArray* scopes;
	DynamicArray* ast;
};

Codegen* initializeCodegen(DynamicArray* ast);
int generate(Codegen* codegen);
void writeToFile(Codegen* codegen, const char* filepath);
void freeCodegen(Codegen* codegen);

#endif
