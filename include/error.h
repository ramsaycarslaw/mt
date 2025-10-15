#ifndef ERROR_H
#define ERROR_H

#include "scanner.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
    // 1xx: Lexical Errors (Scanner)
    E_LEXICAL_UNTERMINATED_STRING       = 101,
    E_LEXICAL_INVALID_CHARACTER         = 102,
    E_LEXICAL_NUMBER_TOO_LARGE          = 103,

    // ----------------------------------------------------
    // 2xx: Compiler/Parser Errors (Grouped by Topic)
    // ----------------------------------------------------

    // 200-210: Basic Syntax Errors (Expected/Missing Tokens)
    E_COMPILER_ERROR                    = 200, // Generic/Catch-all
    E_COMPILER_EXPECTED_EXPRESSION      = 201,
    E_COMPILER_EXPECTED_COLON           = 202,
    E_COMPILER_EXPECTED_DOT             = 203,
    E_COMPILER_EXPECTED_RBRACE          = 204, // Expected '}'
    E_COMPILER_EXPECTED_RPAREN          = 205, // Expected ')'
    E_COMPILER_EXPECTED_IDENTIFIER      = 206,
    E_COMPILER_EXPECTED_RBRACKET        = 207, // Expected ']'
    E_COMPILER_LOOP_BODY_TOO_LARGE      = 209, // Structural Limits (misplaced, but preserving value)

    // 211-220: Resource/Limit Errors (Capacity)
    E_COMPILER_TOO_MANY_CONSTANTS       = 211,
    E_COMPILER_JUMP_TOO_LARGE           = 213,
    E_COMPILER_TUPLE_TOO_LARGE          = 215,
    E_COMPILER_TOO_MANY_ARGS            = 216,
    E_COMPILER_LIST_TOO_LARGE           = 217,
    E_COMPILER_TOO_MANY_LOCALS          = 218,
    E_COMPILER_TOO_MANY_CLOSURES        = 219,

    // 221-230: Class/OOP-Related Errors (Property/Super)
    E_COMPILER_EXPECTED_PROPERTY_NAME   = 221,
    E_COMPILER_SUPER_NOT_ALLOWED        = 223,
    E_COMPILER_EXPECTED_SUPERCLASS_NAME = 225,
    E_COMPILER_RESERVED_KEYWORD         = 227, // E.g., 'this' outside a class

    // 231-240: Variable/Declaration/Scope Errors
    E_COMPILER_INVALID_ASSIGNMENT       = 229,
    E_COMPILER_VARIABLE_REDECLARATION   = 230,
    E_COMPILER_LOCAL_RESOLVER_ERROR     = 231, // Reading local in own initializer
    E_COMPILER_EXPECTED_LPAREN          = 232, // Expected '('
    E_COMPILER_EXPECTED_LBRACE          = 233, // Expected '{'
    E_COMPILER_EXPECTED_ARROW           = 234, // Expected '->' (Lambda)
    E_COMPILER_EXPECTED_METHOD_NAME     = 235,
    E_COMPILER_EXPECTED_CLASS_NAME      = 236,
    E_COMPILER_SELF_INHERITANCE         = 237,
    E_COMPILER_EXPECTED_SEMICOLON       = 238,
    E_COMPILER_UNEXPECTED_BREAK         = 239, // Control Flow (misplaced, but preserving value)
    E_COMPILER_EXPECTED_BREAK           = 240, // Control Flow (misplaced, but preserving value)

    // 241-250: Control Flow Errors (Continuation of 239/240)
    E_COMPILER_UNEXPECTED_CONTINUE      = 241,
    E_COMPILER_EXPECTED_CONTINUE        = 242,
    E_COMPILER_EXPECTED_IN              = 243,
    E_COMPILER_MALFORMED_SWITCH         = 244,
    E_COMPILER_STATEMENT_NOT_ALLOWED    = 245,
    E_COMPILER_UNEXPECTED_RET           = 246,
    E_COMPILER_UNEXPECTED_DEFER         = 247,
} ErrorCode;

extern const char* CURRENT_FILE_PATH;

void reportError(const Token* token, ErrorCode errorCode, const char* errorMessage);

const char* getErrorMessageString(ErrorCode errorCode);

#endif // ERROR_H
