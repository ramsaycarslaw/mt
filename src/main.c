#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"
#include "repl.h"
#include "preproc.h"

/* current version */
#define MT_VERSION "1.3.2"

/* TODO - look into using readline here to get last line and arrow keys */
static void repl() 
{
    repl_loop();

    return;    
}

/* Reads a literate value from the source code 
 * Used to handle use statements */
static char *readLiterate(const char * src)
{
  printf("Entering literate mode\n");
  char *code = malloc(sizeof(char)*100000000);
  int place = 0;
  int in = 0;
  for (int i = 0; i < (int)strlen(src)-4; i++)
  {
    if (src[i] == '-' && src[i+1] == '-' && src[i+2] == '-')
    {
      // toggle in variable
      in = (in == 1) ? 0 : 1;
      i += 3;
    }

    if (in)
    {
      code[place] = src[i];
      place++;
    }
  }
  return code;
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
  int len = (int) strlen(path);

  char* source = readFile(path);
  
  if (path[len-1] == 'l')
  {
    source = readLiterate(source);
  }
  getImports(source);
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
