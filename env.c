#include "env.h"
#include <stdlib.h>
#include <stdio.h>

Environment *env_create(Environment *parent) {
    Environment *new = malloc(sizeof(Environment));
    if(new == NULL) {
        fprintf(stderr, "ENVIRONMENT MALLOC ERROR\n");
        exit(1);
    }
    new->vars = hm_create();
    new->parent = parent;
    return new;
}

void env_destroy(Environment *env) {
    if(env == NULL) {
        return;
    }
    hm_destroy(env->vars);
    free(env);
}

bool env_get(Environment *env, const char *key, Value *out) {
    bool val = hm_get(env->vars, key, out);
    if(val == true) {
        return true;
    } else {
        if(env->parent != NULL) {
            return env_get(env->parent, key, out);
        }
        return false;
    }
}

void env_set(Environment *env, const char *key, Value value) {
    hm_insert(env->vars, key, value);
}