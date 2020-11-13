%{
#include "hoc.h"
#include <stdio.h>
#include <string.h>
#define code2(c1, c2)       code(c1);code(c2);
#define code3(c1, c2, c3)   code(c1);code(c2);code(c3);
void execerror(char* s, char* t);
void yyerror(char* s);
void defnonly(const char* s);
int indef;  // if is in function/procedure definition
%}
%union {
    Symbol* sym;    // symbol table pointer
    Inst*   inst;   // machine instruction
    int     narg;  // number of arguments
}
%token  <sym>   NUMBER STRING PRINT VAR CONST BLTIN UNDEF WHILE IF ELSE
%token  <sym>   FUNCTION PROCEDURE RETURN FUNC PROC READ
%token  <narg>  ARG
%type   <inst>  stmt asgn expr stmtlist prlist
%type   <inst>  cond while if begin end
%type   <sym>   procname
%type   <narg>  arglist
%right  '='
%left   OR
%left   AND
%left   GT GE LT LE EQ NE
%left   '+' '-' // left associative, same precedence
%left   '*' '/'
%left   UNARYMINUS NOT
%right  '^'     // exponentiation
%%
list:    // nothing
        | list '\n'
        | list defn '\n'
        | list asgn '\n'    { code2(pop, STOP); return 1;}
        | list stmt '\n'    { code(STOP); return 1;}
        | list expr '\n'    { code2(print, STOP); return 1;}
        | list error '\n'   { yyerrok; }
        ;
asgn:   VAR '=' expr    { $$=$3; code3(varpush, (Inst)$1, assign); }
        | CONST '=' expr    { execerror("cannot assign to const value", $1->name); }
        | ARG '=' expr      { defnonly("$"); code2(argassign,(Inst)$1); $$=$3; }
        ;
stmt:   expr            { code(pop); }
        | RETURN        { defnonly("return"); code(procret); }
        | RETURN expr   { defnonly("return"); $$=$2; code(funcret); }
        | PROCEDURE begin '(' arglist ')'
                        { $$=$2; code3(call, (Inst)$1, (Inst)$4); }
        | PRINT prlist  { $$=$2; }
        | while cond stmt end {
                ($1)[1] = (Inst)$3; // body of loop
                ($1)[2] = (Inst)$4; // end, if cond fails
            }
        | if cond stmt end { // else-less if
                ($1)[1] = (Inst)$3; // then part
                ($1)[3] = (Inst)$4; // end, if cond fails
            }
        | if cond stmt end ELSE stmt end { // if with else
                ($1)[1] = (Inst)$3; // then part
                ($1)[2] = (Inst)$6; // else part
                ($1)[3] = (Inst)$7; // end, if cond fails
            }
        | '{' stmtlist '}'  { $$ = $2; }
        ;
cond:   '(' expr ')'    { code(STOP); $$ = $2; }
        ;
while:  WHILE           { $$=code3(whilecode, STOP, STOP); }
        ;
if:     IF              { $$=code(ifcode); code3(STOP, STOP, STOP); }
        ;
end:     /*nothing*/    { code(STOP); $$=progp; }
        ;
stmtlist: /*nothing*/   { $$=progp; }
        | stmtlist '\n'
        | stmtlist stmt
        ;
expr:   NUMBER          { $$ = code2(constpush, (Inst)$1); }
        | VAR           { if ($1->type == UNDEF)
                              execerror("undefined variable1", $1->name);
                          $$ = code3(varpush, (Inst)$1, eval);
                        }
        | CONST         { $$ = code3(varpush, (Inst)$1, eval); }
        | ARG           { defnonly("$"); $$ = code2(arg, (Inst)$1); }
        | asgn
        | FUNCTION begin '(' arglist ')'
                        { $$ = $2; code3(call, (Inst)$1, (Inst)$4); }
        | READ '(' VAR ')'   { $$ = code2(varread, (Inst)$3); }
        | BLTIN '(' expr ')' { $$ = $3; code2(bltin, (Inst)$1->u.ptr); }
        | '(' expr ')'       { $$ = $2; }
        | expr '+' expr   { code(add); }
        | expr '-' expr   { code(sub); }
        | expr '*' expr   { code(mul); }
        | expr '/' expr   { code(hocDiv); }
        | expr '^' expr   { code(power); }
        | '-' expr %prec UNARYMINUS { $$ = $2; code(negate); }   // unary minus
        | expr GT expr    { code(gt); }
        | expr GE expr    { code(ge); }
        | expr LT expr    { code(lt); }
        | expr LE expr    { code(le); }
        | expr EQ expr    { code(eq); }
        | expr NE expr    { code(ne); }
        | expr AND expr   { code(hocAnd); }
        | expr OR expr    { code(hocOr); }
        | NOT expr        { $$ = $2; code(hocNot); }
        ;
begin:   /*nothing*/      { $$ = progp; }
        ;

prlist: expr              { code(printexpr); }
        | STRING          { $$ = code2(printstr, (Inst)$1); }
        | prlist ',' expr { code(printexpr); }
        | prlist ',' STRING { code2(printstr, (Inst)$3); }
        ;
// [oak] why also insert code(procret), and both proc and func insert the same code(procret)?
// [oak] to handle the case that user does NOT 'return' in func?
defn:     FUNC procname   { $2->type=FUNCTION; indef=1; }
          '(' ')' stmt    { code(procret); define($2); indef=0; }
        | PROC procname   { $2->type=PROCEDURE; indef=1; }
          '(' ')' stmt    { code(procret); define($2); indef=0; }
        ;
