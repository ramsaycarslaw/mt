#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/common.h"
#include "../include/compiler.h"
#include "../include/object.h"
#include "../include/scanner.h"
#include "../include/memory.h"

#ifdef MT_DEBUG_PRINT_CODE
#include "../include/debug.h"
#endif

/* main struct to store the parser */
typedef struct {
  Token current;
  Token previous;
  int hadError;
  int panicMode;
} Parser;

/* Token priority to the parser */
typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_RANGE,
  PREC_CONDITIONAL,
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_SUBSCRIPT,  // [0]()
  PREC_PRIMARY
} Precedence;

/* Variables used for break and continue statements */
int loopStart = -1;
int loopDepth = 0;

/* Template fuction for a parse rule */
typedef void (*ParseFn)(bool canAssign);

/* A ParseRule stores what function is needed to compile each token */
typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  Token name;
  int depth;
  bool isCaptured;
} Local;

/* Stores function closure upvalues */
typedef struct {
  uint8_t index;
  bool isLocal;
} Upvalue;

/* implicit main fn or actual fn */
typedef enum {
  TYPE_LAMBDA,
  TYPE_FUNCTION,
  TYPE_INITIALIZER,
  TYPE_SCRIPT,
  TYPE_METHOD,
} FunctionType;

/* Stores state for the compiler */
typedef struct Compiler {
  struct Compiler *enclosing;

  ObjFunction *function;
  FunctionType type;

  Local locals[UINT8_COUNT];
  int localCount;
  Upvalue upvalues[UINT8_COUNT];
  int scopeDepth;
} Compiler;

typedef struct ClassCompiler {
  struct ClassCompiler *enclosing;
  Token name;
  bool hasSuperClass;
} ClassCompiler;

/* Used for break/continue statements */
typedef struct BreakJump 
{
	int scopeDepth;
	int offset;
	struct BreakJump* next;
} BreakJump;

/* Global representation of break state */
BreakJump *breakJumps = NULL;

/* Global parser struct */
Parser parser;
static uint8_t identifierConstant(Token *name);

static void rangeExpr(bool canAssign);
static void lambdaExpression(bool canAssign);

/* This line may be redundant */
Compiler *current = NULL;

/* Avoid abuse of this keyword */
ClassCompiler *currentClass = NULL;

/* A get method for compling chunk */
static Chunk *currentChunk() {
  // refactored version
  return &current->function->chunk;
}

/* raises an error with the right line number */
static void errorAt(Token *token, const char *message) {
  if (parser.panicMode)
    return; // stop error loops

  // is this the first error
  parser.panicMode = 1;

  printf("\033[31m");
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = 1;

  printf("\033[0m");
}

/* A wrapper for the errorAt method: passes previos token */
static void error(const char *message) { errorAt(&parser.previous, message); }

/* A wrapper for errorAt method: passes current token */
static void errorAtCurrent(const char *message) {
  errorAt(&parser.current, message);
}

/* Advance the parser to the next token */
static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR)
      break;

    errorAtCurrent(parser.current.start);
  }
}

/* Consume a token and validate it is of an expected type */
static void consume(TokenType type, const char *message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}

/* Create an empty token */
Token tokenEmpty()
{
    return (Token) { .type = TOKEN_NONE, .start = NULL, .length = 0, .line = 0 };
}

static bool check(TokenType type) { return parser.current.type == type; }

static bool match(TokenType type) {
  if (!check(type))
    return false;
  advance();
  return true;
}

/* Append a single byte to be translated to bytecode */
static void emitByte(uint8_t byte) {
#ifdef MT_OUT_STREAM
  // TODO Replace this with actual symbol
  printf("%u\n", byte);
#else
  writeChunk(currentChunk(), byte, parser.current.line);
#endif
}

/* emits 16 bits worth of data by calling emit twice */
static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

/* Similar to emit jump but jumps backwards for loops */
static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);

  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX)
    error("Loop body too large.");

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

/* Creates a placeholder jump for else, backtrack to get correct jump */
static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->count - 2;
}

/* writes a return signal to the chunk */
static void emitReturn() {
  if (current->type == TYPE_INITIALIZER) {
    emitBytes(OP_GET_LOCAL, 0);
  } else {
    emitByte(OP_NIL);
  }
  emitByte(OP_RETURN);
}

