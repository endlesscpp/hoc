%{
#include "hoc.h"
#include <stdio.h>
#define code2(c1, c2)       code(c1);code(c2);
#define code3(c1, c2, c3)   code(c1);code(c2);code(c3);
void execerror(char* s, char* t);
void yyerror(char* s);
%}
%union {
    Symbol* sym;    // symbol table pointer
    Inst*   inst;   // machine instruction
}
%token  <sym>   NUMBER VAR CONST BLTIN UNDEF
%right  '='
%left   '+' '-' // left associative, same precedence
%left   '*' '/'
%left   UNARYMINUS
%right  '^'     // exponentiation
%%
list:    // nothing
        | list '\n'
        | list asgn '\n'    { code2(pop, STOP); return 1;}
        | list expr '\n'    { code2(print, STOP); return 1;}
        | list error '\n'   { yyerrok; }
        ;
asgn:   VAR '=' expr    { code3(varpush, (Inst)$1, assign); }
        | CONST '=' expr    { execerror("cannot assign to const value", $1->name); }
        ;
expr:   NUMBER          { code2(constpush, (Inst)$1); }
        | VAR           { if ($1->type == UNDEF)
                              execerror("undefined variable1", $1->name);
                          code3(varpush, (Inst)$1, eval);
                        }
        | CONST         { code3(varpush, (Inst)$1, eval); }
        | asgn
        | BLTIN '(' expr ')' { code2(bltin, (Inst)$1->u.ptr); }
        | expr '+' expr   { code(add); }
        | expr '-' expr   { code(sub); }
        | expr '*' expr   { code(mul); }
        | expr '/' expr   { code(hocDiv); }
        | expr '^' expr   { code(power); }
        | '(' expr ')'
        | '-' expr %prec UNARYMINUS { code(negate); }   // unary minus
        ;
%%
        /* end of grammer */


#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>

char *progname; // for error messages
int lineno = 1;
jmp_buf begin;

void fprecatch(int signum);

int main(int argc, char** argv)
{
    progname = argv[0];
    init();
    setjmp(begin);
    signal(SIGFPE, fprecatch);
    for (initcode(); yyparse(); initcode())
        execute(prog);
    return 0;
}

int yylex()
{
    int c;

    while ((c = getchar()) == ' ' || c == '\t')
        ;

    if (c == EOF) {
        return 0;
    }
    
    if (c == '.' || isdigit(c)) {
        double d;
        ungetc(c, stdin);
        scanf("%lf", &d);
        yylval.sym = install("", NUMBER, d);
        return NUMBER;
    }

    if (isalpha(c)) {
        Symbol* s;
        char sbuf[100];
        char* p = sbuf;
        do {
            *p++ = c;
        } while ((c = getchar()) != EOF && isalnum(c));
        ungetc(c, stdin);
        *p = '\0';
        
        s = lookup(sbuf);
        if (s == NULL) {
            s = install(sbuf, UNDEF, 0.0);
        }
        yylval.sym = s;
        return s->type == UNDEF ? VAR : s->type;
    }

    if (c == '\n')
        lineno++;

    return c;
}

void warning(char* s, char* t)
{
    fprintf(stderr, "%s: %s", progname, s);

    if (t) {
        fprintf(stderr, " %s", t);
    }

    fprintf(stderr, " near line- %d\n", lineno);
}

void yyerror(char* s)
{
    warning(s, (char*)0);
}

void execerror(char* s, char* t)
{
    warning(s, t);
    longjmp(begin, 0);
}

void fprecatch(int signum) // catch floating point exceptions
{
    execerror("floating point exception", (char*)0);
}


