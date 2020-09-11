#include <stdio.h>

#include "compiler.h"
#include "common.h"
#include "scanner.h"

void compile(const char *src)
{
	initScanner(src);
	int line = -1;
	for (;;) {
		Token token = scanToken();
		if (token.line != line) {
			printf("%4d ", token.line);
			line = token.line;
		} else {
			printf("   | ");
		}
		printf("%2d '%.*s'\n", token.type, token.length, token.start); 
		
		if (token.type == TOKEN_EOF) break;
	}
}

