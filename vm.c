#include <stdio.h>
#include <math.h>

#include "common.h"
#include "compiler.h"
#include "vm.h"
#include "debug.h"

/* Maybe take a pointer later to remove the global variable */
VM vm;

/* 
Since the stack array is declared directly inline in the VM struct, we don’t need to allocate it. We don’t even need to clear the unused cells in the array—we simply won’t access them until after values have been stored in them. The only initialization we need is to set stackTop to point to the beginning of the array to indicate that the stack is empty.
*/
static void resetStack()
{
	vm.stackTop = vm.stack;
}

void initVM()
{
	resetStack();
}

void freeVM()
{
	
}

static int run()
{
#define READ_BYTE() (*vm.ip++) // method to get the next byte
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op) \
    do { \
      double b = pop(); \
      double a = pop(); \
      push(a op b); \
    } while (false)

	for (;;)
	{
#ifdef MT_DEBUG_TRACE_EXEC
		printf("       ");
		for (Value* slot = vm.stack; slot < vm.stackTop; slot++)
		{
			printf("[ ");
			printValue(*slot);
			printf(" ]");
		}
		printf("\n");
		disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
		uint8_t instruction;
		switch (instruction = READ_BYTE())
		{
		case OP_CONSTANT:
		{
			Value constant = READ_CONSTANT();
			push(constant);
			break;
		}
		case OP_ADD:      BINARY_OP(+); break;
		case OP_SUBTRACT: BINARY_OP(-); break;
		case OP_MULTIPLY: BINARY_OP(*); break;
		case OP_DIVIDE:   BINARY_OP(/); break;
		case OP_POW:
		{
			double a = pop();
			double b = pop();
			push(pow(a, b));
		}
		case OP_NEGATE:
		{
			push(-pop());
			break;
		}
		case OP_RETURN:
		{
			printValue(pop());
			printf("\n");
			return INTERPRET_OK;
		}
		}
	}
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult interpret(const char* source)
{
	Chunk chunk;
	initChunk(&chunk);

	if (!compile(source, &chunk))
	{
		freeChunk(&chunk);
		return INTERPRET_COMPILE_ERROR;
	}

	vm.chunk = &chunk;
	vm.ip = vm.chunk->code;

	InterpretResult result = run();
	
	freeChunk(&chunk);
	return result;
}

void push(Value value)
{
	*vm.stackTop = value;
	vm.stackTop++;
}

Value pop()
{
	vm.stackTop--;
	return *vm.stackTop;
}
