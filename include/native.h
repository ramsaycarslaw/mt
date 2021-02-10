#ifndef mt_native_h
#define mt_native_h

#include <dirent.h>
#include <time.h>

#include "object.h"
#include "value.h"
#include "vm.h"

Value clockNative(int argCount, Value *args);
Value sleepNative(int argCount, Value *args);
Value readNative(int argCount, Value *args);
Value writeNative(int argCount, Value *args);
Value randIntNative(int argcount, Value *args);
Value inputNative(int argCount, Value *args);
Value doubleNative(int argCount, Value *args);
Value stringNative(int argCount, Value *args);
Value exitNative(int argCount, Value *args);
Value clearNative(int argCount, Value *args);
Value showNative(int argCount, Value *args);
Value cdNative(int argCount, Value *args);
Value printfNative(int argCount, Value *args);
Value printlnNative(int argCount, Value *args);
Value colorSetNative(int argCount, Value *args);
Value bgSetNative(int argCount, Value *args);
Value appendNative(int argCount, Value *args);
Value deleteNative(int argCount, Value *args);
Value lenNative(int argCount, Value *args);
#endif
