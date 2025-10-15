#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Local libraries */
#include "../include/chunk.h"
#include "../include/common.h"
#include "../include/debug.h"
#include "../include/error.h"
#include "../include/preproc.h"
#include "../include/repl.h"
#include "../include/vm.h"

/* current version */
#define MT_VERSION "1.3.3"

/* TODO - look into using readline here to get last line and arrow keys */
static void repl() {
  repl_loop();

  return;
}

/* Reads a literate value from the source code
 * Used to handle use statements */
static char *readLiterate(const char *src) {
  printf("Entering literate mode\n");
  char *code = malloc(sizeof(char) * 100000000);
  int place = 0;
  int in = 0;
  for (int i = 0; i < (int)strlen(src) - 4; i++) {
    if (src[i] == '-' && src[i + 1] == '-' && src[i + 2] == '-') {
      // toggle in variable
      in = (in == 1) ? 0 : 1;
      i += 3;
    }

    if (in) {
      code[place] = src[i];
      place++;
    }
  }
  return code;
}

static void runFile(const char *path) {
  CURRENT_FILE_PATH = path;
  int len = (int)strlen(path);

  char *source = readFile(path);

  if (path[len - 1] == 'l') {
    source = readLiterate(source);
  }

  // getImports(source);

  InterpretResult result = interpret(source);
  free(source);

  if (result == INTERPRET_COMPILE_ERROR)
    exit(65);
  if (result == INTERPRET_RUNTIME_ERROR)
    exit(70);
}

/* Main loop that handles all the command line arguments and such */
int main(int argc, char *argv[]) {
  initVM(argv[1]);

  if (argc == 1) {
    CURRENT_FILE_PATH = "repl";
    repl();
  } else if (argc == 2) {
    runFile(argv[1]);
  } else

    freeVM();
  return 0;
}
