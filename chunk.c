#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

/* Initialise a new chunk to zero values */
void initChunk(Chunk *chunk)
{
	chunk->count = 0;
	chunk->capacity = 0;
	chunk->code = NULL;
}

/* Free the memory used by a chunk */
void freeChunk(Chunk *chunk)
{
	FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
	initChunk(chunk);
}

/* Write a byte to a chunk */
void writeChunk(Chunk *chunk, uint8_t byte)
{
	/* If the array is too small use preprocessor macros in
	 memory.h to increase it's capacity */
	if (chunk->capacity < chunk->count + 1)
	{
		int oldCapacity = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(oldCapacity);
		chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
	}
	chunk->code[chunk->count] = byte;
	chunk->count++;
}
