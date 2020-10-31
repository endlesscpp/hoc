#ifndef _HOC_INCLUDE_H_
#define _HOC_INCLUDE_H_

#ifndef NULL
#    define NULL (0L)
#endif

/**
 * symbol table entry
 */
typedef struct Symbol {
    char* name;
    short type; // VAR, CONST, BLTIN, UNDEF
    union {
        double val;            // if VAR
        double (*ptr)(double); // if BLTIN
    } u;
    struct Symbol* next; // link to another Symbol
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

typedef void (*Inst)(); // machine instruction
#define STOP (Inst)0

extern Inst prog[];
extern Inst* progp;
extern Inst* code();
extern void eval(), add(), sub(), mul(), hocDiv(), negate(), power();
extern void assign(), bltin(), varpush(), constpush(), print();
extern void printexpr();
extern void gt(), lt(), eq(), ge(), le(), ne(), hocAnd(), hocOr(), hocNot();
extern void ifcode(), whilecode();

void        debugInitSymbolTable();
const char* debugLookupFuncName(void* addr);
#endif
