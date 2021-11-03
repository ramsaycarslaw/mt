#include "strings.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "../include/object.h"

static Value concatNative(int argCount, Value* args) {
  // check argument count
  if (argCount < 1) {
    return NIL_VAL;
  }

  // check if all arguments are strings
  for (int i = 0; i < argCount; i++) {
    if (!IS_STRING(args[i])) {
      runtimeError("concatenation of non-string values");
    }
  }

  // concatenate all strings
  int length = 0;
  for (int i = 0; i < argCount; i++) {
    length += strlen(AS_CSTRING(args[i]));
  }

  char* result = malloc(sizeof(char) * (length + 1));

  int offset = 0;
  for (int i = 0; i < argCount; i++) {
    int argLength = strlen(AS_CSTRING(args[i]));
    memcpy(result + offset, AS_CSTRING(args[i]), argLength);
    offset += argLength;
  }
  
  result[length] = '\0';

  return OBJ_VAL(copyString(result, length));
}

// native strlen method
static Value strlenNative(int argCount, Value* args) {
  if (argCount != 1) {
    runtimeError("wrong number of arguments to 'Len'");
  }

  if (!IS_STRING(args[0])) {
    runtimeError("argument to 'Len' must be a string");
  }

  return NUMBER_VAL(strlen(AS_CSTRING(args[0])));
}

// native substring method
static Value substringNative(int argCount, Value* args) {
  if (argCount != 3) {
    runtimeError("wrong number of arguments to 'Substring'");
  }

  if (!IS_STRING(args[0])) {
    runtimeError("first argument to 'Substring' must be a string");
  }

  if (!IS_NUMBER(args[1]) || !IS_NUMBER(args[2])) {
    runtimeError("second and third arguments to 'Substring' must be numbers");
  }

  int start = AS_NUMBER(args[1]);
  int end = AS_NUMBER(args[2]);

  if (start < 0 || end < 0) {
    runtimeError("start and end arguments to 'Substring' must be non-negative");
  }

  int length = strlen(AS_CSTRING(args[0]));

  if (start > length || end > length) {
    runtimeError("start and end arguments to 'Substring' must be within the bounds of the string");
  }

  char* result = malloc(sizeof(char) * (end - start + 2));
  memcpy(result, AS_CSTRING(args[0]) + start, end - start + 1);
  result[end - start + 1] = '\0';

  return OBJ_VAL(copyString(result, end - start + 1));
}

// native indexOf method
static Value indexOfNative(int argCount, Value* args) {
  if (argCount != 2) {
    runtimeError("wrong number of arguments to 'IndexOf'");
  }

  if (!IS_STRING(args[0])) {
    runtimeError("first argument to 'IndexOf' must be a string");
  }

  if (!IS_STRING(args[1])) {
    runtimeError("second argument to 'IndexOf' must be a string");
  }

  char* haystack = AS_CSTRING(args[0]);
  char* needle = AS_CSTRING(args[1]);

  int length = strlen(haystack);
  int needleLength = strlen(needle);

  for (int i = 0; i < length; i++) {
    if (strncmp(haystack + i, needle, needleLength) == 0) {
      return NUMBER_VAL(i);
    }
  }

  return NUMBER_VAL(-1);
}

// native replace method
static Value replaceNative(int argCount, Value* args) {
  if (argCount != 3) {
    runtimeError("wrong number of arguments to 'Replace'");
  }

  if (!IS_STRING(args[0])) {
    runtimeError("first argument to 'Replace' must be a string");
  }

  if (!IS_STRING(args[1])) {
    runtimeError("second argument to 'Replace' must be a string");
  }

  if (!IS_STRING(args[2])) {
    runtimeError("third argument to 'Replace' must be a string");
  }

  char* haystack = AS_CSTRING(args[0]);
  char* needle = AS_CSTRING(args[1]);
  char* replacement = AS_CSTRING(args[2]);

  int length = strlen(haystack);
  int needleLength = strlen(needle);
  int replacementLength = strlen(replacement);

  char* result = malloc(sizeof(char) * (length + 1));

  int offset = 0;
  for (int i = 0; i < length; i++) {
    if (strncmp(haystack + i, needle, needleLength) == 0) {
      memcpy(result + offset, replacement, replacementLength);
      offset += replacementLength;
      i += needleLength - 1;
    } else {
      result[offset] = haystack[i];
      offset++;
    }
  }

  result[offset] = '\0';

  return OBJ_VAL(copyString(result, offset));
}

