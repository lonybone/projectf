#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include "parser.h"

typedef struct DynamicArray DynamicArray;
static const int INITIAL_CAPACITY = 16;

struct DynamicArray {
	int growthFactor;
	int maxSize;
	int size;
	Statement** array;
};

DynamicArray* dynamicArray(int growthFactor);
Statement* get(DynamicArray* dynamicArray, int idx);
int append(DynamicArray* dynamicArray, Statement* statement);
void freeArray(DynamicArray* dynamicArray);

#endif
