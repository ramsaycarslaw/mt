#include "../include/preproc.h"
#include "../include/vm.h"

/* Read a file frim the given path */
char* readFile(const char* path) {
  FILE* file = fopen(path, "rb");

  if (file == NULL) {
    printf("Could not open file, file doesn't exist.\n");
    exit(74);
  }

  // Find out how big the file is
  fseek(file, 0L, SEEK_END);
  size_t fileSize = ftell(file);
  rewind(file);

  // Allocate a buffer for it
  char* buffer = (char*)malloc(fileSize + 1);

  if (buffer == NULL) {
    printf("Insufficient memory to read file.\n");
    exit(74);
  }

  // Read the entire file
  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);

  if (bytesRead < fileSize) {
    printf("Could not read file\n");
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

