#include <math.h>
#include <stdio.h>
#include <string.h>

#include "hoc.h"
#include "y.tab.h"

extern FILE* fin; // define in hoc.y

#define NSTACK 256
static Datum  stack[NSTACK]; // the stack
static Datum* stackp;        // next free spot on stack

#define NPROG 2000
Inst  prog[NPROG];     // the machine
Inst* progp;           // next free spot for code generation
Inst* pc;              // program counter during execution
Inst* progbase = prog; // start of current subprogram.
                       // 1. function entry(defn) is set to progbase when define
                       // it 2.after define function, progbase is set to progp
                       // 3. when execute_until_eof, progp is set to progbase in
                       // initcode().
int   returning;       // 1 if return stmt seen

/**
 * proc/func call stack frame
 */
typedef struct Frame {
    Symbol* sp;    // symbol table entry
    Inst*   retpc; // where to resume after return
    Datum*  argn;  // n-th argument on stack
    int     nargs; // number of arguments
} Frame;
#define NFRAME 100
Frame  frame[NFRAME];
Frame* fp; // frame pointer

#define EPSILON 0.00000001

void initcode()
{
    progp     = progbase;
    stackp = stack;
    fp        = frame;
    returning = 0;
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
    // TODO: f maybe arg and string
    Inst        prev2    = (progp >= prog - 2) ? *(progp - 2) : NULL;
    Inst        prevInst = (progp > prog) ? *(progp - 1) : NULL;
    const char* name = debugLookupBuiltinFuncName(f);
    if (name != NULL) {
        printf("\t%p\t%s\n", progp, name);
    } else if (f == NULL) {
        printf("\t%p\tSTOP\n", progp);
    } else if (prevInst == argassign || prevInst == arg) {
        printf("\t%p\t$%d[arg]", progp, (int)f);
    } else if (prevInst == printstr) {
        printf("\t%p\t%s[string]", progp, (char*)f);
    } else if (prev2 == call) {
        // [call, func, argcount]
        printf("\t%p\t%d[argcount]\n", progp, (int)f);
    } else {
        Symbol* p = (Symbol*)f;
        if (p->type == CONST) {
            printf("\t%p\t%s\n", progp, p->name);
        } else if (p->type == PROCEDURE) {
            printf("\t%p\t%s[procedure]\n", progp, p->name);
        } else if (p->type == FUNCTION) {
            // [oak] ATTENTION: the Symbol is saved in instrument list.
            // we explicitly call execute(defn) in call()
            printf("\t%p\t%s[function]\n", progp, p->name);
        } else {
            if (p->name == NULL || strlen(p->name) == 0) {
                printf("\t%p\t%.8g[type:%d]\n", progp, p->u.val, p->type);
            } else {
                printf("\t%p\t%s:%.8g[type:%d]\n", progp, p->name, p->u.val,
                       p->type);
            }
        }
    }

    *progp++ = f;
    return oprogp;
}

/**
 * run the machine
 */
void execute(Inst* p)
{
    for (pc = p; *pc != STOP && !returning;) {
        printf("-> pc = %p\n", pc);
        (*(*pc++))();
    }
}

/**
 * define func/proc in symbol table
 */
void define(Symbol* sp)
{
    // TODO: debug the progbase and progp at here.
    printf("define(), progbase = %p, progp = %p\n", progbase, progp);
    sp->u.defn = progbase;
    progbase   = progp;
}

/**
 * call a function
 */
void call()
{
    Symbol* sp = (Symbol*)pc[0];
    if (fp++ >= &frame[NFRAME - 1]) {
        execerror(sp->name, "call nested too deeply");
    }
    // bai: like the C function call(put retpc, args onto stack).
    // but the stackp is increased in HOC.
    fp->sp    = sp;
    fp->nargs = (int)pc[1];
    fp->retpc = pc + 2;
    fp->argn  = stackp - 1; // last argument
    printf("call, nargs = %d, retpc = %p\n", fp->nargs, fp->retpc);
    execute(sp->u.defn);
    returning = 0;
}

/**
 * common return from func or proc
 */
void ret()
{
    int i;
    for (i = 0; i < fp->nargs; i++) {
        pop();
    }
    pc = (Inst*)fp->retpc;
    printf("after return, pc = %p\n", pc);
    --fp;
    returning = 1;
}

/**
 * function ret. (BAI: function must return value, procedure NEVER return
 * value).
 */
