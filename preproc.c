#include "preproc.h"
#include "vm.h"

static char* readFile(const char* path)
{
	FILE* file;
  file = fopen(path, "rb");
	if (file == NULL)
	{
    char * mt_path = "$HOME/.mtlib/stdlib/";
    char * final_path = malloc(sizeof(char)*(strlen(mt_path)+strlen(path)));
    
    strcat(final_path, mt_path);
    strcat(final_path, path);

    file = fopen(final_path, "rb");

    if (file == NULL) 
    {
      printf("Your mt library path is not configured correctly, to fix this run:\n");
      printf("> mkdir -p ~/.mtlib/stdlib/\n");
		  fprintf(stderr, "Could not open file \"%s\".\n", final_path);
		  exit(74);
    }
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

/* Get all the imports from the file which should alloq us to run them */
int getImports(char *src) 
{
  int count = 0;
  /* Duplicate src so it is safe to use strtok */
  char * copy = malloc(sizeof(char)*strlen(src)+1);
  strcpy(copy, src);

  char *word; 
  word = strtok(copy, " ;\"\t\n");
  int imp = 0;
  while (word != NULL) 
  {
    // was the last word import ?
    if (imp) 
    {
      count++;
			interpret(readFile(word));
      imp = 0;
    }

    // is the current word import
    if (strcmp(word, "use") == 0) 
    {
      count++;
      imp = 1;
    }

    word = strtok(NULL, " ;\"\t\n");
  }
  free(copy);
  return count;
}

