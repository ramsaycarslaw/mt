#include <math.h>
#include "math.h"

// --------------------------- OPERATIONS ---------------------------------

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

static Value powNative(int argCount, Value *args) 
{
  // Check we have the right number of arguments
  if (argCount != 2) 
  {
    runtimeError("Expected two arguments to 'math.Pow' %d given.", argCount);
    return NIL_VAL;
  }

  double base = AS_NUMBER(args[0]);
  double exponent = AS_NUMBER(args[1]);
  return NUMBER_VAL(pow(base, exponent));
}

// returns the square root of a number
static Value sqrtNative(int argCount, Value *args) 
{
  // Check we have the right number of arguments
  if (argCount != 1) 
  {
    runtimeError("Expected one argument to 'math.Sqrt' %d given.", argCount);
    return NIL_VAL;
  }

  double n = AS_NUMBER(args[0]);
  return NUMBER_VAL(sqrt(n));
}

// returns the absolute value of a number
static Value absNative(int argCount, Value *args) 
{
  // Check we have the right number of arguments
  if (argCount != 1) 
  {
    runtimeError("Expected one argument to 'math.Abs' %d given.", argCount);
    return NIL_VAL;
  }

  double n = AS_NUMBER(args[0]);
  return NUMBER_VAL(fabs(n));
}

// ---------------------- TRIG ------------------------------

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

// return the value of asin of the given angle
static Value asinNative(int argCount, Value *args) 
{
  // Check we have the right number of arguments
  if (argCount != 1) 
  {
    runtimeError("Expected one argument to 'math.Asin' %d given.", argCount);
    return NIL_VAL;
  }

  double radians = AS_NUMBER(args[0]);
  return NUMBER_VAL(asin(radians));
}

// return the value of acos of the given angle
static Value acosNative(int argCount, Value *args) 
{
  // Check we have the right number of arguments
  if (argCount != 1) 
  {
    runtimeError("Expected one argument to 'math.Acos' %d given.", argCount);
    return NIL_VAL;
  }

  double radians = AS_NUMBER(args[0]);
  return NUMBER_VAL(acos(radians));
}

// return the value of atan of the given angle
static Value atanNative(int argCount, Value *args) 
{
  // Check we have the right number of arguments
  if (argCount != 1) 
  {
    runtimeError("Expected one argument to 'math.Atan' %d given.", argCount);
    return NIL_VAL;
  }

  double radians = AS_NUMBER(args[0]);
  return NUMBER_VAL(atan(radians));
}

// ---------------------- CONSTANTS ------------------------------

// return 32 bit value of pi
static Value piNative(int argCount, Value *args) 
{
  // Check we have the right number of arguments
  if (argCount != 0) 
  {
    runtimeError("Expected no arguments to 'math.Pi' %d given.", argCount);
    return NIL_VAL;
  }

  return NUMBER_VAL(M_PI);
}

// return 32 bit value of e
static Value eNative(int argCount, Value *args) 
{
  // Check we have the right number of arguments
  if (argCount != 0) 
  {
    runtimeError("Expected no arguments to 'math.E' %d given.", argCount);
    return NIL_VAL;
  }

  return NUMBER_VAL(M_E);
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
  defineModuleMethod(klass, "Asin", asinNative);
  defineModuleMethod(klass, "Acos", acosNative);
  defineModuleMethod(klass, "Atan", atanNative);
  defineModuleMethod(klass, "Pi", piNative);
  defineModuleMethod(klass, "E", eNative);
  defineModuleMethod(klass, "Pow", powNative);
  defineModuleMethod(klass, "Sqrt", sqrtNative);
  defineModuleMethod(klass, "Abs", absNative);

  tableSet(&vm.globals, name, OBJ_VAL(klass));
  pop();
  pop();
}
