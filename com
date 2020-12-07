#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "scanner.h"
#include "object.h"
#include "memory.h"

#if defined(DEBUG_PRINT_CODE) || defined(DEBUG_PRINT_SCAN)
#include "debug.h"
#endif

typedef enum {
	TYPE_FUNCTION,
	TYPE_SCRIPT,
	TYPE_METHOD,
	TYPE_INITIALIZER,
} FunctionType;

typedef struct {
	Token current;
	Token previous;
	bool had_error;
	bool panic_mode;
} Parser;

typedef struct {
	Token name;
	int depth;
	bool is_captured;
} Local;

typedef struct {
	uint8_t index;
	bool is_local;
} Upvalue;

typedef struct Compiler {
	struct Compiler* enclosing;
	ObjFunction* func;
	Upvalue upvalues[UINT8_COUNT];
	FunctionType type;
	Local locals[UINT8_COUNT];
	int local_count;
	int scope_depth;
} Compiler;

typedef struct {
	bool is_in_loop;
	int loop_init; // Stores current loop init position. To use with continue.
	// Stores break statement JUMP. To patch for exit. Doing by this way only one break per loop is admited
	int jump_to_exit;
} LoopMetadata;

typedef enum {
	PREC_NONE,
	PREC_ASSIGNMENT,  // =
	PREC_OR,          // or
	PREC_AND,         // and
	PREC_EQUALITY,    // == !=
	PREC_COMPARISON,  // < > <= >=
	PREC_TERM,        // + -
	PREC_FACTOR,      // * /
	PREC_UNARY,       // ! -
	PREC_CALL,        // . () []
	PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool can_assign);

typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;

typedef struct ClassCompiler {
  struct ClassCompiler* enclosing;
  Token name;
  bool has_superclass;
} ClassCompiler;

static void error(const char* message);

static void expression();
static void statement();
static void block_stmt();
static void print_stmt();
static void expr_stmt();
static void if_stmt();
static void for_stmt();
static void while_stmt();
static void break_stmt();
static void return_stmt();
static void continue_stmt();

static void declaration();
static void class_declaration();
static void func_declaration();
static void function(FunctionType type);
static void var_declaration();

static uint8_t parse_variable(const char* error_msg);
static uint8_t identifier_constant(Token* identifier);
static void define_variable(uint8_t global);
static void declare_variable();
static bool identifier_equals(Token* first, Token* second);
static void mark_initialized();
static int resolve_local(Compiler* compiler, Token* name);
static int resolve_upvalue(Compiler* compiler, Token* name);
static int add_upvalue(Compiler* compiler, uint8_t index, bool is_local);

static int emit_jump(uint8_t op_code);
static void patch_jump(int jump_position);
static void emit_loop(int back_pos);

static void method();
static void grouping(bool can_assign);
static void binary(bool can_assign);
static void unary(bool can_assign);
static void dot(bool can_assign);
static void number(bool can_assign);
static void literal(bool can_assign);
static void string(bool can_assign);
static void variable(bool can_assign);
static void and_(bool can_assign);
static void or_(bool can_assign);
static void call(bool can_assign);
static void this_(bool can_assign);
static void super_(bool can_assign);
static uint8_t argument_list();
static void named_variable(Token name, bool can_assign);

static bool match(TokenType type);
static bool check(TokenType type);
static void syncrhonize();
static void advance();

static Token synthetic_token(const char* text);

static void begin_scope();
static void end_scope();

