#ifndef mt_vm_h
#define mt_vm_h

#include "chunk.h"

/* Executes chunks */

/* Manage state of VM */
typedef struct {
	Chunk *chunk;
	uint8_t *ip; // instruction pointer
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(Chunk* chunk);

#endif
