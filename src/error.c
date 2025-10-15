#include "../include/error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *CURRENT_FILE_PATH = "<source_file>";

static char codeBuffer[5];

const char *getErrorCodeString(ErrorCode code) {
  int numeric_code = (int)code;
  snprintf(codeBuffer, sizeof(codeBuffer), "E%d", numeric_code);

  return codeBuffer;
}

void reportError(const Token *token, ErrorCode errorCode,
                 const char *errorMessage) {
  fprintf(stderr, "\n\033[1;31merror\033[0m\033[1m[%s]\033[0m: %s\n",
          getErrorCodeString(errorCode), errorMessage);

  int column_offset = (int)(token->start - token->line_chars);

  fprintf(stderr, " \033[1;34m-->\033[0m %s:%d:%d\n", CURRENT_FILE_PATH,
          token->line, column_offset + 1);

  const char *lineStart = token->line_chars;

  const char *lineEnd = lineStart;
  while (*lineEnd != '\n' && *lineEnd != '\0') {
    lineEnd++;
  }

  int line_number_width = snprintf(NULL, 0, "%d", token->line);
  if (line_number_width < 0)
    line_number_width = 4;

  fprintf(stderr, "\033[1m%*d\033[0m | ", line_number_width, token->line);

  fprintf(stderr, "%.*s\n", (int)(lineEnd - lineStart), lineStart);

  int padding_width = line_number_width + 3;

  fprintf(stderr, "%*s|", padding_width - 2, " ");

  fprintf(stderr, "%*s", column_offset, "");

  fprintf(stderr, "\033[1;31m");
  for (int i = 0; i < token->length; i++) {
    fprintf(stderr, "^");
  }
  fprintf(stderr, "\033[0m\n");

  fprintf(stderr, "%*s=\033[1m ", padding_width - 2, " ");

  fprintf(stderr, "%s\033[0m\n", errorMessage);
}
