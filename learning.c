#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "arena.h"

/* ===== LEXER ===== */

typedef enum {
    TOK_INT,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_EOF
} TokenType;

typedef struct {
    TokenType type;
    union {
        int int_val;
    } value;
} Token;

typedef struct {
    char *pos;
} Lexer;

static Token casehelper(Lexer *l, TokenType type) {
    Token t;
    t.type = type;
    l->pos++;
    return t;
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

/* ===== PARSER ===== */

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV
} OpType;

typedef enum {
    NODE_INT,
    NODE_BINOP
} NodeType;

typedef struct Node {
    NodeType type;
    union {
        int int_value;
        struct {
            OpType op;
            struct Node *left;
            struct Node *right;
        } binop;
    } uni;
} Node;

typedef struct {
    Lexer *l;
    Token curr;
    Arena *arena;
} Parser;

void init_parser(Parser *p, Lexer *lex, Arena *arena) {
    p->l = lex;
    p->curr = next_token(lex);
    p->arena = arena;
}

static Token peek(Parser *p) {
    return p->curr;
}

static Token advance(Parser *p) {
    Token tmp;

    tmp = p->curr;

    p->curr= next_token(p->l);

    return tmp;
}

Node *parse_expr(Parser *p);
Node *parse_term(Parser *p);
Node *parse_factor(Parser *p);

Node *parse_expr(Parser *p) {
    Node *left = parse_term(p);
    while(p->curr.type == TOK_PLUS || p->curr.type == TOK_MINUS) {
        Node *new = arena_alloc(p->arena, sizeof(Node));
        if(new == NULL) {
            fprintf(stderr, "Arena alloc error\n");
            exit(1);
        }
        new->type = NODE_BINOP;
        if(p->curr.type == TOK_PLUS) {
            new->uni.binop.op = OP_ADD;
        } else {
            new->uni.binop.op = OP_SUB;
        }
        new->uni.binop.left = left;
        advance(p);

        Node *right = parse_term(p);
        new->uni.binop.right = right;
        left = new;
    }
    return left;
}

Node *parse_term(Parser *p) {
    Node *left = parse_factor(p);
    while(p->curr.type == TOK_STAR || p->curr.type == TOK_SLASH) {
        Node *new = arena_alloc(p->arena, sizeof(Node));
        if(new == NULL) {
            fprintf(stderr, "Arena alloc error\n");
            exit(1);
        }
        new->type = NODE_BINOP;
        if(p->curr.type == TOK_STAR) {
            new->uni.binop.op = OP_MUL;
        } else {
            new->uni.binop.op = OP_DIV;
        }
        new->uni.binop.left = left;
        advance(p);

        Node *right = parse_factor(p);
        new->uni.binop.right = right;
        left = new;
    }
    return left;
}

Node *parse_factor(Parser *p) {
    if(p->curr.type == TOK_INT) {
        Node *new = arena_alloc(p->arena, sizeof(Node));
        if(new == NULL) {
            fprintf(stderr, "Arena alloc error\n");
            exit(1);
        }
        new->type = NODE_INT;
        new->uni.int_value = p->curr.value.int_val;
        advance(p);
        return new;
    }
    if(p->curr.type == TOK_LPAREN) {
        advance(p);
        Node *tmp =parse_expr(p);
        if(p->curr.type != TOK_RPAREN) {
            fprintf(stderr, "No closing parenthesis\n");
            exit(1);
        }
        advance(p);
        return tmp;
    } else {
        fprintf(stderr, "unexpected token\n");
        exit(1);
    }
}

void print_ast(Node *n) {
    if(n->type == NODE_INT) {
        printf("%d", n->uni.int_value);
    }
    else {
        print_ast(n->uni.binop.left);
        switch(n->uni.binop.op) {
            case OP_ADD: 
                printf("+");
                break;
            case OP_SUB:
                printf("-");
                break;
            case OP_MUL:
                printf("*");
                break;
            case OP_DIV:
                printf("/");
                break;
        }
        print_ast(n->uni.binop.right);
    }
}

/* ===== EVALUATOR ===== */

int eval(Node *n) {
    if(n->type == NODE_INT) {
        return n->uni.int_value;
    }
    else {
        int x = eval(n->uni.binop.left);
        int y = eval(n->uni.binop.right);
        switch(n->uni.binop.op) {
            case OP_ADD: 
                return x + y;
            case OP_SUB:
                return x - y;
            case OP_MUL:
                return x * y;
            case OP_DIV:
                return x / y;
        }
    }
}

/* ===== MAIN ===== */

int main(void) {
    Parser p;
    Lexer l;
    Arena *a = arena_create(4096);
    char x[128];
    while(1) {
        printf("Enter expression to compute: ");
        char *res = fgets(x, 128, stdin);

        if(res == NULL) {
            arena_destroy(a);
            break;
        } 

        l.pos=x;

        init_parser(&p, &l, a);

        printf("%d\n", eval(parse_expr(&p)));
        arena_reset(a);
    }

    return 0;
}