/* Add an entry into the constant table */
static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk, the limit is 255.");
    return 0;
  }

  return (uint8_t)constant;
}

/* Another wrapper for emit Bytes */
static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

/* patch jump backtracks the placeholders in emit jump */
static void patchJump(int offset) {
  // -2 to account for jump instruction itself
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Cannot jump over that much code at if, the limit is 16,535");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

/* Initialise compiler and set to current */
static void initCompiler(Compiler *compiler, FunctionType type) {
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  compiler->function = newFunction();
  current = compiler;

  if (type != TYPE_SCRIPT) {
    current->function->name =
      copyString(parser.previous.start, parser.previous.length);
  }

  Local *local = &current->locals[current->localCount++];
  local->depth = 0;
  local->isCaptured = false;
  if (type != TYPE_FUNCTION) {
    local->name.start = "this";
    local->name.length = 4;
  } else {
    local->name.start = "";
    local->name.length = 0;
  }
}

/* Ends compilation with a return signal */
static ObjFunction *endCompiler() {
  emitReturn();
  ObjFunction *function = current->function;

#ifdef MT_DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(currentChunk(), function->name != NULL
        ? function->name->chars
        : "<script>");
  }
#endif

  current = current->enclosing;
  return function;
}

/* Enter the scope depth */
static void beginScope() { current->scopeDepth++; }

/* Leave the scope */
static void endScope() {
  current->scopeDepth--;

  while (current->localCount > 0 &&
      current->locals[current->localCount - 1].depth > current->scopeDepth) {
    if (current->locals[current->localCount - 1].isCaptured) {
      emitByte(OP_CLOSE_UPVALUE);
    } else {
      emitByte(OP_POP);
    }
    current->localCount--;
  }
}

/* Patch a break statement */
static void patchBreakJumps() 
{
	while (breakJumps != NULL) 
	{	
		if (breakJumps->scopeDepth >= loopDepth) 
		{
			patchJump(breakJumps->offset);

			// free node in linked list
			BreakJump* temp = breakJumps;
			breakJumps = breakJumps->next;
			FREE(BreakJump, temp);	
		} 
		else 
		{
			break;
		}
	}
}

/* Prototype functions */
static uint8_t argumentList();
static void expression();
static void statement();
static void declaration();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

/* Parse a ternary expression */
static void ternary(bool canAssign) 
{
  /* 
   
   expression ? statement : statement;

   */ 

  int jump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  expression();

  consume(TOKEN_COLON, "Expected ':' in ternary expression.");

  int elseJump = emitJump(OP_JUMP);

  patchJump(jump);
  emitByte(OP_POP);

  expression();
  patchJump(elseJump);
}

/* Parser a binary expression */
static void binary(bool canAssign) {
  /*
   * Remember the operator.
   */
  TokenType operatorType = parser.previous.type;

  /* Compile the right operand. */
  ParseRule *rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  /* Emit the operator instruction. */
  switch (operatorType) {
    case TOKEN_BANG_EQUAL:
      emitBytes(OP_EQUAL, OP_NOT);
      break;
    case TOKEN_EQUAL_EQUAL:
      emitByte(OP_EQUAL);
      break;
    case TOKEN_GREATER:
      emitByte(OP_GREATER);
      break;
    case TOKEN_GREATER_EQUAL:
      emitBytes(OP_LESS, OP_NOT);
      break;
    case TOKEN_LESS:
      emitByte(OP_LESS);
      break;
    case TOKEN_LESS_EQUAL:
      emitBytes(OP_GREATER, OP_NOT);
      break;
    case TOKEN_PLUS:
      emitByte(OP_ADD);
      break;
    case TOKEN_MINUS:
      emitByte(OP_SUBTRACT);
      break;
    case TOKEN_STAR:
      emitByte(OP_MULTIPLY);
      break;
    case TOKEN_SLASH:
      emitByte(OP_DIVIDE);
      break;
    case TOKEN_CARAT:
      emitByte(OP_POW);
      break;
    case TOKEN_PERCENT:
      emitByte(OP_MOD);
      break;
    default:
      return; /* Unreachable. */
  }
}

/* Parse a function call */
static void call(bool canAssign) {
  uint8_t argCount = argumentList();
  emitBytes(OP_CALL, argCount);
}

