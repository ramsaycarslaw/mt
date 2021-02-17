#ifndef mt_chunk_h
#define mt_chunk_h

#include "common.h"
#include "value.h"

/* All possible types of opcode */
typedef enum
{
    OP_CONSTANT, // a number
    OP_IMPORT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,      // used for expressions
    OP_GET_LOCAL,// get value of local varible
    OP_SET_LOCAL,// set the value of local variable
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_SET_PROPERTY,
    OP_GET_SUPER,
    OP_GET_PROPERTY,
    OP_BUILD_LIST, // []
    OP_GENERATE_LIST,
    OP_INDEX_SUBSCR, // [n]
    OP_STORE_SUBSCR, // [n] = n
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_NEGATE,   // -number
    OP_INCR,     // ++
    OP_PRINT,
    OP_USE,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_COPY,
    OP_INVOKE,
    OP_SUPER_INVOKE,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,
    OP_ADD,      // +
    OP_SUBTRACT, // -
    OP_MULTIPLY, // *
    OP_DIVIDE,   // /
    OP_NOT,
    OP_POW,      // ^
    OP_MOD,      // %
    OP_RETURN,   // return
    OP_CLASS,
    OP_INHERIT,
    OP_METHOD
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
