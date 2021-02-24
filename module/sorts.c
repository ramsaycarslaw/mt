#include "sorts.h"

static Value bubbleSortNative(int argCount, Value* args) {
  return NIL_VAL;
}

/* Finally we create the module */
void createSortsModule() 
{
  // name of the overall module
  ObjString* name = copyString("sorts", 5);
  push(OBJ_VAL(name));


  // we use the name to create the object
  ObjNativeClass *klass = newNativeClass(name);
  push(OBJ_VAL(name));

  defineModuleMethod(klass, "Bubble", bubbleSortNative);

  tableSet(&vm.globals, name, OBJ_VAL(klass));
  pop();
  pop();
}

