%{
#include "hoc.h"
#include <stdio.h>
extern double Pow(double, double);
void execerror(char* s, char* t);
void yyerror(char* s);
%}
%union {
    double val;     // actual value
    Symbol* sym;    // symbol table pointer
}
%token  <val>   NUMBER
%token  <sym>   VAR CONST BLTIN UNDEF
%type   <val>   expr asgn   // expression, assign
%right  '='
%left   '+' '-' // left associative, same precedence
%left   '*' '/'
%left   UNARYMINUS
%right  '^'     // exponentiation
%%
list:    // nothing
        | list '\n'
        | list asgn '\n'
        | list expr '\n'    { printf("\t%.8g\n", $2); }
        | list error '\n'   { yyerrok; }
        ;
asgn:   VAR '=' expr    { $$ = $1->u.val=$3; $1->type=VAR; }
        | CONST '=' expr    { execerror("cannot assign to const value", $1->name); }
        ;
expr:   NUMBER          { $$ = $1; }
        | VAR           { if ($1->type == UNDEF)
                              execerror("undefined variable", $1->name);
                          $$ = $1->u.val;
                        }
        | CONST         { $$ = $1->u.val; }
        | asgn
        | BLTIN '(' expr ')' { $$ = (*($1->u.ptr))($3); };
        | expr '+' expr   { $$ = $1 + $3; }
        | expr '-' expr   { $$ = $1 - $3; }
        | expr '*' expr   { $$ = $1 * $3; }
        | expr '/' expr   {
                if ($3 == 0.0)
                    execerror("division by zero", "");
                $$ = $1 / $3; 
                }
        | expr '^' expr     { $$ = Pow($1, $3); }
        | '(' expr ')'      { $$ = $2; }
        | '-' expr %prec UNARYMINUS { $$ = -$2; }   // unary minus
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

void main(int argc, char** argv)
{
    progname = argv[0];
    init();
    setjmp(begin);
    signal(SIGFPE, fprecatch);
    yyparse();
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
        ungetc(c, stdin);
        scanf("%lf", &yylval.val);
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


