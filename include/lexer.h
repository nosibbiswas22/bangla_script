#ifndef BANGLASCRIPT_LEXER_H
#define BANGLASCRIPT_LEXER_H

/*
 * The lexer is part of the compiler pipeline implemented in interpreter.c.
 * This public header reserves the stable front-end entry point for future
 * token-stream consumers.
 */
typedef enum {
    BS_TOKEN_EOF,
    BS_TOKEN_IDENTIFIER,
    BS_TOKEN_NUMBER,
    BS_TOKEN_STRING
} BsTokenKind;

#endif