/* Parse a get or set expression of an instance of a class */
static void dot(bool canAssign) {
  consume(TOKEN_IDENTIFIER, "Expected property name after '.', make sure you "
      "are using it on a class.");
  uint8_t name = identifierConstant(&parser.previous);

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(OP_SET_PROPERTY, name);
  } else if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    emitBytes(OP_INVOKE, name);
    emitByte(argCount);
  } else {
    emitBytes(OP_GET_PROPERTY, name);
  }
}

/* Parse a function literal */
static void literal(bool canAssign) {
  switch (parser.previous.type) {
    case TOKEN_FALSE:
      emitByte(OP_FALSE);
      break;
    case TOKEN_NIL:
      emitByte(OP_NIL);
      break;
    case TOKEN_TRUE:
      emitByte(OP_TRUE);
      break;
    default:
      return; /* Unreachable. */
  }
}

/* Check the grouping of parenthesis */
static void grouping(bool canAssign) {
  expression();

  if (match(TOKEN_COMMA)) 
  {
    int itemCount=1; 
    do 
    {
      if (check(TOKEN_RIGHT_PAREN)) 
      {
        // trailing comma
        break;
      }

      parsePrecedence(PREC_OR);

      if (itemCount == UINT8_COUNT) {
        error("Cannot have more than 256 items in a tuple literal.");
      }
      itemCount++;
    } while (match(TOKEN_COMMA));

    consume(TOKEN_RIGHT_PAREN, "Expected ')' after tuple declaration");
    // TODO make op build tuple
    emitByte(OP_BUILD_TUPLE);
    emitByte(itemCount);
    return;
  }

  consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

/* Parse a number */
static void number(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

/* Logical operators using jumping */
static void and_(bool canAssign);
static void or_(bool canAssign) {
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
}

/* Parse a string */
static void string(bool canAssign) {
  emitConstant(OBJ_VAL(
        copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

/* Parse a list */
static void list(bool canAssign) {
  int itemCount = 0;

  // check for empty list
  if (!check(TOKEN_RIGHT_BRACKET)) {
    do {
      if (check(TOKEN_RIGHT_BRACKET)) {
        // Trailing comma case
        break;
      }

      parsePrecedence(PREC_OR);

      if (itemCount == UINT8_COUNT) {
        error("Cannot have more than 256 items in a list literal.");
      }
      itemCount++;
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_BRACKET,
      "Expected ']' after list literal, you should close the brackets.");

  emitByte(OP_BUILD_LIST);
  emitByte(itemCount);
  return;
}

/* Parse a subscript */
static void subscript(bool canAssign) {
  parsePrecedence(PREC_OR);
  consume(TOKEN_RIGHT_BRACKET,
      "Expected ']' after index, add ']' after the number to index to.");

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitByte(OP_STORE_SUBSCR);
  } else {
    emitByte(OP_INDEX_SUBSCR);
  }
  return;
}

static uint8_t identifierConstant(Token *name);
static int resolveLocal(Compiler *compiler, Token *token);

/* Upvalues need to be added to the hash table */
static int addUpvalue(Compiler *compiler, uint8_t index, bool isLocal) {
  int upvalueCount = compiler->function->upvalueCount;

  for (int i = 0; i < upvalueCount; i++) {
    Upvalue *upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal) {
      return i;
    }
  }

  if (upvalueCount == UINT8_COUNT) {
    error("Too many closure variables in function, the limit is 256. Closures "
        "are used for nested functions etc, try to split up your code.");
    return 0;
  }

  compiler->upvalues[upvalueCount].isLocal = isLocal;
  compiler->upvalues[upvalueCount].index = index;
  return compiler->function->upvalueCount++;
}

/* Upvalues for function closures */
static int resolveUpvalue(Compiler *compiler, Token *name) {
  if (compiler->enclosing == NULL)
    return -1;

  int local = resolveLocal(compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals[local].isCaptured = true;
    return addUpvalue(compiler, (uint8_t)local, true);
  }

  int upvalue = resolveUpvalue(compiler->enclosing, name);
  if (upvalue != -1) {
    return addUpvalue(compiler, (uint8_t)upvalue, false);
  }

  return -1;
}

/* Get the named variable from the compiler */
static void namedVariable(Token name, bool canAssign) {
  
  #define SHORT_HAND(op) \
  do { \
    emitBytes(getOp, (uint8_t)arg); \
    expression(); \
    emitByte(op); \
    emitBytes(setOp, (uint8_t)arg); \
  } while (false)

  uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name);

  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(setOp, (uint8_t)arg);
  } else if (canAssign && match(TOKEN_PLUS_EQUALS)) {
    // we use the above macro
    SHORT_HAND(OP_ADD);
  } else if (canAssign && match(TOKEN_MINUS_EQUALS)) {
    SHORT_HAND(OP_SUBTRACT);
  }  else if (canAssign && match(TOKEN_STAR_EQUALS)) {
    SHORT_HAND(OP_MULTIPLY);
  }  else if (canAssign && match(TOKEN_SLASH_EQUALS)) {
    SHORT_HAND(OP_DIVIDE);
  }  else if (canAssign && match(TOKEN_CARAT_EQUALS)) {
    SHORT_HAND(OP_POW);
  }  else if (canAssign && match(TOKEN_PERCENT_EQUALS)) {
    SHORT_HAND(OP_MOD);
  } else {
    emitBytes(getOp, (uint8_t)arg);
  }
}

/* Get the variable from the compiler, given it is able to assign. */
static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}

/* create a 'fake' keyword */
static Token syntheticToken(const char* text) {
  Token token;
  token.start = text;
  token.length = (int)strlen(text);
  return token;
}

/* Parse super keyword */
static void super_(bool canAssign) {
  if (currentClass == NULL) {
    error("Can't use 'super' outside of a class.");
  } else if (!currentClass->hasSuperClass) {
    error("Can't use 'super' in a class with no superclass.");
  }

  consume(TOKEN_DOT, "Expected '.' after 'super'.");
  consume(TOKEN_IDENTIFIER, "Expected superclass method name.");
  uint8_t name = identifierConstant(&parser.previous);

  namedVariable(syntheticToken("this"), false);
  if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    namedVariable(syntheticToken("super"), false);
    emitBytes(OP_SUPER_INVOKE, name);
    emitByte(argCount);
  } else {
    namedVariable(syntheticToken("super"), false);
    emitBytes(OP_GET_SUPER, name);
  }
}

