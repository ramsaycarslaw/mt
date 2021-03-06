#ifndef mt_scanner_h
#define mt_scanner_h

typedef enum {
  // Single-character tokens.
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_COLON, 
  TOKEN_MINUS, TOKEN_PLUS, TOKEN_QUESTION,
  TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

  // One or two character tokens.
  TOKEN_BANG, TOKEN_BANG_EQUAL, TOKEN_BACKSLASH,
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL, TOKEN_RIGHT_ARROW,
  TOKEN_GREATER, TOKEN_GREATER_EQUAL,
  TOKEN_LESS, TOKEN_LESS_EQUAL, TOKEN_CARAT,
  TOKEN_PERCENT, TOKEN_PLUS_PLUS, TOKEN_DOT_DOT, 
  TOKEN_PLUS_EQUALS, TOKEN_MINUS_EQUALS, TOKEN_STAR_EQUALS,
  TOKEN_SLASH_EQUALS, TOKEN_CARAT_EQUALS, TOKEN_PERCENT_EQUALS,

  // TYPES
  TOKEN_N64, TOKEN_STR, TOKEN_BOOL,

  // Literals.
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER, TOKEN_IMPORT,

  // Keywords.
  TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE, TOKEN_LET,
  TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
  TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
  TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE, TOKEN_USE, TOKEN_BREAK,
  TOKEN_IN, TOKEN_DEFER,
  TOKEN_CONTINUE, TOKEN_SWITCH, TOKEN_CASE, TOKEN_DEFAULT,

  TOKEN_ERROR,
  TOKEN_NONE,
  TOKEN_EOF
} TokenType;

typedef struct
{
	TokenType type;
	const char *start;
  const char *line_chars;
	int length;
	int line;
} Token;

void initScanner(const char *src);
Token scanToken();

#endif
