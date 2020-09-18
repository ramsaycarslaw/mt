#ifndef mt_native_h
#define mt_native_h

#include <time.h>

#include "value.h"

Value clockNative(int argCount, Value* args);
Value readNative(int argCount, Value* args);

#endif
