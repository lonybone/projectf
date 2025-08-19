#include <stdio.h>
#include <stdlib.h>
#include "dynamicArray.h"
#include "parser.h"

DynamicArray *dynamicArray(int growthFactor) {
	DynamicArray* dynamicArray = (DynamicArray*)malloc(sizeof(DynamicArray));

	if (dynamicArray == NULL) {
		fprintf(stderr, "failed to allocate dynamic array");
		return NULL;
	}

	dynamicArray->growthFactor = growthFactor;
	dynamicArray->maxSize = INITIAL_CAPACITY; 
	dynamicArray->size = 0;
	dynamicArray->array = malloc(INITIAL_CAPACITY * sizeof(Statement*));

	if (dynamicArray->array == NULL) {
		fprintf(stderr, "failed to allocate static array in dynamic array");
		free(dynamicArray);
		return NULL;
	}

	return dynamicArray;
}

Statement* get(DynamicArray* dynamicArray, int idx) {
	if (idx < 0 || idx > dynamicArray->size) {
		fprintf(stderr, "index out of range for dynamic Array");
		return NULL;
	}
	return dynamicArray->array[idx];
}

int append(DynamicArray *dynamicArray, Statement *statement) {
	dynamicArray->array[dynamicArray->size++] = statement;

	if (dynamicArray->size == dynamicArray->maxSize) {

		dynamicArray->maxSize *= dynamicArray->growthFactor;
		Statement** tmp =  realloc(dynamicArray->array, dynamicArray->maxSize * sizeof(Statement*));

		if (tmp == NULL) {
			fprintf(stderr, "failed to allocate static array in dynamic array");
			return 1;
		}

		dynamicArray->array = tmp;
	}

	return 0;
}

void freeArray(DynamicArray* dynamicArray) {
	for (int i = 0; i < dynamicArray->size; i++) {
		free(dynamicArray->array[i]);
	}
	free(dynamicArray->array);
};
/*
int main(int argc, char* argv[]) {
	DynamicArray* arr = dynamicArray(2);
	for(int i = 0; i < 10; i++) {
		append(arr, malloc(sizeof(Statement*)));
		printf("just added pointer with value: %p\n", get(arr, i));
	}
	freeArray(arr);
	free(arr);
}
*/
