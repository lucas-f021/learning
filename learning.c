#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "arena.h"
#include "hash.h"

/* ===== LEXER ===== */

typedef enum {
    TOK_INT,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_EOF,
    TOK_IDENT,
    TOK_LET,
    TOK_EQ,
    TOK_SEMI
} TokenType;

typedef struct {
    TokenType type;
    union {
        int int_val;
        char *ident;
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
        
        case '=':
            return casehelper(l, TOK_EQ);

        case ';':
            return casehelper(l, TOK_SEMI);

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
        } else if(isalpha(*l->pos)) {
            char *tmp = l->pos;
            while(isalnum(*l->pos)) {
                l->pos++;
            }
            size_t len = l->pos - tmp;
            if(len == 3) {
                if(strncmp(tmp, "let", 3) == 0) {
                    Token t;
                    t.type = TOK_LET;
                    return t;
                }
            }
            Token t;
            t.type = TOK_IDENT;
            t.value.ident = strndup(tmp, len);
            return t;
        }
        else {
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
        case TOK_IDENT: printf("IDENT(%s)\n", t.value.ident); break;
        case TOK_LET: printf("LET\n"); break;
        case TOK_EQ: printf("EQ\n"); break;
        case TOK_SEMI: printf("SEMI\n"); break;
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
    NODE_BINOP,
    NODE_IDENT,
    NODE_LET
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
        char *ident_name;
        struct {
            char *name;
            struct Node *value;
        } let;
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

static Token advance(Parser *p) {
    Token tmp;

    tmp = p->curr;

    p->curr= next_token(p->l);

    return tmp;
}

Node *parse_expr(Parser *p);
Node *parse_term(Parser *p);
Node *parse_factor(Parser *p);
Node *parse_let(Parser *p);
Node *parse_statement(Parser *p);

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
            fprintf(stderr, "Error no closing parenthesis\n");
            exit(1);
        }
        advance(p);
        return tmp;
    } 
    if(p->curr.type == TOK_IDENT) {
        Node *new = arena_alloc(p->arena, sizeof(Node));
        if(new == NULL) {
            fprintf(stderr, "Arena alloc error\n");
            exit(1);
        }
        new->type = NODE_IDENT;
        new->uni.ident_name = p->curr.value.ident;
        advance(p);
        return new;
    } else {
        fprintf(stderr, "unexpected token\n");
        exit(1);
    }
}

Node *parse_let(Parser *p) {
    char *tmp;
    advance(p);
    if(p->curr.type == TOK_IDENT) {
        tmp = p->curr.value.ident;
    } else {
        fprintf(stderr, "Expected identifier\n");
        exit(1);
    }
    advance(p);
    if(p->curr.type == TOK_EQ) {
        advance(p);
    } else {
        fprintf(stderr, "Expected =\n");
        exit(1);
    }
    Node *expr = parse_expr(p);
    if(p->curr.type == TOK_SEMI) {
        advance(p); } 
        else {
        fprintf(stderr, "Expected ;\n");
        exit(1);
    }
    Node *new = arena_alloc(p->arena, sizeof(Node));
    if(new == NULL) {
        fprintf(stderr, "Arena alloc error\n");
        exit(1);
    }
    new->type = NODE_LET;
    new->uni.let.name = tmp;
    new->uni.let.value = expr;
    return new;
}

Node *parse_statement(Parser *p) {
    if(p->curr.type == TOK_LET) {
        return parse_let(p);
    }
    Node *new = parse_expr(p);
    if(p->curr.type == TOK_SEMI) {
        advance(p);
        return new;
    } else {
        fprintf(stderr, "Expected ;\n");
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

int eval(Node *n, HashMap *env) {
    if(n->type == NODE_INT) {
        return n->uni.int_value;
    }
    if(n->type == NODE_IDENT) {
        int out;
        bool found = hm_get(env, n->uni.ident_name, &out);
        if(found != true) {
            fprintf(stderr, "Hashmap Failure\n");
            exit(1);
        } else {
            return out;
        }
    }
    if(n->type == NODE_LET) {
        int val = eval(n->uni.let.value, env);
        hm_insert(env, n->uni.let.name, val);
        return 0;
    }
    else {
        int x = eval(n->uni.binop.left, env);
        int y = eval(n->uni.binop.right, env);
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
    HashMap *env = hm_create();
    char x[128];
    while(1) {
        printf("Enter expression to compute: ");
        char *res = fgets(x, 128, stdin);

        if(res == NULL) {
            arena_destroy(a);
            hm_destroy(env);
            break;
        } 

        l.pos=x;

        init_parser(&p, &l, a);

        while(p.curr.type != TOK_EOF) {
            Node *tmp = parse_statement(&p);
            int val = eval(tmp, env);
            if(tmp->type != NODE_LET) {
                printf("%d\n", val);

            }
        }
            arena_reset(a);
    }

    return 0;
}