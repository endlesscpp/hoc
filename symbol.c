#include <stdlib.h>
#include <string.h>

#include "hoc.h"
#include "y.tab.h"

static Symbol* symlist = 0;

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
    Symbol* sp = NULL;
    for (sp = symlist; sp != (Symbol*)0; sp = sp->next) {
        if (strcmp(sp->name, s) == 0) {
            return sp;
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
    sp->next  = symlist;
    symlist   = sp;
    return sp;
}
