#include <stdio.h>
#include <string.h>

#include "../include/common.h"
#include "../include/scanner.h"

typedef struct
{
  const char *start;
  const char *current;
  int line;
} Scanner;

Scanner scanner;

/* Innit scanner struct state */
void initScanner(const char *src)
{
  scanner.start = src;
  scanner.current = src;
  scanner.line = 1;
}

/* returns true if a member of the alphabet or underscore */
static int isAlpha(char c)
{
  return (c >= 'a' && c <= 'z') ||
    (c >= 'A' && c <= 'Z') ||
    c == '_';
}

/* Returns true if the char is 0123456789 in that set */
static int isDigit(char c)
{
  return c >= '0' && c <= '9';
}

/* Is the given char the end of the source string */
static int isAtEnd()
{
  return *scanner.current == '\0';
}

/* Checks the next token is of the expected type */
static int match(char expected)
{
  if (isAtEnd()) return 0;
  if (*scanner.current != expected) return 0;

  scanner.current++;
  return 1;
}

/* Advance to the next token */
static char advance()
{
  scanner.current++;
  return scanner.current[-1];
}

/* Allows functions to lookahead one char */
static char peek()
{
  return *scanner.current;
}

/* As not to consume the first slash in comments */
static char peekNext()
{
  if (isAtEnd()) return '\0';
  return scanner.current[1];
}

/* Create a new token from a type and current position */
static Token makeToken(TokenType type)
{
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;

  return token;
}

/* 
   Create an error token rather than use exit the compiler
   can try error recovery 
   */
static Token errorToken(const char * errorMessage)
{
  Token token;
  token.type = TOKEN_ERROR;
  token.start = errorMessage;
  token.length = (int)strlen(errorMessage);
  token.line = scanner.line;

  return token;
}

/* Consumes chars until it encounters a non whitespace char */
static void skipWhitespace()
{
  for (;;)
  {
    char c = peek();
    switch (c)
    {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;

      case '\n':
        scanner.line++;
        advance();
        break;

      case '/':
        if (peekNext() == '/')
        {
          // A comment goes until the end of the line.
          while (peek() != '\n' && !isAtEnd()) advance();
        }
        else if (peekNext() == '*') 
        {
          // C - style comments
          while (peek() != '*' && peekNext() != '/' && !isAtEnd()) advance();
          advance();
        }
        else
        {
          return;
        }
        break;

      default:
        return;
    }
  }
}

static TokenType checkKeyword(int start, int length, const char* rest, TokenType type)
{
  if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0)
  {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

static TokenType identifierType()
{
  switch (scanner.start[0])
  {
    case '&': return checkKeyword(1, 1, "&", TOKEN_AND); // and
    case 'c': 
    {
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) 
        {
          case 'o': return checkKeyword(2, 6, "ntinue", TOKEN_CONTINUE);
          case 'l': return checkKeyword(2, 3, "ass", TOKEN_CLASS);
          case 'a': return checkKeyword(2, 2, "se", TOKEN_CASE);
        }
        break;
      }     
      break;    
    } 
    case 'd': 
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'e':
            switch(scanner.start[2]) {
              case 'f':
                switch (scanner.start[3]) {
                  case 'e': return checkKeyword(4, 1, "r", TOKEN_DEFER);
                  case 'a': return checkKeyword(4, 3, "ult", TOKEN_DEFAULT);
                }
            }
            break;
        }
        break;
      }
    case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
    case 'i': 
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'f':
            return TOKEN_IF;
          case 'n':
            return TOKEN_IN;
        }
      }
    case 'l': return checkKeyword(1, 2, "et", TOKEN_LET);  // let x: type = type_inst;
    case 'n': 
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'i': return checkKeyword(2, 1, "l", TOKEN_NIL);
          case '6': return checkKeyword(2, 1, "4", TOKEN_N64);
        }
        break;
      }
    case '|': return checkKeyword(1, 1, "|", TOKEN_OR); // or
    case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
    case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 's': {
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'u': return checkKeyword(2, 3, "per", TOKEN_SUPER);
          case 'w': return checkKeyword(2, 4, "itch", TOKEN_SWITCH);
          case 't': return checkKeyword(2, 1, "r", TOKEN_STR);
        }
        break;
      }
      break;
    }
    case 'u': return checkKeyword(1, 2, "se", TOKEN_USE);
    case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
    case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    case 'b': return checkKeyword(1, 4, "reak", TOKEN_BREAK);
    case 'f':
              if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1])
                {
                  case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                  case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                  case 'n': return TOKEN_FUN;
                }
                case 't':
                if (scanner.current - scanner.start > 1) {
                  switch (scanner.start[1]) {
                    case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                  }
                }
                break;		
              }
              break;
  }

  return TOKEN_IDENTIFIER;
}

