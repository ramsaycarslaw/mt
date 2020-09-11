#include <stdio.h>

#include "common.h"
#include "vm.h"
#include "debug.h"

#define MT_DEBUG_TRACE_EXEC // if on will print stuff for 'pro' users

/* Maybe take a pointer later to remove the global variable */
VM vm;

void initVM()
{
	
}

void freeVM()
{
	
}

static int run()
{
#define READ_BYTE() (*vm.ip++) // method to get the next byte
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

	for (;;)
	{
#ifdef MT_DEBUG_TRACE_EXEC
		disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
		uint8_t instruction;
		switch (instruction = READ_BYTE())
		{
		case OP_CONSTANT:
		{
			Value constant = READ_CONSTANT();
			printValue(constant);
			printf("\n");
			break;
		}
		case OP_RETURN:
		{
			return INTERPRET_OK;
		}
		}
	}
#undef READ_BYTE
#undef READ_CONSTANT
}

InterpretResult interpret(Chunk *chunk)
{
	vm.chunk = chunk;
	vm.ip = vm.chunk->code;
	return run();
}