procname: VAR
        | FUNCTION
        | PROCEDURE
        ;
arglist:  /*nothing*/     { $$ = 0; }
        | expr            { $$ = 1; }
        | arglist ',' expr  { $$ = $1 + 1; }
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
static int s_argc; // argument number (except the program itself)
static char** s_argv;
static char* s_infile;  // input file name
FILE* fin;   // input file pointer
char c;             // global, for use in warning()

void fprecatch(int signum);

/**
 * return 1 if has more, otherwise 0.
 */
int moreinput()
{
    if (s_argc-- <= 0) {
        return 0;
    }
    
    if (fin && fin != stdin) {
        fclose(fin);
        fin = NULL;
    }

    s_infile = *s_argv++;
    lineno = 1;

    if (strcmp(s_infile, "-") == 0) {
        fin = stdin;
        s_infile = NULL;
    } else if ((fin = fopen(s_infile, "r")) == NULL) {
        fprintf(stderr, "%s: can't open %s\n", progname, s_infile);
        return moreinput();
    }

    return 1;
}

/**
 * execute until EOF
 * TODO: move this function to code.c?
 */
static void execute_until_eof()
{
    setjmp(begin);
    signal(SIGFPE, fprecatch);
    for (initcode(); yyparse(); initcode()) {
        // BAI: attention, here use progbase rather than prog in hoc6.
        execute(progbase);
    }
}

static int follow(char expect, int ifyes, int ifno)
{
    int c = getc(fin);
    if (c == expect) {
        return ifyes;
    }
    ungetc(c, fin);
    return ifno;
}

static int look_ahead()
{
    int c = getc(fin);
    ungetc(c, fin);
    return c;
}

int main(int argc, char** argv)
{
    progname = argv[0];
    if (argc == 1) {
        static char* stdinonly[] = {"-"};
        s_argv = stdinonly;
        s_argc = 1;
    } else {
        s_argc = argc - 1;
        s_argv = argv + 1;
    }
    init();
    while (moreinput()) {
        execute_until_eof();
    }
    return 0;
}

static int mygetc(FILE* stream) {
    int c = getc(stream);
    printf("##%c\n", c);
    return c;
}

int yylex()
{
    while ((c = mygetc(fin)) == ' ' || c == '\t')
        ;

    if (c == EOF) {
        return 0;
    }

    // handle '//'
    if (c == '/') {
        if (look_ahead() == '/') {
            while ((c = mygetc(fin)) != '\n' && c != EOF) {
                ;
            }
            if (c == '\n') {
                lineno++;
            }
            return c;
        }
    }
    
    if (c == '.' || isdigit(c)) {
        double d;
        ungetc(c, fin);
        fscanf(fin, "%lf", &d);
        yylval.sym = install("", NUMBER, d);
        return NUMBER;
    }

    if (isalpha(c)) {
        Symbol* s;
        char sbuf[100];
        char* p = sbuf;
        do {
            *p++ = c;
        } while ((c = mygetc(fin)) != EOF && isalnum(c));
        ungetc(c, fin);
        *p = '\0';
        
        s = lookup(sbuf);
        if (s == NULL) {
            s = install(sbuf, UNDEF, 0.0);
        }
        yylval.sym = s;
        return s->type == UNDEF ? VAR : s->type;
    }

    if (c == '$') {
        // argument?
        int n = 0;
        while (isdigit(c=mygetc(fin))) {
            n = 10 * n + c - '0';
        }
        ungetc(c, fin);
        if (n == 0) {
            execerror("strange $...", NULL);
        }
        yylval.narg = n;
        return ARG;
    }

    if (c == '"') {
        // quoted string
        char sbuf[100];
        char* p;
        for (p = sbuf; (c=mygetc(fin)) != '"'; p++) {
            if (c == '\n' || c == EOF) {
                execerror("missing quote", "");
            }
            if (p >= sbuf + sizeof(sbuf) - 1) {
                *p = '\0';
                execerror("string too long", sbuf);
            }
            *p = backslash(c);
        }
        *p = '\0';
        yylval.sym = (Symbol*)emalloc(strlen(sbuf)+1);
        strcpy((char*)yylval.sym, sbuf);
        return STRING;
    }

    switch (c) {
        case '>':   return follow('=', GE, GT);
        case '<':   return follow('=', LE, LT);
        case '=':   return follow('=', EQ, '=');
        case '!':   return follow('=', NE, NOT);
        case '|':   return follow('|', OR, '|');
        case '&':   return follow('&', AND, '&');
        case '\n':  lineno++; return '\n';
        default:    return c;
    }
}

/**
 * get next char with \ interpreted
 */
int backslash(int c)
{
    static char transTable[] = "b\bf\fn\nr\rt\t";
    if (c != '\\') {
        return c;
    }
    c = mygetc(fin);
    if (islower(c) && strchr(transTable, c) != NULL) {
        return strchr(transTable, c)[1];
    }
    return c;
}

void warning(char* s, char* t)
{
    fprintf(stderr, "%s: %s", progname, s);

    if (t) {
        fprintf(stderr, " %s", t);
    }

    if (s_infile) {
        fprintf(stderr, " in %s", s_infile);
    }

    fprintf(stderr, " near line: %d\n", lineno);

    while (c != '\n' && c != EOF) {
        c = getc(fin);
    }

    if (c == '\n') {
        lineno++;
    }
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

/**
 * warn if illegal definition
 */
void defnonly(const char* s)
{
    if (!indef) {
        execerror("s", "used outside definition");
    }
}
