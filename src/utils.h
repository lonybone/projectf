#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdbool.h>

static const int INITIAL_CAPACITY = 16;

typedef struct HashTable HashTable;
typedef struct Bucket Bucket;

typedef enum {
	SUCCESS,
	ALREADY_EXISTS,
	DOESNT_EXIST,
	ALLOC_FAIL
} Operation;

typedef struct DynamicArray {
	int growthFactor;
	int maxSize;
	int size;
	void** array;
} DynamicArray;

DynamicArray* dynamicArray(int growthFactor);
void* getStmt(DynamicArray* dynamicArray, int idx);
bool appendStmt(DynamicArray* dynamicArray, void* statement);
void freeArray(DynamicArray* dynamicArray);

// function by Dan Bernstein found on http://www.cse.yorku.ca/~oz/hash.html
static inline unsigned long hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

struct HashTable {
	int size;
	Bucket* array[];
};

struct Bucket {
	char* id;
	char* value;
	Bucket* next;
};

HashTable* hashTable(int size);
char* getValue(HashTable* table, char* key);
Operation insertKeyPair(HashTable* table, char* key, char* value);
Operation updateKeyPair(HashTable* table, char* key, char* value);
void removeKey(HashTable* table, char* key);
void freeTable(HashTable* table);

#endif
