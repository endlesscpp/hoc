#include <math.h>

#include "hoc.h"
#include "y.tab.h"

extern double Log(double), Log10(double), Exp(double), Sqrt(double),
    Integer(double);

static struct {
    const char* name;
    double      cval;
} consts[]
    = {{"PI", 3.14159265358979323846},    {"E", 2.71828182845904523536},
       {"GAMMA", 0.57721566490153286060}, {"DEG", 57.29577951308232087680},
       {"PHI", 1.61803398874989484820},   {NULL, 0}};

static struct {
    const char* name;
    double (*func)(double);
} builtins[]
    = {{"sin", sin},     {"cos", cos}, {"atan", atan}, {"log", Log},
       {"log10", Log10}, {"exp", Exp}, {"sqrt", Sqrt}, {"int", Integer},
       {"abs", fabs},    {NULL, 0}};

/**
 * install constants and built-ins in table
 */
void init()
{
    int i;
    for (i = 0; consts[i].name != NULL; i++) {
        install(consts[i].name, VAR, consts[i].cval);
    }

    for (i = 0; builtins[i].name != NULL; i++) {
        Symbol* s = install(builtins[i].name, BLTIN, 0.0);
        s->u.ptr  = builtins[i].func;
    }
}
