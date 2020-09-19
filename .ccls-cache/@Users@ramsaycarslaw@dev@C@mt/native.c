#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native.h"

/* read a file for built ins */
static char* readFile(const char* path)
{
	FILE* file = fopen(path, "rb");
	if (file == NULL)
	{
		fprintf(stderr, "Could not open file \"%s\".\n", path);
		exit(74);
	}
	
	fseek(file, 0L, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file);
	
	char* buffer = (char*)malloc(fileSize + 1);
	if (buffer == NULL)
	{
		fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
		exit(74);
	}
	size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
	if (bytesRead < fileSize)
	{
		fprintf(stderr, "Could not read file \"%s\".\n", path);
		exit(74);
	}
	buffer[bytesRead] = '\0';
	
	fclose(file);
	return buffer;
}

/* Warn about fn misuse */
static void warn(int expected, int argCount, const char* name)
{
	printf("Expected %d arguments to %s(), got %d.\n", expected, name, argCount);
}


/* Provides the clock */
Value clockNative(int argCount, Value* args)
{
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

/* read a file */
Value readNative(int argCount, Value* args) 
{
	if (argCount != 1)
	{
		warn(1, argCount, "read");
	}
	const char* path = AS_CSTRING(args[0]);
	char *src = readFile(path);
	return OBJ_VAL(copyString(src, strlen(src)));
}

/* write to a file */
Value writeNative(int argCount, Value* args)
{
	if (argCount != 2)
	{
		warn(2, argCount, "write");
	}
}
