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

/* Check that two values are equal */
static Value assertEqualNative(int argCount, Value *args) 
{
  /* Check we have at least one value */
  if (argCount < 2) 
  {
    runtimeError("Expected at least 2 arguments to 'assert.Equals' %d given.", argCount);
    return NIL_VAL;
  }

  /* True until proven fasle */
  bool result = true;

  /* iterate over all the results */
  for (int i = 1; i < argCount; i++) 
  {
    Value a = args[i-1];
    Value b = args[i];

    result = result && valuesEqual(a, b);
  }

  if (!result) 
  {
    runtimeError("Could not assert all values to be equal.");
    exit(70);
  }

  return NIL_VAL;
}

static Value assertNumber(int argCount, Value *args) 
{
  if (argCount == 0) {
    runtimeError("Expected at least one argument to 'assert.Number'.");
    return NIL_VAL;
  }

  int result = 1;

  for (int i = 0; i < argCount; i++) {
    result = result && IS_NUMBER(args[i]);
  }
    
  if (!result) 
  {
    runtimeError("Could not assert all values to be numberss");
    return NIL_VAL;
  }

  return NIL_VAL;
}

// assert.String(x,y,z,a,b,c);
static Value assertString(int argCount, Value *args) 
{
  if (argCount == 0) {
    runtimeError("Expected at least one argument to 'assert.String'");
    return NIL_VAL;
  }

  bool result = true;

  for (int i = 0; i < argCount; i++) {
    result = result && IS_STRING(args[i]);
  }

  if (!result) {
    runtimeError("Could not assert all the values to be strings.");
    return NIL_VAL;
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
  defineModuleMethod(klass, "Equals", assertEqualNative);
  defineModuleMethod(klass, "Number", assertNumber);
  defineModuleMethod(klass, "String", assertString);


  tableSet(&vm.globals, name, OBJ_VAL(klass));
  pop();
  pop();
}

