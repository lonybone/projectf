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

void* getStmt(DynamicArray* dynamicArray, int idx) {
	if (idx < 0 || idx >= dynamicArray->size) {
		fprintf(stderr, "index out of range for dynamic Array");
		return NULL;
	}
	return dynamicArray->array[idx];
}

bool appendStmt(DynamicArray *dynamicArray, void *statement) {
	dynamicArray->array[dynamicArray->size++] = statement;

	if (dynamicArray->size == dynamicArray->maxSize) {

		dynamicArray->maxSize *= dynamicArray->growthFactor;
		void** tmp = realloc(dynamicArray->array, dynamicArray->maxSize * sizeof(void*));

		if (tmp == NULL) {
			fprintf(stderr, "failed to allocate static array in dynamic array");
			return false;
		}

		dynamicArray->array = tmp;
	}

	return true;
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

Operation insertKeyPair(HashTable *table, char *key, char *value) {
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
	newBucket->value = strdup(value);
	if (newBucket->id == NULL || newBucket->value == NULL) {
		free(newBucket->id);
		free(newBucket->value);
		free(newBucket);
		return ALLOC_FAIL;
	}

	newBucket->next = table->array[hashedKey];
	table->array[hashedKey] = newBucket;
	return SUCCESS;
}

Operation updateKeyPair(HashTable *table, char *key, char *value) {
	unsigned long hashedKey = hash((unsigned char*)key) % table->size;

	Bucket* current = table->array[hashedKey];
	while (current != NULL) {
		if (strcmp(current->id, key) == 0) {
			char* newValue = strdup(value);
			
			if (newValue == NULL) {
				fprintf(stderr, "Error: failed to allocate new value %s for key %s in updateKeyPair", value, key);
				return ALLOC_FAIL;
			}

			free(current->value);
			current->value = newValue;
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
			free(current->value);
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
			free(current->value);

			previous = current;
			current = current->next;

			free(previous);
		}
	}

	free(table);
}

/*
void printTable(HashTable* table) {
    printf("\n--- HASH TABLE CONTENTS ---\n");
    for (int i = 0; i < table->size; i++) {
        printf("Bucket %d: ", i);
        Bucket* current = table->array[i];
        if (current == NULL) {
            printf("~empty~\n");
        } else {
            while (current != NULL) {
                printf("[\"%s\": \"%s\"] -> ", current->id, current->value);
                current = current->next;
            }
            printf("NULL\n");
        }
    }
    printf("---------------------------\n\n");
}

int main() {
    printf("🧪 Initializing Hash Table with size 5...\n");
    HashTable* myTable = hashTable(5);
    if (!myTable) {
        return 1; // Exit if allocation fails
    }

    printTable(myTable);

    printf("➡️ Inserting key-value pairs...\n");
    insertKeyPair(myTable, "name", "Gandalf");
    insertKeyPair(myTable, "race", "Maia");
    insertKeyPair(myTable, "item", "Glamdring");
    insertKeyPair(myTable, "alias", "Mithrandir"); // This will likely collide with "race"
    insertKeyPair(myTable, "enemy", "Balrog");

    printTable(myTable);

    printf("⚠️ Attempting to insert a duplicate key ('name')...\n");
    insertKeyPair(myTable, "name", "Sauron"); // Should fail and print an error

    printf("\n🔄 Updating value for key 'item'...\n");
    updateKeyPair(myTable, "item", "Staff of Power");
    printTable(myTable);
    
    printf("🔄 Attempting to update non-existent key 'location'...\n");
    if (updateKeyPair(myTable, "location", "Middle-earth") == DOESNT_EXIST) {
        printf("   (Successfully detected that key does not exist)\n");
    }


    printf("\n🗑️ Removing key 'race' (head of a list)...\n");
    removeKey(myTable, "race");
    printTable(myTable);

    printf("🗑️ Removing key 'item'...\n");
    removeKey(myTable, "item");
    printTable(myTable);

    printf("\n🧹 Freeing all table buckets and nodes...\n");
    freeTable(myTable);
    printf("✅ Testing complete. Memory freed.\n");

    return 0;
}
*/

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
