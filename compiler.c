#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* Template fuction for a parse rule */
typedef void (*ParseFn)(bool canAssign);

/* A ParseRule stores what function is needed to compile each token */
typedef struct
{
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct
{
	Token name;
	int depth;
} Local;

/* Stores function closure upvalues */
typedef struct
{
    uint8_t index;
    bool isLocal;
} Upvalue;

/* implicit main fn or actual fn */
typedef enum 
{
    TYPE_FUNCTION,
    TYPE_SCRIPT,
} FunctionType;

/* Stores state for the compiler */
typedef struct Compiler
{
    struct Compiler* enclosing;

    ObjFunction* function;
    FunctionType type;

    Local locals[UINT8_COUNT];
    int localCount;
    Upvalue upvalues[UINT8_COUNT];
    int scopeDepth;
} Compiler;

/* Global parser struct */
Parser parser;

/* This line may be redundant */
Compiler* current = NULL;

/* A get method for compling chunk */
static Chunk* currentChunk()
{
    // refactored version
    return &current->function->chunk;
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

/* Similar to emit jump but jumps backwards for loops */
static void emitLoop(int loopStart) 
{
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) error("Loop body too large.");

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

/* Creates a placeholder jump for else, backtrack to get correct jump */
static int emitJump(uint8_t instruction)
{
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}

/* writes a return signal to the chunk */
static void emitReturn()
{
    emitByte(OP_NIL);
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

/* patch jump backtracks the placeholders in emit jump */
static void patchJump(int offset) 
{
    // -2 to account for jump instruction itself
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX) 
    {
        error("Cannot jump over that much code at if");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

/* Initialise compiler and set to current */
static void initCompiler(Compiler* compiler, FunctionType type)
{
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
	compiler->localCount = 0;
	compiler->scopeDepth = 0;
    compiler->function = newFunction();
	current = compiler;

    if (type != TYPE_SCRIPT) 
    {
        current->function->name = copyString(parser.previous.start, parser.previous.length);
    }

    Local* local = &current->locals[current->localCount++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
}

/* Ends compilation with a return signal */
static ObjFunction* endCompiler()
{
	emitReturn();
    ObjFunction* function = current->function;

#ifdef MT_DEBUG_PRINT_CODE
	if (!parser.hadError)
	{
        disassembleChunk(currentChunk(), function->name != NULL ? function->name->chars : "<script>");
	}
#endif

    current = current->enclosing;
    return function;
}

/* Enter the scope depth */
static void beginScope()
{
	current->scopeDepth++;
}

/* Leave the scope */
static void endScope()
{
	current->scopeDepth--;

    while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth)
	{
		emitByte(OP_POP);
		current->localCount--;
	}
}

/* Prototype functions */
static uint8_t argumentList();
static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

/* Parser a binary expression */
static void binary(bool canAssign)
{
	/* 
     * Remember the operator. 
     */
	TokenType operatorType = parser.previous.type;

	/* Compile the right operand. */
	ParseRule* rule = getRule(operatorType);
	parsePrecedence((Precedence)(rule->precedence + 1));

	/* Emit the operator instruction. */
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
    case TOKEN_PERCENT:       emitByte(OP_MOD); break;
	default:
		return; /* Unreachable. */
	}
}

/* Parse a function call */
static void call(bool canAssign) 
{
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

/* Parse a function literal */
static void literal(bool canAssign)
{
	switch (parser.previous.type)
	{
	case TOKEN_FALSE: emitByte(OP_FALSE); break;
	case TOKEN_NIL: emitByte(OP_NIL); break;
	case TOKEN_TRUE: emitByte(OP_TRUE); break;
	default:
		return; /* Unreachable. */
	}
}

/* Check the grouping of parenthesis */
static void grouping(bool canAssign)
{
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/* Parse a number */
static void number(bool canAssign)
{
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

/* Logical operators using jumping */
static void and_(bool canAssign);
static void or_(bool canAssign) 
{
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

/* Parse a string */
static void string(bool canAssign)
{
	emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                    parser.previous.length - 2)));
}

static uint8_t identifierConstant(Token* name);
static int resolveLocal(Compiler* compiler, Token* token);

/* Upvalues need to be added to the hash table */
static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal)
{
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++)
    {
	Upvalue* upvalue = &compiler->upvalues[i];
	if (upvalue->index == index && upvalue->isLocal == isLocal)
	{
	    return i;
	}
    }

    if (upvalueCount == UINT8_COUNT)
    {
	error("Too many closure variables in function.");
	return 0;
    }
    
    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

/* Upvalues for function closures */
static int resolveUpvalue(Compiler *compiler, Token* name)
{
    if (compiler->enclosing == NULL) return -1;

    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1)
    {
	addUpvalue(compiler, (uint8_t)local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1)
    {
	return addUpvalue(compiler, (uint8_t)upvalue, false);
    }

    return -1;
}

/* Get the named variable from the compiler */
static void namedVariable(Token name, bool canAssign)
{
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);

    if (arg != -1) 
    {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;   
    }
    else if ((arg = resolveUpvalue(current, &name)) != -1)
    {
	getOp = OP_GET_UPVALUE;
	setOp = OP_SET_UPVALUE;
    }
    else 
    {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

	if (canAssign && match(TOKEN_EQUAL))
	{
		expression();
		emitBytes(setOp, (uint8_t)arg);
	}
	else
	{
		emitBytes(getOp, (uint8_t)arg);
	}
}

/* Get the variable from the compiler, given it is able to assign. */
static void variable(bool canAssign)
{
	namedVariable(parser.previous, canAssign);
}

/* Parse a Unary operator ie. -a or !b */
static void unary(bool canAssign)
{
  TokenType operatorType = parser.previous.type;

  /* Compile the operand. */
  parsePrecedence(PREC_UNARY);

  /* Emit the operator instruction. */
  switch (operatorType)
  {
  case TOKEN_BANG: emitByte(OP_NOT); break;
  case TOKEN_MINUS: emitByte(OP_NEGATE); break;
  case TOKEN_PLUS_PLUS: emitByte(OP_INCR); break;
  default:
	  return; // Unreachable.
  }
}

/* Stores infomation on how to parse tokens */
ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = { grouping, call,   PREC_CALL },
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
  [TOKEN_PERCENT]       = { NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = { NULL,     binary, PREC_FACTOR },
  [TOKEN_BANG]          = { unary,    NULL,   PREC_NONE },
  [TOKEN_PLUS_PLUS]     = { unary,    NULL,   PREC_NONE,},
  [TOKEN_BANG_EQUAL]    = { NULL,     binary, PREC_EQUALITY },
  [TOKEN_EQUAL]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_EQUAL_EQUAL]   = { NULL,     binary, PREC_EQUALITY },
  [TOKEN_GREATER]       = { NULL,     binary, PREC_COMPARISON },
  [TOKEN_GREATER_EQUAL] = { NULL,     binary, PREC_COMPARISON },
  [TOKEN_LESS]          = { NULL,     binary, PREC_COMPARISON },
  [TOKEN_LESS_EQUAL]    = { NULL,     binary, PREC_COMPARISON },
  [TOKEN_IDENTIFIER]    = { variable, NULL,   PREC_NONE },
  [TOKEN_STRING]        = { string,   NULL,   PREC_NONE },
  [TOKEN_NUMBER]        = { number,   NULL,   PREC_NONE },
  [TOKEN_AND]           = { NULL,     and_,   PREC_AND },
  [TOKEN_CLASS]         = { NULL,     NULL,   PREC_NONE },
  [TOKEN_ELSE]          = { NULL,     NULL,   PREC_NONE },
  [TOKEN_FALSE]         = { literal,  NULL,   PREC_NONE },
  [TOKEN_FOR]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_FUN]           = { NULL,     NULL,   PREC_NONE },
  [TOKEN_IF]            = { NULL,     NULL,   PREC_NONE },
  [TOKEN_NIL]           = { literal,  NULL,   PREC_NONE },
  [TOKEN_OR]            = { NULL,     or_,    PREC_OR },
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

	bool canAssign = precedence <= PREC_ASSIGNMENT;
	prefixRule(canAssign);

	/* Infix rule */
	while (precedence <= getRule(parser.current.type)->precedence)
	{
		advance();
		ParseFn infixRule = getRule(parser.previous.type)->infix;
		infixRule(canAssign);
	}

	if (canAssign && match(TOKEN_EQUAL))
	{
		error("Invalid assignment target.");
	}
}

/* Parse identifier token */
static uint8_t identifierConstant(Token* name)
{
	return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

/* fn to check for variable redeclaration */
static bool identifiersEqual(Token* a, Token* b)
{
	if (a->length != b->length) return false;
	return memcmp(a->start, b->start, a->length) == 0;	
}

/* resolve local gets the value of a local variable */
static int resolveLocal(Compiler* compiler, Token* name) 
{
    for (int i = compiler->localCount - 1; i >= 0; i--) 
    {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) 
        {
            if (local->depth == -1) 
            {
                error("Cannot read variable in it's own initialiser.");
            }
            return i; 
        }
    }

    return -1;
}

/* add a local variable to the current scope */
static void addLocal(Token name)
{
	if (current->localCount == UINT8_COUNT)
	{
		error("Too many local variables in current scope");
		return;
	}
	Local* local = &current->locals[current->localCount++];
	local->name = name;
    local->depth = -1;
}

/* add a local variable */
static void declareVariable()
{
	/* globals are implicit */
	if (current->scopeDepth == 0) return;

	Token* name = &parser.previous;
	for (int i = current->localCount - 1; i >= 0; i--)
	{
		Local * local = &current->locals[i];
		if (local->depth != -1 && local->depth < current->scopeDepth)
		{
			break;
		}

		if (identifiersEqual(name, &local->name))
		{
			error("Variable redeclaration within scope.");
		}
	}
	addLocal(*name);
}

/* Parser variable declare to get name */
static uint8_t parseVariable(const char * errorMessage)
{
	consume(TOKEN_IDENTIFIER, errorMessage);

	declareVariable();
	if (current->scopeDepth > 0) return 0;
	
	return identifierConstant(&parser.previous);
}

/* marks a variable as intalised to prevent reininstalstion */
static void markInitialised() 
{
    if (current->scopeDepth == 0) return;
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

/* defines a new variable */
static void defineVariable(uint8_t global)
{
	if (current->scopeDepth > 0)
	{
        markInitialised();
		return;
	}
	
	emitBytes(OP_DEFINE_GLOBAL, global);
}

/* gets the list of arguments from function call */
static uint8_t argumentList() 
{
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) 
    {
        do 
        {
            expression();
            
            if (argCount == 255) 
            {
                error("Cannot have more than 255 arguments.");
            }

            argCount++; 
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PAREN, "Expected ')' after arguments.");
    return argCount;
}

/* Logical and operator */
static void and_(bool canAssign) 
{
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
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

/* Parse a block statemnt */
static void block()
{
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expected '}' after block statemnent.");
}

/* Compile actual function */
static void function(FunctionType type) 
{
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    /* compile parameter list */
    consume(TOKEN_LEFT_PAREN, "Expected '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) 
    {
        do 
        {
            current->function->arity++;
            if (current->function->arity > 255) 
            {
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
    ObjFunction* function = endCompiler();
    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

    /* Handle closures and upvalues */
    for (int i = 0; i < function->upvalueCount; i++)
    {
	emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
	emitByte(compiler.upvalues[i].index);
    }
}

/* compile function declaration */
static void funDeclaration() 
{
    uint8_t global = parseVariable("Expected function name.");
    markInitialised();
    function(TYPE_FUNCTION);
    defineVariable(global);
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
		emitByte(OP_NIL); /* variables are nil by default */
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

/* Compiles a for statement */
static void forStatement() 
{
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'for'.");

    if (match(TOKEN_SEMICOLON)) 
    {
        /* No initialiser */
    }
    else if (match(TOKEN_VAR)) 
    {
        varDeclaration();
    } 
    else 
    {
        expressionStatement();
    } 

    int loopStart = currentChunk()->count;

    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) 
    {
        expression();
        consume(TOKEN_SEMICOLON, "Expected ';' after loop condition.");


        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
    }

    if (!match(TOKEN_RIGHT_PAREN)) 
    {
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

    if (exitJump != -1) 
    {
        patchJump(exitJump);
        emitByte(OP_POP);
    }

    endScope();
}

/* Compiles an if statement */
static void ifStatement() 
{
    consume(TOKEN_LEFT_PAREN, "Expected '(' after if statement.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition.");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();

    int elseJump = emitJump(OP_JUMP);

    patchJump(thenJump);
    emitByte(OP_POP);

    if (match(TOKEN_ELSE)) statement();
    patchJump(elseJump);
}

/* Compiles a print statement */
static void printStatement()
{
	expression();
	consume(TOKEN_SEMICOLON, "Expected ';' after value.");
	emitByte(OP_PRINT);
}

/* Compile a function return statement */
static void returnStatement() 
{
    /* cannot return from main */
    if (current->type == TYPE_SCRIPT) 
    {
        error("Cannot return from top-level code.");
    }

    if (match(TOKEN_SEMICOLON)) 
    {
        emitReturn();
    }
    else 
    {
        expression();
        consume(TOKEN_SEMICOLON, "Expected ';' after return value.");
        emitByte(OP_RETURN);
    }
}

/* Compile a while statemnt */
static void whileStatement() 
{
    int loopStart = currentChunk()->count;

    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    statement();

    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP);
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
    if (match(TOKEN_FUN)) 
    {
        funDeclaration();
    }
    else if (match(TOKEN_VAR))
	{
		varDeclaration();
	}
	else
	{
		statement();
	}

	if (parser.panicMode) synchronize();
}

/* Parse a generic stateent */
static void statement()
{
	if (match(TOKEN_PRINT))
	{
		printStatement();
	}
    else if (match(TOKEN_FOR)) 
    {
        forStatement();
    }
    else if (match(TOKEN_IF)) 
    {
        ifStatement();
    } 
    else if (match(TOKEN_RETURN)) 
    {
        returnStatement(); 
    } 
    else if (match(TOKEN_WHILE)) 
    {
        whileStatement(); 
    } 
    else if (match(TOKEN_LEFT_BRACE))
	{
		beginScope();
		block();
		endScope();
	}
	else
	{
		expressionStatement();
	}
}

/* Compile is the main function used to create bytecode */
ObjFunction* compile(const char *src)
{
	initScanner(src);
	Compiler compiler;
	initCompiler(&compiler, TYPE_SCRIPT);

	parser.hadError = 0;
	parser.panicMode = 0;
	
	advance();

	while (!match(TOKEN_EOF))
	{
		declaration();
	}
	
    ObjFunction* function = endCompiler();
    return parser.hadError ? NULL : function;
}

