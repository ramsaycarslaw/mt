#include "errors.h"

/* Allow users to trigger runtime errors */
static Value errorRaise(int argCount, Value *args)
{
  if (argCount != 1) 
  {
    runtimeError("Expected exatly 1 value to 'erros.Raise' got %d", argCount);
    return NIL_VAL;
  }

  if (!IS_STRING(args[0])) 
  {
    runtimeError("Expected string as argument to 'errors.Raise'");
    return NIL_VAL;
  }

  runtimeError(AS_CSTRING(args[0]));
  return NIL_VAL;
}

/* define the module */
void createErrorsModule() 
{ 
  ObjString* name = copyString("errors", 6);
  push(OBJ_VAL(name));

  ObjNativeClass *klass = newNativeClass(name);
  push(OBJ_VAL(name));

  defineModuleMethod(klass, "Raise", errorRaise);

  tableSet(&vm.globals, name, OBJ_VAL(klass));
  pop();
  pop();
}
