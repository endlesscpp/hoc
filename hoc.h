#ifndef _HOC_INCLUDE_H_
#define _HOC_INCLUDE_H_

#ifndef NULL
#    define NULL (0L)
#endif

typedef void (*Inst)(); // machine instruction
#define STOP (Inst)0

struct Symbol;

typedef struct Env {
    struct Symbol* symlist;
    struct Env* prev;
} Env;

/**
 * symbol table entry
 */
typedef struct Symbol {
    char* name;
    short type; // VAR, CONST, BLTIN, UNDEF
    union {
        double val;            // if VAR
        double (*ptr)(double); // if BLTIN
        Inst* defn;            // FUNCTION, PROCEDURE
        char* str;             // STRING
    } u;
    struct Symbol* next; // link to another Symbol
    struct Env*    env;  // only function / procedure has env
} Symbol;

/**
 * install s in symbol table
 * @param s - name
 * @param t - type
 * @param d - data
 */
Symbol* install(const char* s, int t, double d);

/**
 * find s in a symbol table
 * @param s - name
 */
Symbol* lookup(const char* s);

/**
 * push local env
 */
void pushLocalEnv(Symbol* sp);

/**
 * pop current env, restore to global env
 */
void popEnv();

/**
 * alloc buffer
 */
void* emalloc(unsigned int n);

/**
 * interpreter stack type
 */
typedef union Datum {
    double  val;
    Symbol* sym;
} Datum;

extern Datum pop();


extern Inst prog[];
extern Inst* progp;
extern Inst* progbase;
extern Inst* code();
extern void eval(), add(), sub(), mul(), hocDiv(), negate(), power();
extern void  assign(), bltin(), varpush(), constpush(), print(), varread();
extern void  printexpr(), printstr();
extern void gt(), lt(), eq(), ge(), le(), ne(), hocAnd(), hocOr(), hocNot();
extern void  ifcode(), whilecode(), call(), arg(), argassign();
extern void  funcret(), procret();

void        debugInitSymbolTable();
const char* debugLookupBuiltinFuncName(void* addr);
#endif
