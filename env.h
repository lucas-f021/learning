#ifndef ENV_H
#define ENV_H

#include "hash.h"

typedef struct Environment {
    HashMap *vars;
    struct Environment *parent;
} Environment;

Environment *env_create(Environment *parent);
void env_destroy(Environment *env);
bool env_get(Environment *env, const char *key, Value *out);
void env_set(Environment *env, const char *key, Value value);

#endif