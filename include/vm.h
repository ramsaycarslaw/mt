#ifndef mt_vm_h
#define mt_vm_h

#include "chunk.h"
#include "object.h"
#include "table.h"
#include "value.h"

#include "../module/assert.h"
#include "../module/http.h"
#include "../module/log.h"
#include "../module/errors.h"
#include "../module/sorts.h"

#define FRAMES_MAX 1024
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

/* Manages call frames/stack for the VM */
typedef struct {
  ObjClosure* closure;
  uint8_t *ip;
  Value *slots;
} CallFrame;

/* Executes chunks */

/* Manage state of VM */
typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frameCount;

  Value stack[STACK_MAX];
  Value *stackTop;

  const char *fileName;

  Table globals;
  Table strings;
  Table imports;

  ObjString* initString; // used to initialise functions
  ObjUpvalue* openUpvalues;

  Obj *objects;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;
void runtimeError(const char *format, ...);
bool isFalsey(Value value);
void initVM();
void freeVM();
InterpretResult interpretModule(const char *source) ;
InterpretResult interpret(const char *src);
void push(Value value);
Value pop();

#endif
