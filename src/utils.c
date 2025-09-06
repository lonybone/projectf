#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

DynamicArray *dynamicArray(int growthFactor, GenericFreeFunc freeFunc) {
	DynamicArray* dynamicArray = (DynamicArray*)malloc(sizeof(DynamicArray));

	if (dynamicArray == NULL) {
		fprintf(stderr, "failed to allocate dynamic array");
		return NULL;
	}

	dynamicArray->growthFactor = growthFactor;
	dynamicArray->maxSize = INITIAL_CAPACITY; 
	dynamicArray->size = 0;
	dynamicArray->freeFunc = freeFunc;
	dynamicArray->array = malloc(INITIAL_CAPACITY * sizeof(void*));

	if (dynamicArray->array == NULL) {
		fprintf(stderr, "failed to allocate static array in dynamic array");
		free(dynamicArray);
		return NULL;
	}

	return dynamicArray;
}

void* getItem(DynamicArray* dynamicArray, int idx) {
	if (dynamicArray == NULL) {
		return NULL;
	}

	if (idx < 0 || idx >= dynamicArray->size) {
		fprintf(stderr, "index out of range for dynamic Array");
		return NULL;
	}
	return dynamicArray->array[idx];
}

void* peekArray(DynamicArray* dynamicArray) {
	if (dynamicArray == NULL) {
		return NULL;
	}

	if (dynamicArray == NULL || dynamicArray->size == 0) {
		return NULL;
	}

	return dynamicArray->array[dynamicArray->size-1];
}

int pushItem(DynamicArray *dynamicArray, void *item) {
	if (dynamicArray == NULL) {
		return 0;
	}
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
	if (dynamicArray == NULL) {
		return NULL;
	}
	
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
	if (dynamicArray == NULL) {
		return;
	}
	for (int i = 0; i < dynamicArray->size; i++) {
		dynamicArray->freeFunc(dynamicArray->array[i]);
	}
	free(dynamicArray->array);
	free(dynamicArray);
};

HashTable* hashTable(int size, GenericFreeFunc freeFunc) {
	int totalSize = sizeof(HashTable) + (size * sizeof(Bucket*));
	HashTable* table = malloc(totalSize);

	if (table == NULL) {
		fprintf(stderr, "failed to allocate memory for hashTable");
		return NULL;
	}

	table->size = size;
	table->freeFunc = freeFunc;
	memset(table->array, 0, size * sizeof(Bucket*));
	return table;
}

int insertKeyPair(HashTable *table, char *key, void* value) {
	if (table == NULL || key == NULL) {
		return 0;
	}
	unsigned long hashedKey = hash((unsigned char*)key) % table->size;

	Bucket* current = table->array[hashedKey];
	while (current != NULL) {
		if (strcmp(current->id, key) == 0) {
			return 0;
		}
		current = current->next;
	}

	Bucket* newBucket = calloc(1, sizeof(Bucket));

	if (newBucket == NULL) {
		fprintf(stderr, "Error: failed to allocate new Bucket");
		return 0;
	}

	newBucket->id = strdup(key);
	if (newBucket->id == NULL) {
		free(newBucket);
		return 0;
	}

	newBucket->value = value;
	newBucket->next = table->array[hashedKey];
	table->array[hashedKey] = newBucket;
	return 1;
}

int containsKey(HashTable* table, char* key) {
	if (table == NULL || key == NULL) {
		return 0;
	}

	unsigned long hashedKey = hash((unsigned char*)key) % table->size;

	Bucket* current = table->array[hashedKey];
	while (current != NULL) {
		if (strcmp(current->id, key) == 0) {
			return 1;
		}
		current = current->next;
	}

	return 0;
}

void* getValue(HashTable* table, char* key) {
	if (table == NULL || key == NULL) {
		return NULL;
	}

	unsigned long hashedKey = hash((unsigned char*)key) % table->size;

	Bucket* current = table->array[hashedKey];
	while (current != NULL) {
		if (strcmp(current->id, key) == 0) {
			return current->value;
		}
		current = current->next;
	}

	return NULL;
}

int updateKeyPair(HashTable *table, char *key, void* value) {
	if (table == NULL) {
		return 0;
	}
	unsigned long hashedKey = hash((unsigned char*)key) % table->size;

	Bucket* current = table->array[hashedKey];
	while (current != NULL) {
		if (strcmp(current->id, key) == 0) {
			table->freeFunc(current->value);
			current->value = value;
			return 1;
		}
		current = current->next;
	}

	return 0;
}

void removeKey(HashTable *table, char *key) {
	if (table == NULL || key == NULL) {
		return;
	}

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
			table->freeFunc(current->value);
			free(current);
			return;
		}
		previous = current;
		current = current->next;
	}
}

void freeTable(void *table) {
	if (table == NULL) {
		return;
	}

	HashTable* t = (HashTable*) table;

	for (int i = 0; i < t->size; i++) {
		Bucket* current = t->array[i];
		Bucket* previous = NULL;

		while (current != NULL) {
			free(current->id);
			t->freeFunc(current->value);

			previous = current;
			current = current->next;

			free(previous);
		}
	}

	free(table);
}
