#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "common.h"
#include "scanner.h"

#ifdef MT_DEBUG_PRINT_CODE
#include "debug.h"
#endif

/* main struct to store the parser */
typedef struct {
	Token current;
	Token previous;
	int hadError;
	int panicMode;
} Parser;

/* Token priority to the parser */
typedef enum
{
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

/* A ParseRule stores what function is needed to compile each token */
typedef struct
{
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

Parser parser;

/* The current chunk under isnpection */
Chunk* compilingChunk;

/* A get method for compling chunk */
static Chunk* currentChunk()
{
  return compilingChunk;
}

/* raises an error with the right line number */
static void errorAt(Token* token, const char* message)
{
	if (parser.panicMode) return; // stop error loops

	// is this the first error
	parser.panicMode = 1;
	
	fprintf(stderr, "[line %d] Error", token->line);
	
	if (token->type == TOKEN_EOF)
	{
		fprintf(stderr, " at end");
	}
	else if (token->type == TOKEN_ERROR)
	{
		// Nothing.
	} else
	{
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);
	parser.hadError = 1;
}

/* A wrapper for the errorAt method: passes previos token */
static void error(const char* message)
{
  errorAt(&parser.previous, message);
}

/* A wrapper for errorAt method: passes current token */
static void errorAtCurrent(const char* message)
{
  errorAt(&parser.current, message);
}

/* Advance the parser to the next token */
static void advance()
{
	parser.previous = parser.current;

	for (;;)
	{
		parser.current = scanToken();
		if (parser.current.type != TOKEN_ERROR) break;

		errorAtCurrent(parser.current.start);
	}
}

/* Consume a token and validate it is of an expected type */
static void consume(TokenType type, const char * message)
{
	if (parser.current.type == type)
	{
		advance();
		return;
	}

	errorAtCurrent(message);
}

/* Append a single byte to be translated to bytecode */
static void emitByte(uint8_t byte)
{
	writeChunk(currentChunk(), byte, parser.current.line);
}

/* emits 16 bits worth of data by calling emit twice */
static void emitBytes(uint8_t byte1, uint8_t byte2)
{
  emitByte(byte1);
  emitByte(byte2);
}

/* writes a return signal to the chunk */
static void emitReturn()
{
  emitByte(OP_RETURN);
}

/* Add an entry into the constant table */
static uint8_t makeConstant(Value value)
{
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX)
  {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

/* Another wrapper for emit Bytes */
static void emitConstant(Value value)
{
  emitBytes(OP_CONSTANT, makeConstant(value));
}

/* Ends compilation with a return signal */
static void endCompiler()
{
	emitReturn();
#ifdef MT_DEBUG_PRINT_CODE
	if (!parser.hadError)
	{
		disassembleChunk(currentChunk(), "code");
	}
#endif
}

/* Prototype functions */
static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

/* Parser a binary expression */
static void binary()
{
	// Remember the operator.
	TokenType operatorType = parser.previous.type;

	// Compile the right operand.
	ParseRule* rule = getRule(operatorType);
	parsePrecedence((Precedence)(rule->precedence + 1));

	// Emit the operator instruction.
	switch (operatorType)
	{
	case TOKEN_PLUS:          emitByte(OP_ADD); break;
	case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
	case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
	case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
	case TOKEN_CARAT:         emitByte(OP_POW); break;
	default:
		return; // Unreachable.
	}
}

/* Check the grouping of parenthesis */
static void grouping()
{
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/* Parse a number */
static void number()
{
  double value = strtod(parser.previous.start, NULL);
  emitConstant(value);
}

static void unary()
{
  TokenType operatorType = parser.previous.type;

  // Compile the operand.
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction.
  switch (operatorType)
  {
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default:
      return; // Unreachable.
  }
}

/* Stores infomation on how to parse tokens */
ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = { grouping, NULL,   PREC_NONE },
  [TOKEN_RIGHT_PAREN]   = { NULL,     NULL,   PREC_NONE },
  [TOKEN_LEFT_BRACE]    = { NULL,     NULL,   PREC_NONE }, 
  [TOKEN_RIGHT_BRACE]   = { NULL,     NULL,   PREC_NONE },
  [TOKEN_COMMA]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_DOT]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_MINUS]         = { unary,    binary, PREC_TERM },
  [TOKEN_PLUS]          = { NULL,     binary, PREC_TERM },
  [TOKEN_SEMICOLON]     = { NULL,     NULL,   PREC_NONE },
  [TOKEN_SLASH]         = { NULL,     binary, PREC_FACTOR },
  [TOKEN_STAR]          = { NULL,     binary, PREC_FACTOR },
  [TOKEN_BANG]          = { NULL,     NULL,   PREC_NONE },
  [TOKEN_BANG_EQUAL]    = { NULL,     NULL,   PREC_NONE },
  [TOKEN_EQUAL]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_EQUAL_EQUAL]   = { NULL,     NULL,   PREC_NONE },
  [TOKEN_GREATER]       = { NULL,     NULL,   PREC_NONE },
  [TOKEN_GREATER_EQUAL] = { NULL,     NULL,   PREC_NONE },
  [TOKEN_LESS]          = { NULL,     NULL,   PREC_NONE },
  [TOKEN_LESS_EQUAL]    = { NULL,     NULL,   PREC_NONE },
  [TOKEN_IDENTIFIER]    = { NULL,     NULL,   PREC_NONE },
  [TOKEN_STRING]        = { NULL,     NULL,   PREC_NONE },
  [TOKEN_NUMBER]        = { number,   NULL,   PREC_NONE },
  [TOKEN_AND]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_CLASS]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_ELSE]          = { NULL,     NULL,   PREC_NONE },
  [TOKEN_FALSE]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_FOR]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_FUN]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_IF]            = { NULL,     NULL,   PREC_NONE },
  [TOKEN_NIL]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_OR]            = { NULL,     NULL,   PREC_NONE },
  [TOKEN_PRINT]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_RETURN]        = { NULL,     NULL,   PREC_NONE },
  [TOKEN_SUPER]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_THIS]          = { NULL,     NULL,   PREC_NONE },
  [TOKEN_TRUE]          = { NULL,     NULL,   PREC_NONE },
  [TOKEN_VAR]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_WHILE]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_ERROR]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_EOF]           = { NULL,     NULL,   PREC_NONE },
};

/* Stops expression() from consuming too much */
static void parsePrecedence(Precedence precedence)
{
	/* Prefix rule */
	advance();
	ParseFn prefixRule = getRule(parser.previous.type)->prefix;
	if (prefixRule == NULL)
	{
		error("Expected expression.");
		return;
	}

	prefixRule();

	/* Infix rule */
	while (precedence <= getRule(parser.current.type)->precedence)
	{
		advance();
		ParseFn infixRule = getRule(parser.previous.type)->infix;
		infixRule();
	}
}

/* get method for the parse table */
static ParseRule* getRule(TokenType type)
{
  return &rules[type];
}

static void expression()
{
	parsePrecedence(PREC_ASSIGNMENT);
}

int compile(const char *src, Chunk *chunk)
{
	initScanner(src);
	compilingChunk = chunk;

	parser.hadError = 0;
	parser.panicMode = 0;
	
	advance();
	expression();
	consume(TOKEN_EOF, "Expected expression.");
	endCompiler();
	return !parser.hadError;
}

