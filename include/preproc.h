#ifndef MT_PRE_PROC_H
#define MT_PRE_PROC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


char *readFile(const char* path);
int getImports(char *src);

#endif
