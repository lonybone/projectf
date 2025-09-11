#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdbool.h>

static const int INITIAL_CAPACITY = 16;
static const int MINUMUM_SIZE = 16;

typedef struct HashTable HashTable;
typedef struct Bucket Bucket;
typedef struct Box Box;

typedef void (*GenericFreeFunc) (void*);
typedef struct DynamicArray {
	int growthFactor;
	int maxSize;
	int minSize;
	int size;
	void** array;
	GenericFreeFunc freeFunc;
} DynamicArray;

DynamicArray* dynamicArray(int growthFactor, GenericFreeFunc freeFunc);
void* getItem(DynamicArray* dynamicArray, int idx);
void* peekArray(DynamicArray* dynamicArray);
int pushItem(DynamicArray* dynamicArray, void* item);
void* popItem(DynamicArray* dynamicArray);
void freeArray(DynamicArray* dynamicArray);

// function by Dan Bernstein found on http://www.cse.yorku.ca/~oz/hash.html
// added double parenthesis in while condition for clarity
static inline unsigned long hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

struct HashTable {
	int size;
	GenericFreeFunc freeFunc;
	Bucket* array[];
};

struct Bucket {
	char* id;
	void* value;
	Bucket* next;
};

HashTable* hashTable(int size, GenericFreeFunc freeFunc);
void* getValue(HashTable* table, char* key);
int insertKeyPair(HashTable* table, char* key, void* value);
int containsKey(HashTable* table, char* key);
int updateKeyPair(HashTable* table, char* key, void* value);
void removeKey(HashTable* table, char* key);
void freeTable(void* table);

#endif
