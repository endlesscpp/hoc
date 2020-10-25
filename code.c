#include <math.h>
#include <stdio.h>
#include <string.h>

#include "hoc.h"
#include "y.tab.h"

#define NSTACK 256
static Datum  stack[NSTACK]; // the stack
static Datum* stackp;        // next free spot on stack

#define NPROG 2000
Inst  prog[NPROG]; // the machine
Inst* progp;       // next free spot for code generation
Inst* pc;          // program counter during execution

void initcode()
{
    stackp = stack;
    progp  = prog;
}

/**
 * push d onto stack
 */
void push(Datum d)
{
    if (stack >= &stack[NSTACK]) {
        execerror("stack overflow", NULL);
    }
    *stackp++ = d;
}

/**
 * pop and return top elem from stack
 */
Datum pop()
{
    if (stackp <= stack) {
        execerror("stack underflow", NULL);
    }
    return *--stackp;
}

/**
 * install one instruction or operand
 * TODO: how to output the generated instruction?
 */
Inst* code(Inst f)
{
    Inst* oprogp = progp;
    if (progp >= &prog[NPROG]) {
        execerror("program too big", NULL);
    }

    // [oak] debug
    const char* name = debugLookupFuncName(f);
    if (name != NULL) {
        printf("\t%s\n", name);
    } else if (f != NULL) {
        Symbol* p = (Symbol*)f;
        if (p->type == CONST) {
            printf("\t%s\n", p->name);
        } else {
            if (p->name == NULL || strlen(p->name) == 0) {
                printf("\t%.8g\n", p->u.val);
            } else {
                printf("\t%s:%.8g\n", p->name, p->u.val);
            }
        }
    } else {
        printf("\tSTOP\n");
    }

    *progp++ = f;
    return oprogp;
}

/**
 * run the machine
 */
void execute(Inst* p)
{
    for (pc = p; *pc != STOP;) {
        (*(*pc++))();
    }
}

/**
 * push constant onto stack
 */
void constpush()
{
    Datum d;
    d.val = ((Symbol*)*pc++)->u.val;
    push(d);
}

/**
 * push variable onto stack
 */
void varpush()
{
    Datum d;
    d.sym = ((Symbol*)*pc++);
    push(d);
}

void add()
{
    Datum d1, d2;
    d2 = pop();
    d1 = pop();
    d1.val += d2.val;
    push(d1);
}

void sub()
{
    Datum d1, d2;
    d2 = pop();
    d1 = pop();
    d1.val -= d2.val;
    push(d1);
}

void mul()
{
    Datum d1, d2;
    d2 = pop();
    d1 = pop();
    d1.val *= d2.val;
    push(d1);
}

void hocDiv()
{
    Datum d1, d2;
    d2 = pop();
    d1 = pop();
    d1.val /= d2.val;
    push(d1);
}

void power()
{
    Datum d1, d2;
    d2     = pop();
    d1     = pop();
    d1.val = pow(d1.val, d2.val);
    push(d1);
}

void negate()
{
    Datum d1;
    d1 = pop();
    d1.val = -d1.val;
    push(d1);
}

/**
 * evaluate variable on stack
 */
void eval()
{
    Datum d;
    d     = pop();
    if (d.sym->type == UNDEF) {
        execerror("undefined variable", d.sym->name);
    }
    d.val = d.sym->u.val;
    push(d);
}

/**
 * assign top value to next value
 */
void assign()
{
    Datum d1, d2;
    d1 = pop();
    d2 = pop();
    if (d1.sym->type != VAR && d1.sym->type != UNDEF) {
        execerror("assignment to non-variable", d1.sym->name);
    }
    d1.sym->u.val = d2.val;
    d1.sym->type  = VAR;
    push(d1);
}

/**
 * pop top value from stack, print it
 */
void print()
{
    Datum d;
    d = pop();
    printf("\t%.8g\n", d.val);
}

/**
 * evalute built-in on top of stack
 */
void bltin()
{
    Datum d;
    d     = pop();
    d.val = (*(double (*)(double))(*pc++))(d.val);
    push(d);
}

