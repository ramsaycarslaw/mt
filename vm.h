#ifndef mt_vm_h
#define mt_vm_h

#include "chunk.h"
#include "value.h"
#include "table.h"

#define STACK_MAX 256 // could lead to a stack overflow

/* Executes chunks */

/* Manage state of VM */
typedef struct {
	Chunk *chunk;
	uint8_t *ip; // instruction pointer
	Value stack[STACK_MAX];
	Value *stackTop;
	Table globals;
	Table strings;
	
	Obj* objects;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char * src);
void push(Value value);
Value pop();

#endif
