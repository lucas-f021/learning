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
    TOK_SEMI,
    TOK_IF,
    TOK_ELSE,
    TOK_TRUE,
    TOK_FALSE,
    TOK_LT,
    TOK_GT,
    TOK_EQEQ,
    TOK_LBRACE,
    TOK_RBRACE
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

static Token casehelper2(Lexer *l, TokenType type) {
    Token t;
    t.type = type;
    l->pos += 2;
    return t;
}

static char view_next(Lexer *l) {
    if (*l->pos == '\0') {
        return '\0';
    }
    return *(l->pos + 1);
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
        
        case '=': {
            char cmp = view_next(l);
            if(cmp == '=') {
                return casehelper2(l, TOK_EQEQ);
            } else {
                return casehelper(l, TOK_EQ);
            }
        }
        case ';':
            return casehelper(l, TOK_SEMI);
        case '<':
            return casehelper(l, TOK_LT);
        case '>':
            return casehelper(l, TOK_GT);
        case '{':
            return casehelper(l, TOK_LBRACE);
        case '}':
            return casehelper(l, TOK_RBRACE);

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
            if(len == 3 && strncmp(tmp, "let", 3) == 0) {
                Token t;
                t.type = TOK_LET;
                return t;
                }
            if(len == 2 && strncmp(tmp, "if", 2) == 0) {
                Token t;
                t.type = TOK_IF;
                return t;
            }
            if(len == 4 && strncmp(tmp, "else", 4) == 0) {
                Token t;
                t.type = TOK_ELSE;
                return t;
            }
            if(len == 4 && strncmp(tmp, "true", 4) == 0) {
                Token t;
                t.type = TOK_TRUE;
                return t;
            }
            if(len == 5 && strncmp(tmp, "false", 5) == 0) {
                Token t;
                t.type = TOK_FALSE;
                return t;
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
        case TOK_IF:     printf("IF\n"); break;
        case TOK_ELSE:   printf("ELSE\n"); break;
        case TOK_TRUE:   printf("TRUE\n"); break;
        case TOK_FALSE:  printf("FALSE\n"); break;
        case TOK_LT:     printf("LT\n"); break;
        case TOK_GT:     printf("GT\n"); break;
        case TOK_EQEQ:   printf("EQEQ\n"); break;
        case TOK_LBRACE: printf("LBRACE\n"); break;
        case TOK_RBRACE: printf("RBRACE\n"); break;
    }
}

/* ===== PARSER ===== */

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_LT,
    OP_GT,
    OP_EQEQ
} OpType;

typedef enum {
    NODE_INT,
    NODE_BINOP,
    NODE_IDENT,
    NODE_LET,
    NODE_BOOL,
    NODE_IF,
    NODE_BLOCK
} NodeType;

typedef struct Node {
    struct Node *next;
    NodeType type;
    union {
        int bool_val;
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
        struct {
            struct Node *cond;
            struct Node *then_branch;
            struct Node *else_branch;
        } if_stmt;
        struct Node *first;
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
Node *parse_comparison(Parser *p);
Node *parse_block(Parser *p);
Node *parse_if(Parser *p);

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
        new->next = NULL;
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
        new->next = NULL;
        left = new;
    }
    return left;
}

