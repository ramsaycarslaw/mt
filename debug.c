#include <stdio.h>

#include "debug.h"

/* Give a chunk a name and view it in a human-readble way */
void disassembleChunk(Chunk* chunk, const char* name)
{
	printf(">== %s ==<\n", name);

	for ( int offset = 0; offset < chunk->count; )
	{
		offset = disassembleInstruction(chunk, offset);
	}
}

/* Subroutine used to print instruction opcode */
static int simpleInstruction(const char* name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

/* Subroutine used by disassembleChunk */
int disassembleInstruction(Chunk* chunk, int offset)
{
	printf("%04d ", offset);

	uint8_t instruction = chunk->code[offset];
	switch (instruction)
	{
	case OP_RETURN:
		return simpleInstruction("OP_RETURN", offset);
	default:
		printf("Unknown opcode %d\n", instruction);
		return offset + 1;
	}
}