ParseRule rules[] = {
	{ grouping, call,    PREC_CALL },       // TOKEN_LEFT_PAREN
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_PAREN
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_LEFT_BRACE
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_BRACE
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_COMMA
	{ NULL,     dot,     PREC_CALL },       // TOKEN_DOT
	{ unary,    binary,  PREC_TERM },       // TOKEN_MINUS
	{ NULL,     binary,  PREC_TERM },       // TOKEN_PLUS
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_SEMICOLON
	{ NULL,     binary,  PREC_FACTOR },     // TOKEN_SLASH
	{ NULL,     binary,  PREC_FACTOR },     // TOKEN_STAR
	{ NULL,     binary,  PREC_FACTOR },     // TOKEN_PERCENT
	{ unary,    NULL,    PREC_NONE },       // TOKEN_BANG
	{ NULL,     binary,  PREC_EQUALITY },   // TOKEN_BANG_EQUAL
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL
	{ NULL,     binary,  PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL
	{ NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER
	{ NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
	{ NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS
	{ NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS_EQUAL
	{ variable, NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
	{ string,   NULL,    PREC_NONE },       // TOKEN_STRING
	{ number,   NULL,    PREC_NONE },       // TOKEN_NUMBER
	{ NULL,     and_,    PREC_AND },        // TOKEN_AND
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE
	{ literal,  NULL,    PREC_NONE },       // TOKEN_FALSE
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_FOR
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_FUN
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_IF
	{ literal,  NULL,    PREC_NONE },       // TOKEN_NIL
	{ NULL,     or_,    PREC_OR },          // TOKEN_OR
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_PRINT
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_RETURN
	{ super_,     NULL,    PREC_NONE },       // TOKEN_SUPER
	{ this_,     NULL,    PREC_NONE },       // TOKEN_THIS
	{ literal,  NULL,    PREC_NONE },       // TOKEN_TRUE
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_VAR
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_WHILE
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_BREAK
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_CONTINUE
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_ERROR
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_EOF
};

static ParseRule* get_rule(TokenType type) {
	return &rules[type];
}

Parser parser;

Compiler* current = NULL;

ClassCompiler* current_class = NULL;

LoopMetadata loop_metadata;

static void add_local(Token name) {
	if(current->local_count == UINT8_COUNT) {
		error("Too many local variables in function");
		return;
	}
	Local* local = &current->locals[current->local_count++];
	local->name = name;
	local->depth = -1;
	local->is_captured = false;
}

static void init_compiler(Compiler* compiler, FunctionType type) {
	compiler->enclosing = current;
	compiler->func = NULL; // Garbage Collector Paranoia
	compiler->local_count = 0;
	compiler->scope_depth = 0;
	compiler->type = type;
	compiler->func = new_function();
	current = compiler;

	Local* local = &current->locals[current->local_count++];
	local->depth = 0;
	local->is_captured = false;
	if (type != TYPE_FUNCTION) {
		local->name.start = "this";
		local->name.length = 4;
	} else {
		local->name.start = "";
		local->name.length = 0;
	}

}

static void init_loop_metadata() {
	loop_metadata.is_in_loop = false;
	loop_metadata.loop_init = -1;
	loop_metadata.jump_to_exit = -1;
}

static Chunk* current_chunk() {
	return &current->func->chunk;
}

static void emit_byte(uint8_t byte) {
	write_chunk(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes(uint8_t byte1, uint8_t byte2) {
	emit_byte(byte1);
	emit_byte(byte2);
}

static void error_at(Token* token, const char* message) {
	if (parser.panic_mode) return;
	parser.panic_mode = true;
	fprintf(stderr, "[line %d] Error", token->line);
	if (token->type == TOKEN_EOF) {
		fprintf(stderr, " at end");
	}
	else if (token->type == TOKEN_ERROR) {
		// Nothing.
	}
	else {
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}
	fprintf(stderr, ": %s\n", message);
	parser.had_error = true;
}

static void syncrhonize() {
	parser.panic_mode = false;
	while(parser.current.type != TOKEN_EOF) {
		if(parser.previous.type == TOKEN_SEMICOLON) return;
		switch(parser.current.type) {
		case TOKEN_PRINT:
		case TOKEN_CLASS:
		case TOKEN_FUN:
		case TOKEN_VAR:
		case TOKEN_FOR:
		case TOKEN_IF:
		case TOKEN_WHILE:
		case TOKEN_RETURN:
			return;
		default: ;// DO NOTHING. STILL PANIC
		}
		advance();
	}
}

static void error_at_current(const char* message) {
	error_at(&parser.current, message);
}

static void error(const char* message) {
	error_at(&parser.previous, message);
}

static void advance() {
	parser.previous = parser.current;
	for (;;) {
		parser.current = scan_token();
#ifdef DEBUG_PRINT_SCAN
		printf("%s\n", get_token_str(parser.current.type));
#endif
		if (parser.current.type != TOKEN_ERROR) break;
		error_at_current(parser.current.start);
	}
}

static void consume(TokenType type, const char* message) {
	if (parser.current.type == type) {
		advance();
		return;
	}
	error_at_current(message);
}

static bool match(TokenType type) {
	if(!check(type)) return false;
	advance();
	return true;
}

static bool check(TokenType type) {
	return parser.current.type == type;
}

static void emit_return() {
	if (current->type == TYPE_INITIALIZER) {
		emit_bytes(OP_GET_LOCAL, 0);
	} else {
		emit_byte(OP_NIL);
	}
	emit_byte(OP_RETURN);
}

static uint8_t make_constant(Value value) {
	int constant_index = add_constant(current_chunk(), value);
	if (constant_index > UINT8_MAX) {
		error("Too many constants in one chunk.");
		return 0;
	}
	return (uint8_t)constant_index;
}

static void emit_constant(Value value) {
	emit_bytes(OP_CONSTANT, make_constant(value));
}

static ObjFunction* end_compiler() {
	emit_return();
	ObjFunction* func = current->func;
#ifdef DEBUG_PRINT_CODE
	if (!parser.had_error) {
		disassemble_chunk(current_chunk(), func->name ? func->name->chars : "<GLOBAL>");
	}
#endif
	current = current->enclosing;
	return func;
}

static void parse_precedence(Precedence precedence) {
	advance();
	ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
	if (prefix_rule == NULL) {
		error("Expect expression.");
		return;
	}

	bool can_assign = precedence <= PREC_ASSIGNMENT;
  	prefix_rule(can_assign);

	while (precedence <= get_rule(parser.current.type)->precedence) {
		advance();
		ParseFn infixRule = get_rule(parser.previous.type)->infix;
		infixRule(can_assign);
	}
}

static void declaration() {
	if(match(TOKEN_CLASS)) {
		class_declaration();
	} else if(match(TOKEN_FUN)) {
		func_declaration();
	} else if(match(TOKEN_VAR)) {
		var_declaration();
	} else {
		statement();
	}
	if(parser.panic_mode) syncrhonize();
}

static void class_declaration() {
	consume(TOKEN_IDENTIFIER, "Expected class name");
	Token class_name = parser.previous;
	uint8_t name_constant = identifier_constant(&parser.previous);
	declare_variable();

	emit_bytes(OP_CLASS, name_constant);
	define_variable(name_constant);

	ClassCompiler class_compiler;
	class_compiler.name = parser.previous;
	class_compiler.enclosing = current_class;
	class_compiler.has_superclass = false;
	current_class = &class_compiler;
	
	if(match(TOKEN_LESS)) {
	    consume(TOKEN_IDENTIFIER, "Expected superclass name");
	    variable(false);
		if (identifier_equals(&class_name, &parser.previous)) {
			error("A class cannot inherit from itself.");
		}
		begin_scope();
		add_local(synthetic_token("super"));
		define_variable(0);
	    named_variable(class_name, false);
	    emit_byte(OP_INHERIT);
		class_compiler.has_superclass = true;
	}

	named_variable(class_name, false);
	consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
	while(!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
		method();
	}
	consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
	emit_byte(OP_POP);

	if(class_compiler.has_superclass) {
		end_scope();
	}

	current_class = current_class->enclosing;
}

static Token synthetic_token(const char* text) {
	Token token;
	token.start = text;
	token.length = (int)strlen(text);
	return token;
}

static void method() {
	consume(TOKEN_IDENTIFIER, "Expected method name");
	uint8_t constant = identifier_constant(&parser.previous);
	FunctionType type = TYPE_METHOD;
	if (parser.previous.length == 4 && memcmp(parser.previous.start, "init", 4) == 0) {
		type = TYPE_INITIALIZER;
	}
	function(type);
	emit_bytes(OP_METHOD, constant);
}

static void func_declaration() {
	uint8_t func_name = parse_variable("Expected function name");
	mark_initialized();
	function(TYPE_FUNCTION);
	define_variable(func_name);
}

static void function(FunctionType type) {
	Compiler compiler;
	init_compiler(&compiler, type);
	begin_scope();

	if(type != TYPE_SCRIPT) {
		current->func->name = copy_string(parser.previous.start, parser.previous.length);
	}

	// Compile the parameter list.
	consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
	if(!check(TOKEN_RIGHT_PAREN)) {
		do {
			current->func->arity++;
			if(current->func->arity > 255) {
				error("Too many function arguments");
			}
			uint8_t param = parse_variable("Expected parameter name");
			define_variable(param);
		} while(match(TOKEN_COMMA));
	}
	consume(TOKEN_RIGHT_PAREN, "Expected ) after parameter list in function declaration");

	// The body.
	consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
	block_stmt();

	// Create the function object.
	ObjFunction* func = end_compiler();
	emit_bytes(OP_CLOSURE, make_constant(OBJ_VALUE(func)));

	for(int i = 0; i < func->upvalue_count; i++) {
		emit_byte(compiler.upvalues[i].is_local ? 1 : 0);
		emit_byte(compiler.upvalues[i].index);
	}
}

static void var_declaration() {
	uint8_t global = parse_variable("Expected variable name");
	if(match(TOKEN_EQUAL)) {
		expression();
	} else {
		emit_byte(OP_NIL);
	}
	consume(TOKEN_SEMICOLON, "Expected ; after variable declaration");
	define_variable(global);
}

static uint8_t parse_variable(const char* error_msg) {
	consume(TOKEN_IDENTIFIER, error_msg);

	declare_variable();
	if(current->scope_depth > 0) return 0;
	return identifier_constant(&parser.previous);
}

static uint8_t identifier_constant(Token* identifier) {
	return make_constant(OBJ_VALUE(copy_string(identifier->start, identifier->length)));
}

static void declare_variable() {
	if(current->scope_depth == 0) {
		return;
	}

	Token* name = &parser.previous;

	for(int i = current->local_count - 1; i >= 0; i--) {
		Local* local = &current->locals[i];
		if(local->depth != -1 && local->depth < current->scope_depth) {
			break;
		}
		if(identifier_equals(name, &local->name)) {
			error("Variable with this name already declared in this scope");
		}
	}

	add_local(*name);
}

static bool identifier_equals(Token* first, Token* second) {
	if(first->length != second->length) return false;
	return memcmp(first->start, second->start, first->length) == 0;
}

static void define_variable(uint8_t global) {
	if(current->scope_depth > 0) {
		mark_initialized();
		return;
	};
	emit_bytes(OP_DEFINE_GLOBAL, global);
}

static void mark_initialized() {
	if(current->scope_depth == 0) return;
	current->locals[current->local_count-1].depth = current->scope_depth;
}

static void statement() {
	if(match(TOKEN_PRINT)) {
		print_stmt();
	} else if(match(TOKEN_LEFT_BRACE)) {
		begin_scope();
		block_stmt();
		end_scope();
	} else if(match(TOKEN_IF)) {
		if_stmt();
	} else if(match(TOKEN_WHILE)) {
		while_stmt();
	} else if(match(TOKEN_FOR)) {
		for_stmt();
	} else if(match(TOKEN_BREAK)) {
		break_stmt();
	} else if(match(TOKEN_CONTINUE)) {
		continue_stmt();
	} else if(match(TOKEN_RETURN)) {
		return_stmt();
	} else {
		expr_stmt();
	}
}

static void return_stmt() {
	if(current->type == TYPE_SCRIPT) {
		error("Cannot use return in global");
	}
	if(match(TOKEN_SEMICOLON)) {
		emit_return();
	} else {
		if (current->type == TYPE_INITIALIZER) {
			error("Cannot return a value from an initializer.");
		}
		expression();
		consume(TOKEN_SEMICOLON, "Expected ; after return statement");
		emit_byte(OP_RETURN);
	}
}

static void continue_stmt() {
	consume(TOKEN_SEMICOLON, "Expected ; after break");
	if(!loop_metadata.is_in_loop) {
		error("Illegal continue statment. You can only use continue inside loops.");
	}
	emit_loop(loop_metadata.loop_init);
}

static void break_stmt() {
	consume(TOKEN_SEMICOLON, "Expected ; after break");
	if(!loop_metadata.is_in_loop) {
		error("Illegal break statment. You can only use break inside loops.");
	}
	if(loop_metadata.jump_to_exit != -1) {
		error("You can only have one break inside loop");
	}
	loop_metadata.jump_to_exit = emit_jump(OP_JUMP);
}

static void handle_loop_metadata() {
	if(loop_metadata.jump_to_exit != -1) {
		patch_jump(loop_metadata.jump_to_exit);
	}
	init_loop_metadata();
}

static void start_loop_metadata() {
	loop_metadata.is_in_loop = true;
	loop_metadata.loop_init = current_chunk()->size;
}

static void for_stmt() {
	begin_scope();
	start_loop_metadata(); // Enable continue and break statements
	consume(TOKEN_LEFT_PAREN, "Expected ( after for");
	if(match(TOKEN_SEMICOLON)) {
		// NO INITIALIZER
	} else if (match(TOKEN_VAR)) {
		var_declaration();
	} else {
		expr_stmt();
	}

	int loop_back = current_chunk()->size;

	int exit_jump = -1;
	if(!match(TOKEN_SEMICOLON)) {
		expression();
		consume(TOKEN_SEMICOLON, "Expected ';' after loop condition");

		// JUMP out loop
		exit_jump = emit_jump(OP_JUMP_IF_FALSE);
		emit_byte(OP_POP); // clean condition
	}

	if(!match(TOKEN_RIGHT_PAREN)) {
		int body_jump = emit_jump(OP_JUMP);
		int incremental_start = current_chunk()->size;
		expression();
		emit_byte(OP_POP);
		consume(TOKEN_RIGHT_PAREN, "Expected ) after for");

		emit_loop(loop_back);
		loop_back = incremental_start;
		patch_jump(body_jump);
	}

	statement();

	emit_loop(loop_back);
	if(exit_jump != -1) {
		patch_jump(exit_jump);
		emit_byte(OP_POP); // clean condition
	}
	handle_loop_metadata();
	end_scope();
}

static void while_stmt() {
	int loop_start = current_chunk()->size;
	start_loop_metadata(); // Enable continue and break statements
	consume(TOKEN_LEFT_PAREN, "Expected ( after while");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expected ) after while");
	int exit_pos = emit_jump(OP_JUMP_IF_FALSE);
	emit_byte(OP_POP);
	statement();
	emit_loop(loop_start);
	patch_jump(exit_pos);
	emit_byte(OP_POP);
	handle_loop_metadata();
}

static void emit_loop(int back_pos) {
	emit_byte(OP_LOOP);
	int offset = current_chunk()->size - back_pos + 2;
	if(offset > UINT16_MAX) {
		error("Body to large in loop");
	}
	emit_byte((offset >> 8) & 0xff);
	emit_byte(offset & 0xff);
}

static void if_stmt() {
	consume(TOKEN_LEFT_PAREN, "Expected ( after if");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expected ) after condition in if");
	int then_jump_pos = emit_jump(OP_JUMP_IF_FALSE);
	emit_byte(OP_POP);
	statement();
	int else_jump_pos = emit_jump(OP_JUMP);
	patch_jump(then_jump_pos);
	emit_byte(OP_POP);
	if(match(TOKEN_ELSE)) {
		statement();
	}
	patch_jump(else_jump_pos);
}

static int emit_jump(uint8_t op_code) {
	emit_byte(op_code);
	emit_byte(0xff); // Temporal chunk direction to jump
	emit_byte(0xff);
	return current_chunk()->size - 2;
}

static void patch_jump(int jump_position) {
	Chunk* chunk = current_chunk();

	// Check if the size to jump is too big
	int jump = chunk->size - jump_position - 2;
	if(jump > UINT16_MAX) {
		error("Too much code to jump over");
	}

	// Write a clean 16 bit integer as jump argument
	chunk->code[jump_position] = (jump >> 8) & 0xff;
	chunk->code[jump_position + 1] = jump & 0xff;
}

static void begin_scope() {
	current->scope_depth++;
}

static void end_scope() {
	current->scope_depth--;
	while(current->local_count > 0 &&
		  current->locals[current->local_count - 1].depth > current->scope_depth) {
		if(current->locals[current->local_count - 1].is_captured) {
			emit_byte(OP_CLOSE_UPVALUE);
		} else {
			emit_byte(OP_POP);
		}
		current->local_count--;
	}
}

static void block_stmt() {
	while(!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
		declaration();
	}
	consume(TOKEN_RIGHT_BRACE, "Expected } to match { in block");
}

static void print_stmt() {
	expression();
	consume(TOKEN_SEMICOLON, "Expected ; after value");
	emit_byte(OP_PRINT);
}

static void expr_stmt() {
	expression();
	consume(TOKEN_SEMICOLON, "Expected ; after value");
	emit_byte(OP_POP);
}

static void expression() {
	parse_precedence(PREC_ASSIGNMENT);
}

static void number(bool can_assign) {
	double value = strtod(parser.previous.start, NULL);
	emit_constant(NUMBER_VALUE(value));
}

static void grouping(bool can_assign) {
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

static void unary(bool can_assign) {
	TokenType operatorType = parser.previous.type;
	parse_precedence(PREC_UNARY); //compile operand
	// Emit the operator instruction.
	switch (operatorType) {
	case TOKEN_BANG: emit_byte(OP_NOT); break;
	case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
	default:
		return; // Unreachable.
	}
}

static void binary(bool can_assign) {
	// Remember the operator.
	TokenType operatorType = parser.previous.type;

	// Compile the right operand.
	ParseRule* rule = get_rule(operatorType);
	parse_precedence((Precedence)(rule->precedence + 1));

	// Emit the operator instruction.
	switch (operatorType) {
	case TOKEN_PLUS: emit_byte(OP_ADD); break;
	case TOKEN_MINUS: emit_byte(OP_SUBSTRACT); break;
	case TOKEN_STAR: emit_byte(OP_MULTIPLY); break;
	case TOKEN_SLASH: emit_byte(OP_DIVIDE); break;
	case TOKEN_BANG_EQUAL: emit_bytes(OP_EQUAL, OP_NOT); break;
	case TOKEN_EQUAL_EQUAL: emit_byte(OP_EQUAL); break;
	case TOKEN_GREATER: emit_byte(OP_GREATER); break;
	case TOKEN_GREATER_EQUAL: emit_bytes(OP_LESS, OP_NOT); break;
	case TOKEN_LESS: emit_byte(OP_LESS); break;
	case TOKEN_LESS_EQUAL: emit_bytes(OP_GREATER, OP_NOT); break;
	case TOKEN_PERCENT: emit_byte(OP_MODULE); break;
	default:
		return; // Unreachable.
	}
}

static void literal(bool can_assign) {
	switch (parser.previous.type) {
	case TOKEN_NIL: emit_byte(OP_NIL); break;
	case TOKEN_TRUE: emit_byte(OP_TRUE); break;
	case TOKEN_FALSE: emit_byte(OP_FALSE); break;
	default:
		return; // Unreachable
	}
}

static void and_(bool can_assign) {
	// Logical AND is implemented using JUMP.
	// IS NOT THE WAY TO DO IT. JUST FOR LEARNING PURPOSES.
	int jump = emit_jump(OP_JUMP_IF_FALSE);
	emit_byte(OP_POP);
	parse_precedence(PREC_AND);
	patch_jump(jump);
}

static void or_(bool can_assign) {
	// Logical OR is implemented using JUMP.
	// IS NOT THE WAY TO DO IT. JUST FOR LEARNING PURPOSES.
	int else_jump = emit_jump(OP_JUMP_IF_FALSE);
	int jump = emit_jump(OP_JUMP);
	patch_jump(else_jump);
	emit_byte(OP_POP);
	parse_precedence(PREC_AND);
	patch_jump(jump);
}

static void this_(bool can_assign) {
	if (current_class == NULL) {
		error("Cannot use 'this' outside of a class.");
		return;
	}
	variable(false);
}

static void string(bool can_assign) {
	emit_constant(OBJ_VALUE(copy_string(parser.previous.start + 1, parser.previous.length - 2)));
}

static void variable(bool can_assign) {
	named_variable(parser.previous, can_assign);
}

static void call(bool can_assign) {
	uint8_t arg_count = argument_list();
	emit_bytes(OP_CALL, arg_count);
}

static void dot(bool can_assign) {
	consume(TOKEN_IDENTIFIER, "Expected identifier after <dot>");
	uint8_t name = identifier_constant(&parser.previous);

	if(can_assign && match(TOKEN_EQUAL)) {
		expression();
		emit_bytes(OP_SET_PROPERTY, name);
	} else if(match(TOKEN_LEFT_PAREN)) {
		uint8_t arg_count = argument_list();
		emit_bytes(OP_INVOKE, name);
		emit_byte(arg_count);
	} else {
		emit_bytes(OP_GET_PROPERTY, name);
	}
}

static void super_(bool can_assign) {
	if (current_class == NULL) {
		error("Cannot use 'super' outside of a class.");
	} else if (!current_class->has_superclass) {
		error("Cannot use 'super' in a class with no superclass.");
	}
	consume(TOKEN_DOT, "Expect '.' after 'super'");
	consume(TOKEN_IDENTIFIER, "Expect superclass method name");
	uint8_t name = identifier_constant(&parser.previous);
	named_variable(synthetic_token("this"), false);
	if(match(TOKEN_LEFT_PAREN)) {
		uint8_t arg_count = argument_list();
		named_variable(synthetic_token("super"), false);
		emit_bytes(OP_SUPER_INVOKE, name);
		emit_byte(arg_count);
	} else {
		named_variable(synthetic_token("super"), false);
		emit_bytes(OP_GET_SUPER, name);
	}
}

static uint8_t argument_list() {
	uint8_t args = 0;
	if(!check(TOKEN_RIGHT_PAREN)) {
		do {
			expression();
			if(args == 255) {
				error("Cannot have more than 255 arguments in function call");
			}
			args++;
		} while(match(TOKEN_COMMA));
	}
	consume(TOKEN_RIGHT_PAREN, "Expected ) after function call");
	return args;
}

static void named_variable(Token name, bool can_assign) {
	uint8_t set_op, get_op;
	int arg = resolve_local(current, &name);
	if(arg != -1) {
		set_op = OP_SET_LOCAL;
		get_op = OP_GET_LOCAL;
	} else if((arg = resolve_upvalue(current, &name)) != -1) {
		set_op = OP_SET_UPVALUE;
		get_op = OP_GET_UPVALUE;
	} else {
		arg = identifier_constant(&name);
		set_op = OP_SET_GLOBAL;
		get_op = OP_GET_GLOBAL;
	}

	if(can_assign && match(TOKEN_EQUAL)) {
		expression();
		emit_bytes(set_op, (uint8_t)arg);
	} else {
		emit_bytes(get_op, (uint8_t)arg);
	}
}

static int resolve_local(Compiler* compiler, Token* name) {
	for(int i = compiler->local_count - 1; i >= 0; i--) {
		Local* local = &compiler->locals[i];
		if(identifier_equals(&local->name, name)) {
			if(local->depth == -1) {
				error("Cannot read local variable in its own initializer");
			}
			return i;
		}
	}
	return -1;
}

static int resolve_upvalue(Compiler* compiler, Token* name) {
	if(compiler->enclosing == NULL) return -1;
	int local = resolve_local(compiler->enclosing, name);
	if(local != -1) {
		compiler->enclosing->locals[local].is_captured = true;
		return add_upvalue(compiler, (uint8_t)local, true);
	}
	int outer_upvalue = resolve_upvalue(compiler->enclosing, name);
	if(outer_upvalue != -1) {
		return add_upvalue(compiler, (uint8_t)outer_upvalue, false);
	}
	return -1;
}

static int add_upvalue(Compiler* compiler, uint8_t index, bool is_local) {
	int upvalue_count = compiler->func->upvalue_count;
	for(int i = 0; i < upvalue_count; i++) {
		Upvalue* upvalue = &compiler->upvalues[i];
		if(upvalue->index == index && upvalue->is_local == is_local) {
			return i;
		}
	}
	if(upvalue_count >= UINT8_COUNT) {
		error("Upvalues overflow. Your function access to many outside variables");
		return 0;
	}
	compiler->upvalues[upvalue_count].is_local = is_local;
	compiler->upvalues[upvalue_count].index = index;
	return compiler->func->upvalue_count++;
}

ObjFunction* compile(const char* source) {
	init_scanner(source);
	init_loop_metadata();
	Compiler compiler;
	init_compiler(&compiler, TYPE_SCRIPT);
	parser.had_error = false;
	parser.panic_mode = false;
	advance();
	while(!match(TOKEN_EOF)) {
		declaration();
	}
	ObjFunction* func = end_compiler();
	return parser.had_error ? NULL : func;
}

void mark_compiler_roots() {
	Compiler* compiler = current;
	while (compiler != NULL) {
		mark_object((Obj*)compiler->func);
		compiler = compiler->enclosing;
	}
}
