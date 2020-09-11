#ifndef mt_chunk_h
#define mt_chunk_h

#include "common.h"

/* All possible types of opcode */
typedef enum {
	OP_RETURN,
} OpCode;

/* Byte code chunk definition: wrapper for dynamic array */
typedef struct {
	int count;
	int capacity;
	uint8_t* code;
} Chunk;

/* Initialises a new bytecode chunk */
void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte);

#endif
