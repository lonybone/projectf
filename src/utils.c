#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

DynamicArray *dynamicArray(int growthFactor) {
	DynamicArray* dynamicArray = (DynamicArray*)malloc(sizeof(DynamicArray));

	if (dynamicArray == NULL) {
		fprintf(stderr, "failed to allocate dynamic array");
		return NULL;
	}

	dynamicArray->growthFactor = growthFactor;
	dynamicArray->maxSize = INITIAL_CAPACITY; 
	dynamicArray->size = 0;
	dynamicArray->array = malloc(INITIAL_CAPACITY * sizeof(void*));

	if (dynamicArray->array == NULL) {
		fprintf(stderr, "failed to allocate static array in dynamic array");
		free(dynamicArray);
		return NULL;
	}

	return dynamicArray;
}

void* getItem(DynamicArray* dynamicArray, int idx) {
	if (idx < 0 || idx >= dynamicArray->size) {
		fprintf(stderr, "index out of range for dynamic Array");
		return NULL;
	}
	return dynamicArray->array[idx];
}

DynamicArray* peekArray(DynamicArray* dynamicArray) {

	if (dynamicArray == NULL || dynamicArray->size == 0) {
		return NULL;
	}

	return dynamicArray->array[dynamicArray->size-1];
}

int pushItem(DynamicArray *dynamicArray, void *item) {
	dynamicArray->array[dynamicArray->size++] = item;

	if (dynamicArray->size == dynamicArray->maxSize) {

		dynamicArray->maxSize *= dynamicArray->growthFactor;
		void** tmp = realloc(dynamicArray->array, dynamicArray->maxSize * sizeof(void*));

		if (tmp == NULL) {
			fprintf(stderr, "Error: failed to allocate static array in dynamic array in pushItem");
			return 0;
		}

		dynamicArray->array = tmp;
	}

	return 1;
}

void* popItem(DynamicArray* dynamicArray) {
	
	if (dynamicArray == NULL || dynamicArray->size == 0) {
		return NULL;
	}

	void* ret = dynamicArray->array[dynamicArray->size-1];
	dynamicArray->array[dynamicArray->size-1] = NULL;
	dynamicArray->size--;

	if (dynamicArray->size <= dynamicArray->maxSize / dynamicArray->growthFactor) {

		dynamicArray->maxSize /= dynamicArray->growthFactor;
		void** tmp = realloc(dynamicArray->array, dynamicArray->maxSize * sizeof(void*));

		if (tmp == NULL) {
			return ret;
		}

		dynamicArray->array = tmp;

	}

	return ret;
}

void freeArray(DynamicArray* dynamicArray) {
	free(dynamicArray->array);
	free(dynamicArray);
};

HashTable* hashTable(int size) {
	int totalSize = sizeof(HashTable) + (size * sizeof(Bucket*));
	HashTable* table = malloc(totalSize);

	if (table == NULL) {
		fprintf(stderr, "failed to allocate memory for hashTable");
		return NULL;
	}

	table->size = size;
	memset(table->array, 0, size * sizeof(Bucket*));
	return table;
}

Operation insertKeyPair(HashTable *table, char *key, int value) {
	unsigned long hashedKey = hash((unsigned char*)key) % table->size;

	Bucket* current = table->array[hashedKey];
	while (current != NULL) {
		if (strcmp(current->id, key) == 0) {
			return ALREADY_EXISTS;
		}
		current = current->next;
	}

	Bucket* newBucket = malloc(sizeof(Bucket));

	if (newBucket == NULL) {
		fprintf(stderr, "Error: failed to allocate new Bucket");
		return ALLOC_FAIL;
	}

	newBucket->id = strdup(key);
	newBucket->value = value;
	if (newBucket->id == NULL) {
		free(newBucket->id);
		free(newBucket);
		return ALLOC_FAIL;
	}

	newBucket->next = table->array[hashedKey];
	table->array[hashedKey] = newBucket;
	return SUCCESS;
}

Operation updateKeyPair(HashTable *table, char *key, int value) {
	unsigned long hashedKey = hash((unsigned char*)key) % table->size;

	Bucket* current = table->array[hashedKey];
	while (current != NULL) {
		if (strcmp(current->id, key) == 0) {
			current->value = value;
			return SUCCESS;
		}
		current = current->next;
	}

	return DOESNT_EXIST;
}

void removeKey(HashTable *table, char *key) {

	unsigned long hashedKey = hash((unsigned char*)key) % table->size;

	Bucket* current = table->array[hashedKey];
	Bucket* previous = NULL;
	while (current != NULL) {
		if (strcmp(current->id, key) == 0) {
			if (previous == NULL) {
				table->array[hashedKey] = current->next;
			}

			else {
				previous->next = current->next;
			}

			free(current->id);
			free(current);
			return;
		}
		previous = current;
		current = current->next;
	}
}

void freeTable(HashTable *table) {
	for (int i = 0; i < table->size; i++) {
		Bucket* current = table->array[i];
		Bucket* previous = NULL;

		while (current != NULL) {
			free(current->id);

			previous = current;
			current = current->next;

			free(previous);
		}
	}

	free(table);
}
