#include "../include/repl.h"

#define MT_RL_BUFSIZE 1024

// read a line from stdin
void repl_loop(void) 
{
    char *line;
    int status = 1;

    do 
    {
        printf("mt> ");
        line = mt_readline();
        
        interpret(line);

        free(line);
    } while (status);
    return;
}

// read a line from the terminal
char *mt_readline(void)
{
  int bufsize = MT_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(1);
  }

  while (1) {
    // Read a character
    c = getchar();

    if (c ==  '\x1b') 
    {
        printf("Escape detected\n");
    }

    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += MT_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "mt: allocation error\n");
        exit(1);
      }
    }
  }
}

