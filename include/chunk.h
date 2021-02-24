#ifndef mt_chunk_h
#define mt_chunk_h

#include "common.h"
#include "value.h"

/* All possible types of opcode */
typedef enum
{
    OP_ADD,      // +
    OP_BUILD_LIST, // []
    OP_BUILD_TUPLE, // ()
    OP_CALL,
    OP_CLASS,
    OP_CLOSE_UPVALUE,
    OP_CLOSURE,
    OP_CONSTANT, // a number
    OP_COPY,
    OP_DEFER,    // defer
    OP_DEFINE_GLOBAL,
    OP_DIVIDE,   // /
    OP_EQUAL,
    OP_FALSE,
    OP_FOR_ITERATOR,
    OP_GENERATE_LIST,
    OP_GET_GLOBAL,
    OP_GET_LOCAL,// get value of local varible
    OP_GET_PROPERTY,
    OP_GET_SUPER,
    OP_GET_UPVALUE,
    OP_GREATER,
    OP_IMPORT,
    OP_ITERATOR,
    OP_INCR,     // ++
    OP_INDEX_SUBSCR, // [n]
    OP_INHERIT,
    OP_INVOKE,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LESS,
    OP_LOOP,
    OP_METHOD,
    OP_MOD,      // %
    OP_MULTIPLY, // *
    OP_NEGATE,   // -number
    OP_NIL,
    OP_NOT,
    OP_POP,      // used for expressions
    OP_POW,      // ^
    OP_PRINT,
    OP_RANGE,    // range 0..n
    OP_RETURN,   // return
    OP_SET_GLOBAL,
    OP_SET_LOCAL,// set the value of local variable
    OP_SET_PROPERTY,
    OP_SET_UPVALUE,
    OP_STORE_SUBSCR, // [n] = n
    OP_SUBTRACT, // -
    OP_SUPER_INVOKE,
    OP_TRUE,
    OP_TYPE_ASSIGNMENT_ERROR,
    OP_TYPE_SET,
    OP_USE,
    OP_USE_ALL,
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