/* Parse a Unary operator ie. -a or !b */
static void unary(bool canAssign) {
  TokenType operatorType = parser.previous.type;

  /* Compile the operand. */
  parsePrecedence(PREC_UNARY);

  /* Emit the operator instruction. */
  switch (operatorType) {
    case TOKEN_BANG:
      emitByte(OP_NOT);
      break;
    case TOKEN_MINUS:
      emitByte(OP_NEGATE);
      break;
    case TOKEN_PLUS_PLUS:
      emitByte(OP_INCR);
      break;
    default:
      return; // Unreachable.
  }
}

static void this_(bool canAssign) {
  if (currentClass == NULL) {
    error("'this' is a resevered keyword and as such cannot be used outside of "
        "a class.");
    return;
  }
  variable(false);
}

/* Used to increase the value of a variable by 1 */
static void increment(bool canAssign)
{
  return;
}

/* Stores infomation on how to parse tokens */
ParseRule rules[] = {
	[TOKEN_BREAK] = {NULL, NULL, PREC_NONE},
	[TOKEN_CONTINUE] = {NULL, NULL, PREC_NONE},
  [TOKEN_AND] = {NULL, and_, PREC_AND},
  [TOKEN_BACKSLASH] = {lambdaExpression, NULL, PREC_NONE}, // lambda expression
  [TOKEN_BANG] = {unary, NULL, PREC_NONE},
  [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
  [TOKEN_CARAT] = {NULL, binary, PREC_FACTOR},
  [TOKEN_CASE] = {NULL, NULL, PREC_NONE},
  [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
  [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
  [TOKEN_DEFAULT] = {NULL, NULL, PREC_NONE},
  [TOKEN_DEFER] = {NULL, NULL, PREC_NONE},
  [TOKEN_DOT] = {NULL, dot, PREC_CALL},
  [TOKEN_DOT_DOT] = {NULL, rangeExpr, PREC_RANGE},
  [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
  [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
  [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
  [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
  [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
  [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
  [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
  [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
  [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
  [TOKEN_IF] = {NULL, NULL, PREC_NONE},
  [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
  [TOKEN_LEFT_BRACKET] = {list, subscript, PREC_SUBSCRIPT},
  [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
  [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
  [TOKEN_MINUS] = {unary, binary, PREC_TERM},
  [TOKEN_NIL] = {literal, NULL, PREC_NONE},
  [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
  [TOKEN_OR] = {NULL, or_, PREC_OR},
  [TOKEN_PERCENT] = {NULL, binary, PREC_FACTOR},
  [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
  [TOKEN_PLUS_EQUALS] = {NULL, binary, PREC_NONE},
  [TOKEN_PLUS_PLUS] = { unary, increment, PREC_NONE,},
  [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
  [TOKEN_QUESTION] = {NULL, ternary, PREC_OR},
  [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
  [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
  [TOKEN_RIGHT_BRACKET] = {NULL, NULL, PREC_NONE},
  [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
  [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
  [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
  [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
  [TOKEN_STRING] = {string, NULL, PREC_NONE},
  [TOKEN_SUPER]  = {super_, NULL, PREC_NONE},
  [TOKEN_SWITCH] = {NULL, NULL, PREC_NONE}, // handle switch statements
  [TOKEN_THIS] = {this_, NULL, PREC_NONE},
  [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
  [TOKEN_USE] = {NULL, NULL, PREC_NONE},
  [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
  [TOKEN_LET] = {NULL, NULL, PREC_NONE},
  [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
};

/* Stops expression() from consuming too much */
static void parsePrecedence(Precedence precedence) {
  /* Prefix rule */
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expected expression.");
    return;
  }

  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);

  /* Infix rule */
  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

/* Parse identifier token */
static uint8_t identifierConstant(Token *name) {
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

/* fn to check for variable redeclaration */
static bool identifiersEqual(Token *a, Token *b) {
  if (a->length != b->length)
    return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

/* resolve local gets the value of a local variable */
static int resolveLocal(Compiler *compiler, Token *name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local *local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {
      if (local->depth == -1) {
        error("Cannot read variable in it's own initialiser.");
      }
      return i;
    }
  }

  return -1;
}

/* add a local variable to the current scope */
static void addLocal(Token name) {
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in current scope");
    return;
  }
  Local *local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
  local->isCaptured = false;
}

/* add a local variable */
static void declareVariable() {
  /* globals are implicit */
  if (current->scopeDepth == 0)
    return;

  Token *name = &parser.previous;
  for (int i = current->localCount - 1; i >= 0; i--) {
    Local *local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break;
    }

    if (identifiersEqual(name, &local->name)) {
      error("Variable redeclaration within scope.");
    }
  }
  addLocal(*name);
}

/* Parser variable declare to get name */
static uint8_t parseVariable(const char *errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);

  declareVariable();
  if (current->scopeDepth > 0)
    return 0;

  return identifierConstant(&parser.previous);
}

/* marks a variable as intalised to prevent reininstalstion */
static void markInitialised() {
  if (current->scopeDepth == 0)
    return;
  current->locals[current->localCount - 1].depth = current->scopeDepth;
}

/* defines a new variable */
static void defineVariable(uint8_t global) {
  if (current->scopeDepth > 0) {
    markInitialised();
    return;
  }

  emitBytes(OP_DEFINE_GLOBAL, global);
}

/* TODO finish this */
static void defineTypedVariable(uint8_t global, Type type) {
  if (current->scopeDepth > 0) {
    markInitialised();
    return;
  }

  emitByte(global);
  emitBytes(OP_TYPE_SET, type);
}

/* gets the list of arguments from function call */
static uint8_t argumentList() {
  uint8_t argCount = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();

      if (argCount == 255) {
        error("Cannot have more than 255 arguments.");
      }

      argCount++;
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_PAREN, "Expected ')' after arguments.");
  return argCount;
}

/* Logical and operator */
static void and_(bool canAssign) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  parsePrecedence(PREC_AND);

  patchJump(endJump);
}

/* get method for the parse table */
static ParseRule *getRule(TokenType type) { return &rules[type]; }

static void expression() { parsePrecedence(PREC_ASSIGNMENT); }

/* Parse a block statemnt */
static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expected '}' after block statemnent.");
}

/* Compile actual function */
static void function(FunctionType type) {
  Compiler compiler;
  initCompiler(&compiler, type);
  beginScope();

  /* compile parameter list */
  consume(TOKEN_LEFT_PAREN, "Expected '(' after function name.");
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        errorAtCurrent("Cannot have more than 255 parameters.");
      }

      uint8_t paramConstant = parseVariable("Expected variable name");
      defineVariable(paramConstant);
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after parameteres");

  /* function body compiler */
  consume(TOKEN_LEFT_BRACE, "Expected '{' before function body");
  block();

  /* creare function object representation */
  ObjFunction *function = endCompiler();
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

  /* Handle closures and upvalues */
  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
}

/* Compile a range expression 0..n */
static void rangeExpr(bool canAssign) {
  expression();
  emitByte(OP_POP);

  for (int i = 0; i < 10; i++) {
    emitConstant(NUMBER_VAL(i));
  }

  emitByte(OP_BUILD_LIST);
  emitConstant(NUMBER_VAL(10));
  return;
}

/* Compile a lamda expression */
static void lambdaExpression(bool canAssign) {
  Compiler compiler;
  initCompiler(&compiler, TYPE_LAMBDA);
  beginScope();

  /* If we don't find an arrow they must want arguments */
  if (!check(TOKEN_RIGHT_ARROW)) {  
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        errorAtCurrent("Cannot have more than 255 parameters.");
      }

      uint8_t paramConstant = parseVariable("Expected variable name");
      defineVariable(paramConstant);
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_ARROW, "Expected '->' after lambda expression.");

  consume(TOKEN_LEFT_BRACE, "Expected '{' after lambda's '->'.");
    block();
  /* creare function object representation */
  ObjFunction *function = endCompiler();
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

  /* Handle closures and upvalues */
  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
}


static void method() {
  consume(TOKEN_IDENTIFIER, "Expected method name.");
  uint8_t constant = identifierConstant(&parser.previous);

  FunctionType type = TYPE_METHOD;

  if (parser.previous.length == 4 && memcmp(parser.previous.start, "init", 4) == 0) {
    type = TYPE_INITIALIZER;
  }

  function(type);
  emitBytes(OP_METHOD, constant);
}

/* Parse & Compile a class declaration */
static void classDeclaration() {
  /* We expect a class name */
  consume(TOKEN_IDENTIFIER, "Expect class name");
  Token className = parser.previous;
  uint8_t nameConstant = identifierConstant(&parser.previous);
  /* We make a variable out of the class name */
  declareVariable();

  /* Compile it as a class constant */
  emitBytes(OP_CLASS, nameConstant);
  defineVariable(nameConstant);

  /* We use a struct to store state for a class */
  ClassCompiler classCompiler;
  classCompiler.name = parser.previous;
  classCompiler.hasSuperClass = false;
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;

  /* Add the ability for a superclass */
  if (match(TOKEN_LESS)) {
    consume(TOKEN_IDENTIFIER, "Expect superclass name.");
    /* The superclass is already a variable */
    variable(false);

    /* Preent a class from inheriting from itself */
    if (identifiersEqual(&className, &parser.previous)) {
      error("A class can't inherit from itself.");
    }

    beginScope();
    addLocal(syntheticToken("super"));
    defineVariable(0);

    namedVariable(className, false);
    emitByte(OP_INHERIT);
    classCompiler.hasSuperClass = true;
  }

  namedVariable(className, false);

  consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
  /* Now we parse the class body */
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    method();
  }
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
  emitByte(OP_POP);

  if (classCompiler.hasSuperClass) {
    endScope();
  }

  currentClass = currentClass->enclosing;
}

/* compile function declaration */
static void funDeclaration() {
  uint8_t global = parseVariable("Expected function name.");
  markInitialised();
  function(TYPE_FUNCTION);
  defineVariable(global);
}

/* Compile a var declaration */
static void varDeclaration() {
  uint8_t global = parseVariable("Expected variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL); /* variables are nil by default */
  }
  consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");

  defineVariable(global);
}

/* Compile a statically typed let declaration */
static void letDeclaration() {
  uint8_t global = parseVariable("Expected variable name.");

  if (!match(TOKEN_COLON)) {
    if (match(TOKEN_EQUAL)) {
      expression();
    } else {
      emitByte(OP_NIL); /* variables are nil by default */
    }
    consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");

    defineVariable(global);
  } 
  else 
  {
    Type type = NO_TYPE;

    if (match(TOKEN_N64)) {
      type = NUMBER_TYPE;
    } else if (match(TOKEN_STR)) {
      type = STRING_TYPE;
    } else {
      error("Could not resolve type of 'let'.");
    }
    advance();

    if (!match(TOKEN_EQUAL)) {
      expression();
    } else {
      emitByte(OP_NIL);
    }

    consume(TOKEN_SEMICOLON, "Expected ';' after let declartation.");

    defineTypedVariable(global, type);
  }
}

/* Compile a use statement */
static void useDeclaration() 
{
  // TODO add as 
  expression();
  emitByte(OP_USE);
  consume(TOKEN_SEMICOLON, "Expected ';' after 'use' path");
}

/* Compiles a break statement */
static void breakStatement() 
{
  if (loopStart == -1) 
  {
    error("Unexpected 'break' outside of loop body");
  }

  // we expect a semicolon
  consume(TOKEN_SEMICOLON, "Expected ';' after break statement");

  // clear all local variables from memory
  for (int i = current->localCount-1; 
      i >= 0 && current->locals[i].depth > loopDepth; i--)
  {
    // probably a better way of doing this
    emitByte(OP_POP);
  }

  // jump out of the loop
  int jump = emitJump(OP_JUMP);

	// Add breakJump to start of linked list
	BreakJump* breakJump = ALLOCATE(BreakJump, 1);
	breakJump->scopeDepth = loopDepth;
	breakJump->offset = jump;
	breakJump->next = breakJumps;
	breakJumps = breakJump;
}

/* Compiles a continue statement */
static void continueStatement() 
{
	if (loopStart == -1) 
	{
		error("Unexpected 'comntinue' outisde of loop body");
	}	

	consume(TOKEN_SEMICOLON, "Expected ';' after 'continue'");

	for (int i = current->localCount - 1;
  	i >= 0 && current->locals[i].depth > loopDepth; i--)
	{
		emitByte(OP_POP);		
	}
	emitJump(loopStart);
}

/* Compiles an expression statement */
static void expressionStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expected ';' after expression");
  emitByte(OP_POP);
}


/* Compiles a for statement */
static void forStatement() {

  // consume(TOKEN_LEFT_PAREN, "Expected '(' after 'for'.");  
  /* FOR IN statement */
  if (!match(TOKEN_LEFT_PAREN)) {

    beginScope();
    advance();
    Token target = parser.previous;

    declareVariable();
    markInitialised();

    consume(TOKEN_IN, "Expected 'in' after variable.");

    expression();

    emitByte(OP_ITERATOR);
    addLocal(tokenEmpty());
    markInitialised();
    
    loopStart = currentChunk()->count;
    int exitJump = emitJump(OP_FOR_ITERATOR);

    namedVariable(target, false);
    emitByte(OP_POP);

    statement();

    emitLoop(loopStart);
    patchJump(exitJump);
    endScope();
  } else {
    beginScope();
    /* FOR STATEMENT */
    if (match(TOKEN_SEMICOLON)) {
      /* No initialiser */
    } else if (match(TOKEN_VAR)) {
      varDeclaration();
    } else if (match(TOKEN_LET)) { 
      letDeclaration();    
    } else {
      expressionStatement();
    }

    // int loopStart = currentChunk()->count;
    int surroundingStart = loopStart;
    int surroundingDepth = loopDepth;
    loopStart = currentChunk()->count;
    loopDepth = current->scopeDepth;

    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {
      expression();
      consume(TOKEN_SEMICOLON, "Expected ';' after loop condition.");

      exitJump = emitJump(OP_JUMP_IF_FALSE);
      emitByte(OP_POP);
    }

    if (!match(TOKEN_RIGHT_PAREN)) {
      int bodyJump = emitJump(OP_JUMP);

      int incrementStart = currentChunk()->count;
      expression();
      emitByte(OP_POP);
      consume(TOKEN_RIGHT_PAREN, "Expected ')' after for clauses.");

      emitLoop(loopStart);
      loopStart = incrementStart;
      patchJump(bodyJump);
    }

    statement();

    emitLoop(loopStart);

    if (exitJump != -1) {
      patchJump(exitJump);
      emitByte(OP_POP);
    }

    patchBreakJumps();

    loopStart = surroundingStart;
    loopDepth = surroundingDepth;

    endScope();
  }
}

/* Compiles an if statement */
static void ifStatement() {
  consume(TOKEN_LEFT_PAREN, "Expected '(' after if statement.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition.");

  int thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();

  int elseJump = emitJump(OP_JUMP);

  patchJump(thenJump);
  emitByte(OP_POP);

  if (match(TOKEN_ELSE))
    statement();
  patchJump(elseJump);
}

/* Compile a switch statement */
static void switchStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'switch'");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression");
  consume(TOKEN_LEFT_BRACE, "Expect '{' before switch cases.");

  int state = 0;
  int cases[500];
  int case_count = 0;
  int case_ = -1;

  while (!match(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    if (match(TOKEN_CASE) || match(TOKEN_DEFAULT)) {
      TokenType caseType = parser.previous.type;

      if (state == 2) {
        error("Can't have another case or default after the default case.");
      }

      if (state == 1) {
        cases[case_count++] = emitJump(OP_JUMP);
        patchJump(case_);
        emitByte(OP_POP);
      }

      if (caseType == TOKEN_CASE) {
        state = 1;

        emitByte(OP_COPY);
        expression();

        consume(TOKEN_COLON, "Expected ':' after case value.");
        beginScope();

        emitByte(OP_EQUAL);
        case_ = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
        endScope();
      } else {
        state = 2;
        consume(TOKEN_COLON, "Expected ':' after 'default'.");
        case_ = -1;
      }
    } else {
      if (state == 0) {
        error("Can't have statements before any case.");
      }
      statement();
    }
  }

  if (state == 1) 
  {
    patchJump(case_);
    emitByte(OP_POP);
  }

  for (int i = 0; i < case_count; i++)
    patchJump(cases[i]);

  emitByte(OP_POP);
}

/* Compiles a print statement */
static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expected ';' after value.");
  emitByte(OP_PRINT);
}

/* Compile a function return statement */
static void returnStatement() {
  /* cannot return from main */
  if (current->type == TYPE_SCRIPT) {
    error("Cannot return from top-level code.");
  }

  if (match(TOKEN_SEMICOLON)) {
    emitReturn();
  } else {
    if (current->type == TYPE_INITIALIZER) {
      error("Cannot return from an initialiser function.");
    }
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after return value.");
    emitByte(OP_RETURN);
  }
}

/* Compile a defer statement */
static void deferStatement() {
  if (current->type == TYPE_SCRIPT) {
    error("Cannot call 'defer' from top level code");
  }
  expression();
  consume(TOKEN_SEMICOLON, "Expected ';' after return value.");
  emitByte(OP_DEFER);
}

/* Compile a while statemnt */
static void whileStatement() {
  int surroundingStart = loopStart;
  int surroundingDepth = loopDepth;
  loopStart = currentChunk()->count;
  loopDepth = current->scopeDepth;

  consume(TOKEN_LEFT_PAREN, "Expected '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition.");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  statement();

  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP);

  patchBreakJumps();

  loopStart = surroundingStart;
  loopDepth = surroundingDepth;
}

/* Basic error recovery */
static void synchronize() {
  parser.panicMode = 0;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON)
      return;

    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_LET:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_USE:
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

static void declaration() {
  if (match(TOKEN_USE)) {
    useDeclaration();
  } else if (match(TOKEN_CLASS)) {
    classDeclaration();
  } else if (match(TOKEN_FUN)) {
    funDeclaration();
  } else if (match(TOKEN_VAR)) {
    varDeclaration();
  } else if (match(TOKEN_LET)) {
    letDeclaration();
  } else {
    statement();
  }

  if (parser.panicMode)
    synchronize();
}

/* Parse a generic stateent */
static void statement() {
  if (match(TOKEN_BREAK)) {
    breakStatement();
  } else if (match(TOKEN_CONTINUE)) {
    continueStatement();
  } else if (match(TOKEN_PRINT)) {
    printStatement();
  } else if (match(TOKEN_DEFER)) {
    deferStatement();
  } else if (match(TOKEN_FOR)) {
    forStatement();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_SWITCH)) {
    switchStatement();
  } else if (match(TOKEN_RETURN)) {
    returnStatement();
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  } else {
    expressionStatement();
  }
}

/* Compile is the main function used to create bytecode */
ObjFunction *compile(const char *src, bool andRun) {
  initScanner(src);
  Compiler compiler;
  initCompiler(&compiler, TYPE_SCRIPT);

  parser.hadError = 0;
  parser.panicMode = 0;

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  ObjFunction *function = endCompiler();
  return parser.hadError ? NULL : function;
}
