#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

char *parseArgs(int argc, char *argv[]) {

	// Currently only one argument (the about to be compiled file) is accepted
	// TODO: Add check for custom file extension to ONLY compile files with that extension
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <filename> \n", argv[0]);
		exit(EXIT_FAILURE);
	}

	return argv[1];
}

char *readFileToBuffer(char *filepath) {

    FILE *file = fopen(filepath, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Error allocating memory for file buffer.\n");
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        fprintf(stderr, "Error reading file.\n");
        fclose(file);
        free(buffer);
        return NULL;
    }

    buffer[file_size] = '\0';

    fclose(file);
    return buffer;
}

int main(int argc, char *argv[]) {

	char *filepath = parseArgs(argc, argv);
	char *buffer = readFileToBuffer(filepath);

	if (buffer == NULL) {
		return 1;
	}

	initializeLexer(buffer);

	Token *token = getToken();

	while (token->type != E_O_F) {
		printf("|%.*s", token->length, token->start);;
		token = getToken();
	}
}
