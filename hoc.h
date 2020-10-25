#ifndef _HOC_INCLUDE_H_
#define _HOC_INCLUDE_H_

#ifndef NULL
#    define NULL (0L)
#endif

typedef struct Symbol {
    char* name;
    short type; // VAR, BLTIN, UNDEF
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

#endif
