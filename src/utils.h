#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdbool.h>

static const int INITIAL_CAPACITY = 16;

typedef struct HashTable HashTable;
typedef struct Bucket Bucket;

typedef struct DynamicArray {
	int growthFactor;
	int maxSize;
	int size;
	void** array;
} DynamicArray;

DynamicArray* dynamicArray(int growthFactor);
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
	Bucket* array[];
};

struct Bucket {
	char* id;
	int value;
	Bucket* next;
};

HashTable* hashTable(int size);
int getValue(HashTable* table, char* key);
int insertKeyPair(HashTable* table, char* key, int value);
int containsKey(HashTable* table, char* key);
int updateKeyPair(HashTable* table, char* key, int value);
void removeKey(HashTable* table, char* key);
void freeTable(HashTable* table);

#endif