Node *parse_factor(Parser *p) {
    if(p->curr.type == TOK_MINUS) {
        advance(p);
        Node *local = parse_factor(p);
        Node *syn_zero = arena_alloc(p->arena, sizeof(Node));
        if(syn_zero == NULL) {
            fprintf(stderr, "Arena alloc error\n");
            exit(1);
        }
        syn_zero->type = NODE_INT;
        syn_zero->uni.int_value = 0;
        syn_zero->next = NULL;  
    
        Node *binop_wrapper = arena_alloc(p->arena, sizeof(Node));
        if(binop_wrapper == NULL) {
            fprintf(stderr, "Arena alloc error\n");
            exit(1);
        }
        binop_wrapper->type = NODE_BINOP;
        binop_wrapper->uni.binop.op = OP_SUB;
        binop_wrapper->uni.binop.left = syn_zero;
        binop_wrapper->uni.binop.right = local;
        binop_wrapper->next = NULL;
        return binop_wrapper;
    }

    if(p->curr.type == TOK_INT) {
        Node *new = arena_alloc(p->arena, sizeof(Node));
        if(new == NULL) {
            fprintf(stderr, "Arena alloc error\n");
            exit(1);
        }
        new->type = NODE_INT;
        new->uni.int_value = p->curr.value.int_val;
        new->next = NULL;
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
        tmp->next = NULL;
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
        new->next = NULL;
        advance(p);
        return new;
    } else {
        fprintf(stderr, "unexpected token\n");
        exit(1);
    }
    if(p->curr.type == TOK_TRUE) {
        Node *new = arena_alloc(p->arena, sizeof(Node));
        if(new == NULL) {
            fprintf(stderr, "Arena alloc error\n");
            exit(1);
        }
        new->type = NODE_BOOL;
        new->uni.bool_val = 1;
        new->next = NULL;
    }
    if(p->curr.type == TOK_FALSE) {
        Node *new = arena_alloc(p->arena, sizeof(Node));
        if(new == NULL) {
            fprintf(stderr, "Arena alloc error\n");
            exit(1);
        }
        new->type = NODE_BOOL;
        new->uni.bool_val = 0;
        new->next = NULL;
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
    Node *expr = parse_comparison(p);
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
    new->next = NULL;
    return new;
}

Node *parse_statement(Parser *p) {
    if(p->curr.type == TOK_IF) {
        return parse_if(p);
    }
    if(p->curr.type == TOK_LET) {
        return parse_let(p);
    }
    Node *new = parse_comparison(p);
    if(p->curr.type == TOK_SEMI) {
        advance(p);
        return new;
    } else {
        fprintf(stderr, "Expected ;\n");
        exit(1);
    }
}

Node *parse_comparison(Parser *p) {
    Node *tmp = parse_expr(p);
    if(p->curr.type == TOK_LT || p->curr.type == TOK_GT || p->curr.type == TOK_EQEQ) {
        Node *new = arena_alloc(p->arena, sizeof(Node));
        if(new == NULL) {
            fprintf(stderr, "Arena alloc error\n");
            exit(1);
        }
        new->type = NODE_BINOP;
        if(p->curr.type == TOK_LT) {
            new->uni.binop.op = OP_LT;
        }
        if(p->curr.type == TOK_GT) {
            new->uni.binop.op = OP_GT;
        }
        if(p->curr.type == TOK_EQEQ) {
            new->uni.binop.op = OP_EQEQ;
        }
        new->uni.binop.left = tmp;
        advance(p);
        new->uni.binop.right = parse_expr(p);
        new->next = NULL;
        return new;

    } else {
        return tmp;
    }
}

Node *parse_block(Parser *p) {
    if(p->curr.type != TOK_LBRACE) {
        fprintf(stderr, "Expected a {");
        exit(1);
    }
    advance(p);
    Node *head = NULL;
    Node *tail = NULL;
    while(p->curr.type != TOK_RBRACE) {
        if(p->curr.type == TOK_EOF) {
            fprintf(stderr, "EOF hit before }");
            exit(1);
        }
        Node *s = parse_statement(p);
        if(head == NULL) {
            head = s;
            tail = s;
        } else{
            tail->next = s;
            tail = s;
        }
    }
    advance(p);
    Node *new = arena_alloc(p->arena, sizeof(Node));
    if(new == NULL) {
        fprintf(stderr, "Arena alloc error\n");
        exit(1);
    }
    new->type = NODE_BLOCK;
    new->uni.first = head;
    new->next = NULL;
    return new;
}

Node *parse_if(Parser *p) {
    advance(p);
    if(p->curr.type != TOK_LPAREN) {
        fprintf(stderr, "Expected (");
        exit (1);
    } else {
        advance(p);
    }
    Node *cond = parse_comparison(p);
    if(p->curr.type != TOK_RPAREN) {
        fprintf(stderr, "Expected )");
        exit(1);
    } else{
        advance(p);
    }
    Node *then_branch = parse_block(p);
    Node *else_branch = NULL;
    if(p->curr.type == TOK_ELSE) {
        advance(p);
        else_branch = parse_block(p);
    }
    Node *new = arena_alloc(p->arena, sizeof(Node));
    if(new == NULL) {
        fprintf(stderr, "Arena alloc error\n");
        exit(1);
    }

    new->type = NODE_IF;
    new->uni.if_stmt.cond = cond;
    new->uni.if_stmt.then_branch = then_branch;
    new->uni.if_stmt.else_branch = else_branch;
    new->next = NULL;
    return new;
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
            case OP_LT:
                printf("<");
                break;
            case OP_GT:
                printf(">");
                break;
            case OP_EQEQ:
                printf("==");
                break;
        }
        print_ast(n->uni.binop.right);
    }
}

/* ===== EVALUATOR ===== */

int eval(Node *n, HashMap *env) {
    if (n->type == NODE_INT) {
        return n->uni.int_value;
    }
    if (n->type == NODE_BOOL) {
        return n->uni.bool_val;
    }
    if (n->type == NODE_IDENT) {
        int out;
        bool found = hm_get(env, n->uni.ident_name, &out);
        if (!found) {
            fprintf(stderr, "Undefined variable: %s\n", n->uni.ident_name);
            exit(1);
        }
        return out;
    }
    if (n->type == NODE_LET) {
        int val = eval(n->uni.let.value, env);
        hm_insert(env, n->uni.let.name, val);
        return 0;
    }
    if (n->type == NODE_IF) {
        int cond = eval(n->uni.if_stmt.cond, env);
        if (cond != 0) {
            eval(n->uni.if_stmt.then_branch, env);
        } else if (n->uni.if_stmt.else_branch != NULL) {
            eval(n->uni.if_stmt.else_branch, env);
        }
        return 0;
    }
    if (n->type == NODE_BLOCK) {
           Node *s = n->uni.first;
           int last = 0;
           while(s != NULL) {
             last = eval(s, env);
             s = s->next;
           }
           return last;
}
    if (n->type == NODE_BINOP) {
        int x = eval(n->uni.binop.left, env);
        int y = eval(n->uni.binop.right, env);
        switch (n->uni.binop.op) {
            case OP_ADD: return x + y;
            case OP_SUB: return x - y;
            case OP_MUL: return x * y;
            case OP_DIV: return x / y;
            case OP_LT: return x < y;
            case OP_GT: return x > y;
            case OP_EQEQ: return x == y;
        }
    }
    fprintf(stderr, "eval: unknown node type\n");
    exit(1);
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
            if(tmp->type != NODE_LET && tmp->type != NODE_IF && tmp->type != NODE_BLOCK  ) {
                printf("%d\n", val);

            }
        }
            arena_reset(a);
    }

    return 0;
}