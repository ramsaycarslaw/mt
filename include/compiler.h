#ifndef mt_compiler_h
#define mt_compiler_h

#include "vm.h"

typedef enum 
{
  NO_TYPE,
  NUMBER_TYPE,
  STRING_TYPE,
  BOOL_TYPE,
} Type;

ObjFunction* compile(const char * src, bool andRun);

#endif
