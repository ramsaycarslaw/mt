#ifndef mt_modules_driver
#define mt_modules_driver

#include <string.h>
#include "../include/vm.h"
#include "../include/value.h"

void defineModuleMethod(ObjNativeClass* klass, const char* name, NativeFn function);

#endif  // mt_modules_driver
