#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "native.h"

#define BUFFERSIZE 256

// ------------------------------------------------------------
//                     System Natives                         |
// ------------------------------------------------------------

/* read a file for built ins */
static char *readFile(const char *path) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file \"%s\".\n", path);
    exit(74);
  }

  fseek(file, 0L, SEEK_END);
  size_t fileSize = ftell(file);
  rewind(file);

  char *buffer = (char *)malloc(fileSize + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
    exit(74);
  }
  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
  if (bytesRead < fileSize) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(74);
  }
  buffer[bytesRead] = '\0';

  fclose(file);
  return buffer;
}

/* Warn about fn misuse */
static void warn(int expected, int argCount, const char *name) {
  printf("Expected %d arguments to %s(), got %d.\n", expected, name, argCount);
}

/* Provides the clock */
Value clockNative(int argCount, Value *args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

/* read a file */
Value readNative(int argCount, Value *args) {
  if (argCount != 1) {
    warn(1, argCount, "read");
  }
  const char *path = AS_CSTRING(args[0]);
  char *src = readFile(path);
  return OBJ_VAL(copyString(src, strlen(src)));
}

/* write to a file */
Value writeNative(int argCount, Value *args) {
  if (argCount != 2) {
    warn(2, argCount, "write");
  }
  const char *path = AS_CSTRING(args[0]);
  const char *wrt = AS_CSTRING(args[1]);

  FILE *fptr;
  fptr = fopen(path, "w");

  if (fptr == NULL) {
    printf("Error wriiting to file.\n");
    return NUMBER_VAL(1);
  }

  fprintf(fptr, "%s", wrt);
  return NUMBER_VAL(0);
}

Value randIntNative(int argCount, Value *args) {
  if (argCount != 2) {
    warn(2, argCount, "randInt");
  }

  int l;
  int r;
  l = AS_NUMBER(args[0]);
  r = AS_NUMBER(args[1]);

  srand(time(0)); // get seed as time
  int rand_num = (rand() % (r - l + 1)) + l;
  return NUMBER_VAL(rand_num);
}

/* Get user input as string */
Value inputNative(int argCount, Value *args) {
  char out[255];

  if (argCount == 1) {
    const char *message = AS_CSTRING(args[0]);
    printf("%s", message);
  }

  scanf("%[^\n]s", out);

  return OBJ_VAL(copyString(out, strlen(out)));
}

/* Convert all other values to double */
Value doubleNative(int argCount, Value *args) {
  if (argCount != 1) {
    warn(1, argCount, "double");
  }

  switch (args[0].type) {
  case VAL_BOOL: {
    if (AS_BOOL(args[0])) {
      return NUMBER_VAL(1);
    }
    return NUMBER_VAL(0);
  }

  case VAL_NUMBER: {
    return NUMBER_VAL(AS_NUMBER(args[0]));
  }

  default: {
    char value[255];
    char *eptr;

    strcpy(value, AS_CSTRING(args[0]));

    return NUMBER_VAL(strtod(value, &eptr));
  }
  }
  return NUMBER_VAL(0);
}

/* Change all values into string */
Value stringNative(int argCount, Value *args) {
  char output[255];
  switch (args[0].type) {
  case VAL_BOOL:
    if (AS_BOOL(args[0])) {
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
Value exitNative(int argCount, Value *args) { exit(0); }

/* Clear the screen on POSIX systems */
Value clearNative(int argCount, Value *args) {
  printf("\e[1;1H\e[2J");
  return NUMBER_VAL(0);
}

/* License infomation for the REPL */
Value showNative(int argCount, Value *args) {
  if (argCount != 1)
    printf("Please select an option");
  const char *message = AS_CSTRING(args[0]);

  if (strcmp(message, "c") == 0) {
    printf("See https://www.gnu.org/licenses/gpl-3.0.en.html\n");
  } else if (strcmp(message, "w") == 0) {
    printf("See https://www.gnu.org/licenses/gpl-3.0.en.html\n");
  }
  return NUMBER_VAL(0);
}

/* functions like the unix `cd' command */
Value cdNative(int argCount, Value *args) {
  char *pth = AS_CSTRING(args[0]);

  char path[BUFFERSIZE];
  strcpy(path, pth);

  char cwd[BUFFERSIZE];
  if (pth[0] != '/') {
    getcwd(cwd, sizeof(cwd));
    strcat(cwd, "/");
    strcat(cwd, path);
    chdir(cwd);
  } else {
    chdir(pth);
  }

  return NUMBER_VAL(0);
}

// ------------------------------------------------------------
//                     Printing
// ------------------------------------------------------------

/* Helper function for printf */
static void printNewline(const char *string) {
  for (int i = 0; i < (int)strlen(string); i++) {
    if (string[i] == '\\') {
      switch (string[i + 1]) {
      case 'n':
        printf("\n");
        i += 2;
        break;
      case 't':
        printf("\t");
        i += 2;
        break;
      case 'r':
        printf("\r");
        i += 2;
        break;
      case 'e':
        printf("\e%c%c", string[i + 2], string[i + 3]);
        i += 4;
        break;
      }
    }
    printf("%c", string[i]);
  }
  return;
}

/* C style printf function */
Value printfNative(int argCount, Value *args) {
  /* Check we have the right number of args */
  if (argCount < 1) {
    printf("Nothing to print\n");
    exit(EXIT_FAILURE);
  }

  char *str = AS_CSTRING(args[0]);
  /*
   *  In this case no futher computation is required
   *  There are no other percent signs and as such print
   *  the first string
   */
  if (argCount == 1) {
    printNewline(str);
    return NUMBER_VAL(1);
  }

  /* Now we need to find all escape codes */
  int escp = 1;
  for (int i = 0; i < strlen(str); i++) {

    if (str[i] == '%') {
      switch (str[i + 1]) {
      case 'd':
        printf("%lf", AS_NUMBER(args[escp]));
        escp++;
        i += 2;
        break;
      case 's':
        printf("%s", AS_CSTRING(args[escp]));
        escp++;
        i += 2;
        break;
      }
    }

    if (str[i] == '\\') {
      switch (str[i + 1]) {
      case 'n':
        printf("\n");
        i += 2;
        break;
      case 't':
        printf("\t");
        i += 2;
        break;
      case 'r':
        printf("\r");
        i += 2;
        break;
      }
    }

    printf("%c", str[i]);
  }
  return NUMBER_VAL(0);
}

/* Identical to printf except adds a newline afterwards */
Value printlnNative(int argCount, Value *args) {
  Value r = printfNative(argCount, args);
  printf("\n");
  return r;
}

/* Print with more colors */
Value colorSetNative(int argCount, Value *args) {
  if (argCount < 2) {
    printf("Not enough arguments to pretty");
  }

  char *color = AS_CSTRING(args[0]);
  char *weight = AS_CSTRING(args[1]);

  int delim = 0;

  if (strcmp(weight, "bold") == 0 || strcmp("b", weight) == 0) {
    delim = 1;
  }

  if (strcmp(color, "red") == 0 || strcmp(color, "r") == 0) {
    printf("\033[%d;31m", delim);
  } else if (strcmp(color, "green") == 0 || strcmp(color, "g") == 0) {
    printf("\033[%d;32m", delim);
  } else if (strcmp(color, "yellow") == 0 || strcmp(color, "y") == 0) {
    printf("\033[%d;33m", delim);
  } else if (strcmp(color, "blue") == 0 || strcmp(color, "b") == 0) {
    printf("\033[%d;34m", delim);
  } else if (strcmp(color, "magenta") == 0 || strcmp(color, "m") == 0) {
    printf("\033[%d;35m", delim);
  } else if (strcmp(color, "cyan") == 0 || strcmp(color, "c") == 0) {
    printf("\033[%d;36m", delim);
  } else {
    printf("\033[0m");
  }
  return NUMBER_VAL(0);
}

// ------------------------------------------------------------
//                     Array Natives
// ------------------------------------------------------------

/* append to a list */
Value appendNative(int argCount, Value *args) {
  // Append a value to the end of a list increasing the list's length by 1
  if (argCount != 2 || !IS_LIST(args[0])) {
    printf("List index out of range.\n");
    exit(EXIT_FAILURE);
  }
  ObjList *list = AS_LIST(args[0]);
  Value item = args[1];
  appendToList(list, item);
  return NIL_VAL;
}

/* delete from a list */
Value deleteNative(int argCount, Value *args) {
  // Delete an item from a list at the given index.
  if (argCount != 2 || !IS_LIST(args[0]) || !IS_NUMBER(args[1])) {
    printf("List index out of range.\n");
    exit(EXIT_FAILURE);
  }

  ObjList *list = AS_LIST(args[0]);
  int index = AS_NUMBER(args[1]);

  if (!isValidListIndex(list, index)) {
    printf("List index out of range.\n");
    exit(EXIT_FAILURE);
  }

  deleteFromList(list, index);
  return NIL_VAL;
}

/* Get the length of the list */
Value lenNative(int argCount, Value *args) {
  if (argCount != 1 || (!IS_LIST(args[0]) && !IS_STRING(args[0]))) {
    printf("Cannot get length from no list/string object.\n");
    exit(EXIT_FAILURE);
  }

  if (IS_LIST(args[0])) {
    return NUMBER_VAL(AS_LIST(args[0])->count);
  }
  return NUMBER_VAL(strlen(AS_CSTRING(args[0])));
}