void funcret()
{
    Datum d;
    if (fp->sp->type == PROCEDURE) {
        execerror(fp->sp->name, "(proc) returns value");
    }
    d = pop(); // preserve function return value
    ret();
    push(d);
}

/**
 * procedure return
 */
void procret()
{
    if (fp->sp->type == FUNCTION) {
        execerror(fp->sp->name, "(func) returns no value");
    }
    ret();
}

/**
 * return pointer to argument i (i start from 1 ($1, $2, $3...))
 */
double* getarg()
{
    int i = (int)*pc++;
    if (i > fp->nargs) {
        execerror(fp->sp->name, "not enough arguments");
    }
    // BAI: fp->argn point to the last arguments
    return &fp->argn[i - fp->nargs].val;
}

/**
 * push argument onto stack
 */
void arg()
{
    Datum d;
    d.val = *getarg();
    push(d);
}

/**
 * store item on top of stack to argument
 */
void argassign()
{
    Datum d;
    d = pop();
    push(d); // leave value on stack
    *getarg() = d.val;
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
    printf("*****in assign...\n");
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
 * print numeric value
 */
void printexpr()
{
    Datum d;
    d = pop();
    printf("%.8g ", d.val);
}

/**
 * print string value
 */
void printstr()
{
    printf("%s", (char*)*pc++);
}

/**
 * read into variable
 */
void varread()
{
    Datum   d;
    Symbol* var = (Symbol*)*pc++;
Again:
    switch (fscanf(fin, "%lf", &var->u.val)) {
    case EOF:
        if (moreinput()) {
            goto Again;
        }
        d.val = var->u.val = 0.0;
        break;
    case 0:
        execerror("non-number read into", var->name);
        break;
    default:
        d.val = 1.0;
        break;
    }

    var->type = VAR;
    push(d);
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

void lt()
{
    Datum d1, d2;
    d2     = pop();
    d1     = pop();
    d1.val = (double)(d1.val < d2.val);
    push(d1);
}

void gt()
{
    Datum d1, d2;
    d2     = pop();
    d1     = pop();
    d1.val = (double)(d1.val > d2.val);
    push(d1);
}

void eq()
{
    Datum d1, d2;
    d2     = pop();
    d1     = pop();
    d1.val = (double)(d1.val == d2.val);
    push(d1);
}

void le()
{
    Datum d1, d2;
    d2     = pop();
    d1     = pop();
    d1.val = (double)(d1.val <= d2.val);
    push(d1);
}

void ge()
{
    Datum d1, d2;
    d2     = pop();
    d1     = pop();
    d1.val = (double)(d1.val >= d2.val);
    push(d1);
}

void ne()
{
    Datum d1, d2;
    d2     = pop();
    d1     = pop();
    d1.val = (double)(d1.val != d2.val);
    push(d1);
}

void hocAnd()
{
    Datum d1, d2;
    d2     = pop();
    d1     = pop();
    d1.val = (double)(d1.val > EPSILON && d2.val > EPSILON);
    push(d1);
}

void hocOr()
{
    Datum d1, d2;
    d2     = pop();
    d1     = pop();
    d1.val = (double)(d1.val > EPSILON || d2.val > EPSILON);
    push(d1);
}

void hocNot()
{
    Datum d1;
    d1     = pop();
    d1.val = (double)(d1.val <= EPSILON);
    push(d1);
}


/**
 * stack layout:
 * - whilecode
 * - loopbody
 * - next statement
 * - condition
 */
void whilecode()
{
    Datum d;
    Inst* savepc = pc; // loop body

    execute(savepc + 2); // condition
    d = pop();
    while (d.val > EPSILON) {
        execute(*((Inst**)savepc)); // body
        if (returning) {
            break;
        }
        execute(savepc + 2);        // condition
        d = pop();
    }

    if (!returning) {
        pc = *(Inst**)(savepc + 1); // next statment
    }
}

/**
 * stack layout:
 * ifcode
 * then part addr <- pc
 * else part addr
 * next statment addr
 * condition
 */
void ifcode()
{
    Datum d;
    Inst* savepc = pc; // then

    execute(savepc + 3); // condition
    d = pop();
    if (d.val > EPSILON) {
        execute(*(Inst**)savepc);
    } else if (*(Inst**)(savepc + 1) != NULL) {
        execute(*(Inst**)(savepc + 1)); // else
    }

    if (!returning) {
        pc = *(Inst**)(savepc + 2); // next
    }
}
