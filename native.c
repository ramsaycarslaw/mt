#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "native.h"

#define BUFFERSIZE 256

// ------------------------------------------------------------
//                     System Natives                         |
// ------------------------------------------------------------

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
    const char * path = AS_CSTRING(args[0]);
    const char * wrt = AS_CSTRING(args[1]);

    FILE *fptr;
    fptr = fopen(path, "w");

    if (fptr == NULL) 
    {
        printf("Error wriiting to file.\n");
        return NUMBER_VAL(1);
    }

    fprintf(fptr, "%s", wrt);
    return NUMBER_VAL(0);
}

/* Get user input as string */
Value inputNative(int argCount, Value* args) 
{
    char out[255];

    if (argCount == 1) 
    {
        const char *message = AS_CSTRING(args[0]);
        printf("%s", message);
    }
    
    scanf("%[^\n]s", out);

    return OBJ_VAL(copyString(out, strlen(out)));
}

/* Convert all other values to double */
Value doubleNative(int argCount, Value* args) 
{
    if (argCount != 1) 
    {
        warn(1, argCount, "double");
    }
    
    switch (args[0].type) 
    {
        case VAL_BOOL:
        {
            if (AS_BOOL(args[0])) 
            {
                return NUMBER_VAL(1);
            }
            return NUMBER_VAL(0);
        }
        
        case VAL_NUMBER: 
        {
            return NUMBER_VAL(AS_NUMBER(args[0]));
        }    
    
        default: 
        {
            char value[255];
            char *eptr;

            strcpy(value, AS_CSTRING(args[0]));
    
            return NUMBER_VAL(strtod(value, &eptr));
        }
    }
    return NUMBER_VAL(0);
}

/* Change all values into string */
Value stringNative(int argCount, Value* args) 
{
    char output[255];
    switch (args[0].type) 
    {
        case VAL_BOOL:
            if (AS_BOOL(args[0])) 
            {
                return OBJ_VAL(copyString("true", 4));
            }
            return OBJ_VAL(copyString("false", 5));

        case VAL_OBJ:
            printf("Cannot assign object to string\n");
            break;

        default:
            
            snprintf(output, 255, "%f", AS_NUMBER(args[0]));
            return OBJ_VAL(copyString(output, 255));
    }
    // unreacable
    return NUMBER_VAL(0);
}

/* Halt execution */
Value exitNative(int argCount, Value *args)
{
    exit(0);
}

/* Clear the screen on POSIX systems */
Value clearNative(int argCount, Value *args)
{
    printf("\e[1;1H\e[2J");
    return NUMBER_VAL(0);
}

/* License infomation for the REPL */
Value showNative(int argCount, Value* args)
{
    if (argCount != 1)
	printf("Please select an option");
    const char * message = AS_CSTRING(args[0]);

    if (strcmp(message, "c") == 0)
    {
	printf("See https://www.gnu.org/licenses/gpl-3.0.en.html\n");
    }
    else if (strcmp(message, "w") == 0)
    {
	printf("See https://www.gnu.org/licenses/gpl-3.0.en.html\n");
    }
    return NUMBER_VAL(0);
}

/* functions like the unix `cd' command */
Value cdNative(int argCount, Value *args)
{
    char *pth = AS_CSTRING(args[0]);
    
    char path[BUFFERSIZE];
    strcpy(path,pth);

    char cwd[BUFFERSIZE];
    if(pth[0] != '/')
    {
        getcwd(cwd,sizeof(cwd));
        strcat(cwd,"/");
        strcat(cwd,path);
        chdir(cwd);
    }else{
        chdir(pth);
    }

    return NUMBER_VAL(0);
}

/* functions like the unix `ls' command */
Value lsNative(int argCount, Value *args)
{
    struct dirent **namelist;
    int n;
    if (argCount < 1 && argCount != 0)
    {
	printf("Too many arguments to call...\n");
	exit(EXIT_FAILURE);
    }
    else if (argCount == 0)
    {
      n = scandir(".", &namelist, NULL, alphasort);
    }
    else
    {
	n = scandir(AS_CSTRING(args[0]), &namelist, NULL, alphasort);
    }

    if (n < 0)
    {
	printf("Could not perform operation `scandir'.\n");
	exit(EXIT_FAILURE);
    }
    
    while (n--)
    {
	// could add color here with d_type
	printf("%s\n", namelist[n]->d_name);
	free(namelist[n]);
    }
    free(namelist);
    return NUMBER_VAL(0);
}
