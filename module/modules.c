#include "modules.h"

/* Create native classses as well as functions */
void defineModuleMethod(ObjNativeClass* klass, const char* name, NativeFn function) 
{
    ObjNative *native = newNative(function);
    push(OBJ_VAL(native));
    ObjString *methodName = copyString(name, strlen(name));
    push(OBJ_VAL(methodName));
    tableSet(&klass->methods, methodName, OBJ_VAL(native));
    pop();
    pop();
}
