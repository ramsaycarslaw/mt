#include <math.h>
#include "math.h"

// factorial method
static Value factorial(int argCount, Value *args) 
{
  // Check we have the right number of arguments
  if (argCount != 1) 
  {
    runtimeError("Expected one argument to 'math.Fac' %d given.", argCount);
    return NIL_VAL;
  }

  int n = AS_NUMBER(args[0]);
  long int result = 1;

  for (int i = 1; i <= n; i++) 
  {
    result *= i;
  }

  return NUMBER_VAL(result);
}

// return the value of sin of the given angle
static Value sinNative(int argCount, Value *args) 
{
  // Check we have the right number of arguments
  if (argCount != 1) 
  {
    runtimeError("Expected one argument to 'math.Sin' %d given.", argCount);
    return NIL_VAL;
  }

  double radians = AS_NUMBER(args[0]);
  return NUMBER_VAL(sin(radians));
}

// return the value of cos of the given angle
static Value cosNative(int argCount, Value *args) 
{
  // Check we have the right number of arguments
  if (argCount != 1) 
  {
    runtimeError("Expected one argument to 'math.Cos' %d given.", argCount);
    return NIL_VAL;
  }

  double radians = AS_NUMBER(args[0]);
  return NUMBER_VAL(cos(radians));
}

// return the value of tan of the given angle
static Value tanNative(int argCount, Value *args) 
{
  // Check we have the right number of arguments
  if (argCount != 1) 
  {
    runtimeError("Expected one argument to 'math.Tan' %d given.", argCount);
    return NIL_VAL;
  }

  double radians = AS_NUMBER(args[0]);
  return NUMBER_VAL(tan(radians));
}

void createMathModule()
{
  // set up the name of the module
  ObjString* name = copyString("math", 4);
  push(OBJ_VAL(name));

  // now create the runtime object
  ObjNativeClass *klass = newNativeClass(name);
  push(OBJ_VAL(name));

  defineModuleMethod(klass, "Fac", factorial);
  defineModuleMethod(klass, "Sin", sinNative);
  defineModuleMethod(klass, "Cos", cosNative);
  defineModuleMethod(klass, "Tan", tanNative);

  tableSet(&vm.globals, name, OBJ_VAL(klass));
  pop();
  pop();
}
