#ifndef mt_chunk_h
#define mt_chunk_h

#include "common.h"
#include "value.h"

/* All possible types of opcode */
typedef enum
{
	OP_CONSTANT, // a number
	OP_NEGATE,   // -number
	OP_ADD,      // +
	OP_SUBTRACT, // -
	OP_MULTIPLY, // *
	OP_DIVIDE,   // /
	OP_POW,      // ^
	OP_RETURN,   // return
} OpCode;

/* Byte code chunk definition: wrapper for dynamic array */
typedef struct
{
	int count;
	int capacity;
	uint8_t* code;
	int *lines; // parralles bytecode array to keep track of linum
	ValueArray constants;
} Chunk;

/* Initialises a new bytecode chunk */
void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
/* Convienience function */
int addConstant(Chunk* chunk, Value value);

#endif
