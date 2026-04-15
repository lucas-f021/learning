#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    TOK_INT,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_EOF
}TokenType;

typedef struct {
    enum TokenType type;
    union {
        int int_val;
    } value;
}Token;

typedef struct {
    char *pos;
}Lexer;

Token next_token(Lexer *l) {
    while(isspace(*l->pos)) {
        l->pos++;
    }
    switch(*l->pos) {
        default:
        if(isdigit(*l->pos)) {
            char *tmp = l->pos;
            while(isdigit(*l->pos)) {
                l->pos++;
            }
            Token t;
            t.type = TOK_INT;
            t.value.int_val = atoi(tmp);
            return t;
        }
    }
}