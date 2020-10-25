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

typedef struct DebugSymbol {
    const char*         name;
    void*               addr;
    struct DebugSymbol* next;
} DebugSymbol;

static DebugSymbol* sDebugSymList = NULL;

static void debugInstallSymbol(const char* name, void* addr)
{
    DebugSymbol* p = (DebugSymbol*)emalloc(sizeof(DebugSymbol));
    p->name        = name;
    p->addr        = addr;
    p->next        = sDebugSymList;
    sDebugSymList  = p;
}

void debugInitSymbolTable()
{
    debugInstallSymbol("eval", eval);
    debugInstallSymbol("add", add);
    debugInstallSymbol("sub", sub);
    debugInstallSymbol("mul", mul);
    debugInstallSymbol("div", hocDiv);
    debugInstallSymbol("power", power);
    debugInstallSymbol("negate", negate);
    debugInstallSymbol("assign", assign);
    debugInstallSymbol("execBltin", bltin);
    debugInstallSymbol("varpush", varpush);
    debugInstallSymbol("constpush", constpush);
    debugInstallSymbol("pop", pop);
    debugInstallSymbol("popPrint", print);

    int i;
    for (i = 0; builtins[i].name != NULL; i++) {
        debugInstallSymbol(builtins[i].name, builtins[i].func);
    }
}

const char* debugLookupFuncName(void* addr)
{
    DebugSymbol* p = sDebugSymList;
    while (p != NULL) {
        if (p->addr == addr) {
            return p->name;
        }
        p = p->next;
    }
    return NULL;
}

/**
 * install constants and built-ins in table
 */
void init()
{
    int i;
    for (i = 0; consts[i].name != NULL; i++) {
        install(consts[i].name, CONST, consts[i].cval);
    }

    for (i = 0; builtins[i].name != NULL; i++) {
        Symbol* s = install(builtins[i].name, BLTIN, 0.0);
        s->u.ptr  = builtins[i].func;
    }

    debugInitSymbolTable();
}
