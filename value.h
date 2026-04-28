#ifndef VALUE_H
#define VALUE_H


struct ParamList;
struct Node;
struct Environment;

typedef enum {
    VAL_INT,
    VAL_FUNCTION
} ValueType;

typedef struct {
    ValueType type;
    union {
        int int_val;
        struct {
            struct ParamList *params;
            struct Node *body;
            struct Environment *captured_env;
} function;
    } uni;
} Value;

#endif