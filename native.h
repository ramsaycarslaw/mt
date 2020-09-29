#ifndef mt_native_h
#define mt_native_h

#include <time.h>

#include "value.h"
#include "object.h"
#include "vm.h"

Value clockNative(int argCount, Value* args);
Value readNative(int argCount, Value* args);
Value writeNative(int argCount, Value* args);
Value inputNative(int argCount, Value* args);
Value doubleNative(int argCount, Value* args);
Value stringNative(int argCount, Value* args);
Value exitNative(int argCount, Value* args);
Value clearNative(int argCount, Value* args);
Value showNative(int argCount, Value* args);
Value cdNative(int argCount, Value *args);
Value lsNative(int argCount, Value *args);

#endif
