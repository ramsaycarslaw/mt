#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "common.h"
#include "scanner.h"
#include "object.h"

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

static bool check(TokenType type)
{
	return parser.current.type == type;
}

static bool match(TokenType type)
{
	if (!check(type)) return false;
	advance();
	return true;
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
static void statement();
static void declaration();
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
	case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
	case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
	case TOKEN_GREATER:       emitByte(OP_GREATER); break;
	case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
	case TOKEN_LESS:          emitByte(OP_LESS); break;
	case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
	case TOKEN_PLUS:          emitByte(OP_ADD); break;
	case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
	case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
	case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
	case TOKEN_CARAT:         emitByte(OP_POW); break;
	default:
		return; // Unreachable.
	}
}

static void literal()
{
	switch (parser.previous.type)
	{
	case TOKEN_FALSE: emitByte(OP_FALSE); break;
	case TOKEN_NIL: emitByte(OP_NIL); break;
	case TOKEN_TRUE: emitByte(OP_TRUE); break;
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
  emitConstant(NUMBER_VAL(value));
}

static void string()
{
	emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void unary()
{
  TokenType operatorType = parser.previous.type;

  // Compile the operand.
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction.
  switch (operatorType)
  {
  case TOKEN_BANG: emitByte(OP_NOT); break;
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
  [TOKEN_CARAT]         = { NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = { NULL,     binary, PREC_FACTOR },
  [TOKEN_BANG]          = { unary,    NULL,   PREC_NONE },
  [TOKEN_BANG_EQUAL]    = { NULL,     binary, PREC_EQUALITY },
  [TOKEN_EQUAL]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_EQUAL_EQUAL]   = { NULL,     binary, PREC_EQUALITY },
  [TOKEN_GREATER]       = { NULL,     binary, PREC_COMPARISON },
  [TOKEN_GREATER_EQUAL] = { NULL,     binary, PREC_COMPARISON },
  [TOKEN_LESS]          = { NULL,     binary, PREC_COMPARISON },
  [TOKEN_LESS_EQUAL]    = { NULL,     binary, PREC_COMPARISON },
  [TOKEN_IDENTIFIER]    = { NULL,     NULL,   PREC_NONE },
  [TOKEN_STRING]        = { string,   NULL,   PREC_NONE },
  [TOKEN_NUMBER]        = { number,   NULL,   PREC_NONE },
  [TOKEN_AND]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_CLASS]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_ELSE]          = { NULL,     NULL,   PREC_NONE },
  [TOKEN_FALSE]         = { literal,  NULL,   PREC_NONE },
  [TOKEN_FOR]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_FUN]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_IF]            = { NULL,     NULL,   PREC_NONE },
  [TOKEN_NIL]           = { literal,  NULL,   PREC_NONE },
  [TOKEN_OR]            = { NULL,     NULL,   PREC_NONE },
  [TOKEN_PRINT]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_RETURN]        = { NULL,     NULL,   PREC_NONE },
  [TOKEN_SUPER]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_THIS]          = { NULL,     NULL,   PREC_NONE },
  [TOKEN_TRUE]          = { literal,  NULL,   PREC_NONE },
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

/* Parse identifier token */
static uint8_t identifierConstant(Token* name)
{
	return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

/* Parser variable declare to get name */
static uint8_t parseVariable(const char * errorMessage)
{
	consume(TOKEN_IDENTIFIER, errorMessage);
	return identifierConstant(&parser.previous);
}

static void defineVariable(uint8_t global)
{
	emitBytes(OP_DEFINE_GLOBAL, global);
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

/* Compile a var declaration */
static void varDeclaration()
{
	uint8_t global = parseVariable("Expected variable name.");

	if (match(TOKEN_EQUAL))
	{
		expression();
	}
	else
	{
		emitByte(OP_NIL); // variables are nil by default
	}
	consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");

	defineVariable(global);
}

/* Compiles an expression statement */
static void expressionStatement()
{
	expression();
	consume(TOKEN_SEMICOLON, "Expected ';' after expression");
	emitByte(OP_POP);
}

/* Compiles a print statement */
static void printStatement()
{
	expression();
	consume(TOKEN_SEMICOLON, "Expected ';' after value.");
	emitByte(OP_PRINT);
}

/* Basic error recovery */
static void synchronize()
{
	parser.panicMode = 0;

	while (parser.current.type != TOKEN_EOF)
	{
		if (parser.previous.type == TOKEN_SEMICOLON) return;

		switch (parser.current.type)
		{
		case TOKEN_CLASS:
		case TOKEN_FUN:
		case TOKEN_VAR:
		case TOKEN_FOR:
		case TOKEN_IF:
		case TOKEN_WHILE:
		case TOKEN_PRINT:
		case TOKEN_RETURN:
			return;
		 default:
			 // do nothing
			 ;
		}
		advance();
	}
}

static void declaration()
{
	if (match(TOKEN_VAR))
	{
		varDeclaration();
	}
	else
	{
		statement();
	}

	if (parser.panicMode) synchronize();
}

static void statement()
{
	if (match(TOKEN_PRINT))
	{
		printStatement();
	}
	else
	{
		expressionStatement();
	}
}

int compile(const char *src, Chunk *chunk)
{
	initScanner(src);
	compilingChunk = chunk;

	parser.hadError = 0;
	parser.panicMode = 0;
	
	advance();

	while (!match(TOKEN_EOF))
	{
		declaration();
	}
	
	endCompiler();
	return !parser.hadError;
}

