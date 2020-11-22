#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hoc.h"
#include "y.tab.h"

static Env  globalEnv = {0};
static Env* currEnv   = &globalEnv;

void* emalloc(unsigned int n)
{
    void* p = malloc(n);
    if (p == NULL) {
        execerror("out of memory", (char*)0);
    }
    return p;
}

/**
 * find s in symbol table
 */
Symbol* lookup(const char* s)
{
    if (strcmp(s, "a") == 0) {
        printf("env = %p, env->next = %p, globalEnv = %p\n", currEnv,
               currEnv->prev, &globalEnv);
    }
    Symbol* sp = NULL;
    Env*    env;
    for (env = currEnv; env != NULL; env = env->prev) {
        for (sp = env->symlist; sp != (Symbol*)0; sp = sp->next) {
            if (strcmp(sp->name, s) == 0) {
                return sp;
            }
        }
    }
    return NULL;
}

/*
 * install constants and built-ins in a symbol table
 */
Symbol* install(const char* s, int t, double d)
{
    Symbol* sp;
    sp       = (Symbol*)emalloc(sizeof(Symbol));
    sp->name = (char*)emalloc(strlen(s) + 1);
    strcpy(sp->name, s);
    sp->type  = t;
    sp->u.val = d;
    sp->next         = currEnv->symlist;
    sp->env          = NULL;
    currEnv->symlist = sp;
    return sp;
}

void pushLocalEnv(Symbol* sp)
{
    printf("push local env...\n");
    if (sp->type != FUNCTION && sp->type != PROCEDURE) {
        printf("invalid symbol type when create local env, type = %d\n",
               sp->type);
        return;
    }

    sp->env          = (Env*)malloc(sizeof(Env));
    sp->env->symlist = NULL;
    sp->env->prev    = &globalEnv;
    currEnv          = sp->env;
}

void popEnv()
{
    printf("pop env...\n");
    currEnv = &globalEnv;
}

