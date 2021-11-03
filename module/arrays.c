#include "arrays.h"
#include "../include/vm.h"
#include "../include/object.h"

Value lenNativeModule(int argCount, Value *args) {
    if (argCount != 1) {
        runtimeError("wrong number of arguments. got=%d, want=1", argCount);
        return NIL_VAL;
    }

    if (!IS_LIST(args[0])) {
        runtimeError("argument to `Len` not an array");
        return NIL_VAL;
    }

    return NUMBER_VAL(AS_LIST(args[0])->count);
}

// native reverse list
Value reverseNative(int argCount, Value *args) {
    if (argCount != 1) {
        runtimeError("wrong number of arguments. got=%d, want=1", argCount);
        return NIL_VAL;
    }

    if (!IS_LIST(args[0])) {
        runtimeError("argument to `Reverse` not an array");
        return NIL_VAL;
    }

    ObjList *list = AS_LIST(args[0]);
    ObjList *new = newList();

    for (int i = list->count - 1; i >= 0; i--) {
        appendToList(new, list->items[i]);
    }

    return OBJ_VAL(new);
}

// native push list
Value pushNative(int argCount, Value *args) {
    if (argCount != 2) {
        runtimeError("wrong number of arguments. got=%d, want=2", argCount);
        return NIL_VAL;
    }

    if (!IS_LIST(args[0])) {
        runtimeError("argument to `Push` not an array");
        return NIL_VAL;
    }

    ObjList *list = AS_LIST(args[0]);
    appendToList(list, args[1]);

    return args[0];
}

// native pop list
Value popNative(int argCount, Value *args) {
    if (argCount != 1) {
        runtimeError("wrong number of arguments. got=%d, want=1", argCount);
        return NIL_VAL;
    }

    if (!IS_LIST(args[0])) {
        runtimeError("argument to `Pop` not an array");
        return NIL_VAL;
    }

    ObjList *list = AS_LIST(args[0]);
    Value last = list->items[list->count - 1];
    list->count--;

    return last;
}

// native shift list
Value shiftNative(int argCount, Value *args) {
    if (argCount != 1) {
        runtimeError("wrong number of arguments. got=%d, want=1", argCount);
        return NIL_VAL;
    }

    if (!IS_LIST(args[0])) {
        runtimeError("argument to `Shift` not an array");
        return NIL_VAL;
    }

    ObjList *list = AS_LIST(args[0]);
    Value first = list->items[0];
    for (int i = 0; i < list->count - 1; i++) {
        list->items[i] = list->items[i + 1];
    }
    list->count--;

    return first;
}

// native unshift list
Value unshiftNative(int argCount, Value *args) {
    if (argCount != 2) {
        runtimeError("wrong number of arguments. got=%d, want=2", argCount);
        return NIL_VAL;
    }

    if (!IS_LIST(args[0])) {
        runtimeError("argument to `Unshift` not an array");
        return NIL_VAL;
    }

    ObjList *list = AS_LIST(args[0]);
    for (int i = list->count; i > 0; i--) {
        list->items[i] = list->items[i - 1];
    }
    list->items[0] = args[1];
    list->count++;

    return args[0];
}

// native slice list
Value sliceNative(int argCount, Value *args) {
    if (argCount != 3) {
        runtimeError("wrong number of arguments. got=%d, want=3", argCount);
        return NIL_VAL;
    }

    if (!IS_LIST(args[0])) {
        runtimeError("argument to `Slice` not an array");
        return NIL_VAL;
    }

    ObjList *list = AS_LIST(args[0]);
    int start = AS_NUMBER(args[1]);
    int end = AS_NUMBER(args[2]);

    if (start < 0) {
        start = list->count + start;
    }

    if (end < 0) {
        end = list->count + end;
    }

    if (start < 0) {
        start = 0;
    }

    if (end > list->count) {
        end = list->count;
    }

    if (start > end) {
        return NIL_VAL;
    }

    ObjList *new = newList();
    for (int i = start; i < end; i++) {
        appendToList(new, list->items[i]);
    }

    return OBJ_VAL(new);
}

// native function to create a new list populated with random values
Value randNative(int argCount, Value *args) {
    if (argCount != 1) {
        runtimeError("wrong number of arguments. got=%d, want=1", argCount);
        return NIL_VAL;
    }

    if (!IS_NUMBER(args[0])) {
        runtimeError("argument to `Rand` not a number");
        return NIL_VAL;
    }

    int size = AS_NUMBER(args[0]);
    if (size < 0) {
        runtimeError("argument to `Rand` must be greater than 0");
        return NIL_VAL;
    }

    ObjList *list = newList();
    for (int i = 0; i < size; i++) {
        appendToList(list, NUMBER_VAL(rand() % 100));
    }

    return OBJ_VAL(list);
}

void createArraysModule() 
{
  ObjString* name = copyString("arrays", 6);
  push(OBJ_VAL(name));

  // now create the runtime object
  ObjNativeClass *klass = newNativeClass(name);
  push(OBJ_VAL(name));

  defineModuleMethod(klass, "Len", lenNativeModule);
  defineModuleMethod(klass, "Reverse", reverseNative);
  defineModuleMethod(klass, "Push", pushNative);
  defineModuleMethod(klass, "Pop", popNative);
  defineModuleMethod(klass, "Shift", shiftNative);
  defineModuleMethod(klass, "Unshift", unshiftNative);
  defineModuleMethod(klass, "Slice", sliceNative);
  defineModuleMethod(klass, "Rand", randNative);

  tableSet(&vm.globals, name, OBJ_VAL(klass));
  pop();
  pop();
}