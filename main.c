#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

/* current version */
#define MT_VERSION "1.3.1"

/* TODO - look into using readline here to get last line and arrow keys */
static void repl() {
	char line[2056];
	printf("MT v%s Copyright (C) 2020 Ramsay Carslaw This program comes with ABSOLUTELY NO WARRANTY; for details type `show(w);'. This is free software, and you are welcome to redistribute it under certain conditions; type `show(c);' for details.\n", MT_VERSION);
	while (1)
	{
		printf("mt> ");
		
		if (!fgets(line, sizeof(line), stdin))
		{
			printf("\n");
			break;
		}

		if (line[0] == 'f' && line[1] == 'n')
		{
		    int scope = 0;
		    for (int i = 0; i < (int)strlen(line); i++)
		    {
                      switch (line[i]) {
                      case '{':
                        scope++;
                        break;
                      case '}':
                        scope--;
                        break;
                      default:
                        break;
                      }
                    }

		    while (scope != 0)
		    {
			char newline[256];

			for (int k=0;k<scope;k++)
			    printf("\t");
			
			if (!fgets(newline, sizeof(newline), stdin))
			{
			    printf("\n");
			    break;
			}

			strcat(line, newline);

			for (int j = 0; j < strlen(newline); j++)
			{
			    switch (newline[j]) {
			    case '{':
				scope++;
				break;
			    case '}':
				scope--;
				break;
			    default:
				break;
                      }
			}
		    }
		}
		
		interpret(line);
	}
}

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

static void runFile(const char* path)
{
	char* source = readFile(path);
	InterpretResult result = interpret(source);
	free(source); 
	
	if (result == INTERPRET_COMPILE_ERROR) exit(65);
	if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, char *argv[])
{
	initVM();

	if (argc == 1)
	{
		repl();
	}
	else if (argc == 2)
	{
		runFile(argv[1]);
	}
	else
	{    
	    FILE *target;
	    target = fopen("__devel_linker__.mt", "w");
	    
	    for (int i = argc-1; i> 0; i--)
	    {
		FILE *fptr;
		char ch;

		fptr = fopen(argv[i], "r");

		if (fptr == NULL)
		{
		    printf("File %s does not exist\n", argv[i]);
		    exit(0);
		}

		if (target == NULL)
		{
		    printf("Error linking files attempt recovery\n");
		    runFile(argv[1]);
		}

		while ((ch = fgetc(fptr)) != EOF)
		    fputc(ch,target);

		fclose(fptr);
	    }
	    fclose(target);
	    runFile("__devel_linker__.mt"); 	   
	    remove("__devel_linker__.mt");
	}
	
	freeVM();
	return 0;
}
