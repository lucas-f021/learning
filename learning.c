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
    TokenType type;
    union {
        int int_val;
    } value;
}Token;

typedef struct {
    char *pos;
}Lexer;

static Token casehelper(Lexer *l, TokenType type) {
    Token t;
    t.type = type;
    l->pos++;
    return t;
}

static void print_token(Token t) {
    switch (t.type) {
        case TOK_INT:    printf("INT(%d)\n", t.value.int_val); break;
        case TOK_PLUS:   printf("PLUS\n"); break;
        case TOK_MINUS:  printf("MINUS\n"); break;
        case TOK_STAR:   printf("STAR\n"); break;
        case TOK_SLASH:  printf("SLASH\n"); break;
        case TOK_LPAREN: printf("LPAREN\n"); break;
        case TOK_RPAREN: printf("RPAREN\n"); break;
        case TOK_EOF:    printf("EOF\n"); break;
    }
}


Token next_token(Lexer *l) {
    while(isspace(*l->pos)) {
        l->pos++;
    }
    switch(*l->pos) {
        case '+': 
            return casehelper(l, TOK_PLUS);
        
        case '-': 
            return casehelper(l, TOK_MINUS);
        
        case '*': 
            return casehelper(l, TOK_STAR);
    
        case '/': 
            return casehelper(l, TOK_SLASH);
        
        case '(': 
            return casehelper(l, TOK_LPAREN);
    
        case ')': 
            return casehelper(l, TOK_RPAREN);
        
        case '\0': {
            Token t;
            t.type = TOK_EOF;
            return t;
        }
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
        } else {
            fprintf(stderr, "unknown char: %c\n", *l->pos);
            exit(1);
        }
    }
}

int main(void) {
    Lexer l;
    Token t;

    char *test = "1 + 2 * (3-4)";

    l.pos = test;

    do {
        t = next_token(&l);
        print_token(t);
    } while(t.type != TOK_EOF);

    return 0;
}