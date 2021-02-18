#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "assert.h"
#include "../include/vm.h"

/* Asserts that a value is true */
static Value assertIsTrue(int argCount, Value *args) 
{
  /* Check that we have at least one value */
  if (argCount == 0) 
  {
    runtimeError("Expected at least one argument to 'Assert.true' %d given.", argCount);
    return NIL_VAL;
  }

  /* Actual assertion */
  if (isFalsey(args[0]))
  {
    if (argCount == 2) 
    {
      char message[512];
      sprintf(message, "Could not assert %s to be true.", AS_CSTRING(args[1]));
      runtimeError(message);
    }
    else 
    {
      runtimeError("Fatal error in 'Assert.true', exiting...");
    }
    exit(70);
  }
  return NIL_VAL;
}


/* Asserts that a value is false */
static Value assertIsFalse(int argCount, Value *args) 
{
  /* Check that we have at least one value */
  if (argCount == 0) 
  {
    runtimeError("Expected at least one argument to 'Assert.false' %d given.", argCount);
    return NIL_VAL;
  }

  /* Actual assertion */
  if (!isFalsey(args[0]))
  {
    if (argCount == 2) 
    {
      char message[512];
      sprintf(message, "Could not assert %s to be true.", AS_CSTRING(args[1]));
      runtimeError(message);
    }
    else 
    {
      runtimeError("Fatal error in 'Assert.false', exiting...");
    }
    exit(70);
  }
  return NIL_VAL;
}

/* Finally we create the module */
void createAssertModule() 
{
  // name of the overall module
  ObjString* name = copyString("assert", 6);
  push(OBJ_VAL(name));

  // we use the name to create the object
  ObjNativeClass *klass = newNativeClass(name);
  push(OBJ_VAL(name));

  defineModuleMethod(klass, "True", assertIsTrue);
  defineModuleMethod(klass, "False", assertIsFalse);


  tableSet(&vm.globals, name, OBJ_VAL(klass));
  pop();
  pop();
}