static Token identifier()
{
  while (isAlpha(peek()) || isDigit(peek())) advance();

  return makeToken(identifierType());
}

static Token string()
{
  while (peek() != '"' && !isAtEnd())
  {
    if (peek() == '\n') scanner.line++;
    advance();
  }

  if (isAtEnd()) return errorToken("Unterminated string: A string is missing a closing quote.");

  // The closing quote.
  advance();
  return makeToken(TOKEN_STRING);
}

static Token number()
{
  while (isDigit(peek())) advance();

  // Look for a fractional part.
  if (peek() == '.' && isDigit(peekNext()))
  {
    // Consume the ".".
    advance();

    while (isDigit(peek())) advance();
  }

  return makeToken(TOKEN_NUMBER);
}

/* Scan the token in place */
Token scanToken()
{
  skipWhitespace();
  scanner.start = scanner.current;

  if (isAtEnd()) return makeToken(TOKEN_EOF);

  char c = advance();
  if (isAlpha(c)) return identifier();
  if (isDigit(c)) return number();

  /* One or two characer tokens */
  switch (c)
  {
    // single chars
    case '(': return makeToken(TOKEN_LEFT_PAREN);
    case ')': return makeToken(TOKEN_RIGHT_PAREN);
    case '{': return makeToken(TOKEN_LEFT_BRACE);
    case '}': return makeToken(TOKEN_RIGHT_BRACE);
    case '[': return makeToken(TOKEN_LEFT_BRACKET);
    case ']': return makeToken(TOKEN_RIGHT_BRACKET);
    case ';': return makeToken(TOKEN_SEMICOLON);
    case ',': return makeToken(TOKEN_COMMA);
    case ':': return makeToken(TOKEN_COLON);
    case '?': return makeToken(TOKEN_QUESTION);
    case '\\': return makeToken(TOKEN_BACKSLASH);
    case '.': {
      if (match('.')) {
        // used in list comprehensions
        return makeToken(TOKEN_DOT_DOT);
      }
      return makeToken(TOKEN_DOT);
    }
    case '-': 
      if (match('=')) {
        return makeToken(TOKEN_MINUS_EQUALS);
      } else if (match('>')) {
        return makeToken(TOKEN_RIGHT_ARROW);
      }
      return makeToken(TOKEN_MINUS);
    case '+':  {
                 if (match('+')) {
                   return makeToken(TOKEN_PLUS_PLUS);
                 } else if (match('=')) {
                  return makeToken(TOKEN_PLUS_EQUALS);
                 }
                 return makeToken(TOKEN_PLUS);
               }
    case '/': 
      if (match('=')) {
        return makeToken(TOKEN_SLASH_EQUALS);
      }
      return makeToken(TOKEN_SLASH);
    case '*': 
      if (match('=')) {
        return makeToken(TOKEN_STAR_EQUALS);
      }
      return makeToken(TOKEN_STAR);
    case '^': 
      if (match('=')) {
        return makeToken(TOKEN_CARAT_EQUALS);
      }
      return makeToken(TOKEN_CARAT);
    case '%': 
      if (match('=')) {
        return makeToken(TOKEN_PERCENT_EQUALS);
      }
      return makeToken(TOKEN_PERCENT);
              // two or one 
    case '!':
              return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
              return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
              return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
              return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '|':
              if (!match('|'))
              {
                return errorToken("Unexpexted character after |");
              }
              return makeToken(TOKEN_OR);
    case '&':
              if (!match('&'))
              {
                return errorToken("Unexpexted character after |");
              }
              return makeToken(TOKEN_AND);
              // literals
    case '"': return string();
  }

  return errorToken("Unexpected character.");
}