// native toLower method
static Value toLowerNative(int argCount, Value* args) {
  if (argCount != 1) {
    runtimeError("wrong number of arguments to 'Lower'");
  }

  if (!IS_STRING(args[0])) {
    runtimeError("argument to 'Lower' must be a string");
  }

  char* string = AS_CSTRING(args[0]);
  int length = strlen(string);

  char* result = malloc(sizeof(char) * (length + 1));

  for (int i = 0; i < length; i++) {
    result[i] = tolower(string[i]);
  }

  result[length] = '\0';

  return OBJ_VAL(copyString(result, length));
}

// native toUpper method
static Value toUpperNative(int argCount, Value* args) {
  if (argCount != 1) {
    runtimeError("wrong number of arguments to 'Upper'");
  }

  if (!IS_STRING(args[0])) {
    runtimeError("argument to 'Upper' must be a string");
  }

  char* string = AS_CSTRING(args[0]);
  int length = strlen(string);

  char* result = malloc(sizeof(char) * (length + 1));

  for (int i = 0; i < length; i++) {
    result[i] = toupper(string[i]);
  }

  result[length] = '\0';

  return OBJ_VAL(copyString(result, length));
}

// native trim method
static Value trimNative(int argCount, Value* args) {
  if (argCount != 1) {
    runtimeError("wrong number of arguments to 'Trim'");
  }

  if (!IS_STRING(args[0])) {
    runtimeError("argument to 'Trim' must be a string");
  }

  char* string = AS_CSTRING(args[0]);
  int length = strlen(string);

  char* result = malloc(sizeof(char) * (length + 1));

  int offset = 0;
  for (int i = 0; i < length; i++) {
    if (string[i] != ' ' && string[i] != '\t' && string[i] != '\n' && string[i] != '\r') {
      result[offset] = string[i];
      offset++;
    }
  }

  result[offset] = '\0';

  return OBJ_VAL(copyString(result, offset));
}

// native split method
static Value splitNative(int argCount, Value* args) {
  if (argCount != 2) {
    runtimeError("wrong number of arguments to 'Split'");
  }

  if (!IS_STRING(args[0])) {
    runtimeError("first argument to 'Split' must be a string");
  }

  if (!IS_STRING(args[1])) {
    runtimeError("second argument to 'Split' must be a string");
  }

  char* string = AS_CSTRING(args[0]);
  char* delimiter = AS_CSTRING(args[1]);

  int length = strlen(string);
  int delimiterLength = strlen(delimiter);

  int count = 0;
  for (int i = 0; i < length; i++) {
    if (strncmp(string + i, delimiter, delimiterLength) == 0) {
      count++;
      i += delimiterLength - 1;
    }
  }

  ObjList* result = newList();

  int offset = 0;
  for (int i = 0; i < length; i++) {
    if (strncmp(string + i, delimiter, delimiterLength) == 0) {
      appendToList(result, OBJ_VAL(copyString(string + offset, i - offset)));
      offset = i + delimiterLength;
      i += delimiterLength - 1;
    }
  }

  appendToList(result, OBJ_VAL(copyString(string + offset, length - offset)));

  return OBJ_VAL(result);
}

// native toString method
static Value toStringNative(int argCount, Value* args) {
  if (argCount != 1) {
    runtimeError("wrong number of arguments to 'ToString'");
  }

  if (!IS_NUMBER(args[0])) {
    runtimeError("argument to 'ToString' must be a number");
  }

  char buffer[32];
  snprintf(buffer, 32, "%g", AS_NUMBER(args[0]));

  return OBJ_VAL(copyString(buffer, strlen(buffer)));
}


void createStringsModule() {
  ObjString* name = copyString("strings", 7);
  push(OBJ_VAL(name));

  // now create the runtime object
  ObjNativeClass *klass = newNativeClass(name);
  push(OBJ_VAL(name));

  defineModuleMethod(klass, "Concat", concatNative);
  defineModuleMethod(klass, "Len", strlenNative);
  defineModuleMethod(klass, "Substring", substringNative);
  defineModuleMethod(klass, "IndexOf", indexOfNative);
  defineModuleMethod(klass, "Replace", replaceNative);
  defineModuleMethod(klass, "Lower", toLowerNative);
  defineModuleMethod(klass, "Upper", toUpperNative);
  defineModuleMethod(klass, "Trim", trimNative);
  defineModuleMethod(klass, "Split", splitNative);
  defineModuleMethod(klass, "ToString", toStringNative);

  tableSet(&vm.globals, name, OBJ_VAL(klass));
  pop();
  pop();
}