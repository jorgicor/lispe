#ifndef COMMON_H
#define COMMON_H

#define NELEMS(arr) (sizeof(arr)/sizeof(arr[0]))

#define USE_TREE 0

typedef struct literal * LITERAL;

/* literal will be 4 bytes on 32 bit systems, and 8 on 64 bit systems! */
struct sexpr {
	signed char type;
	union {
		LITERAL literal;
		float number;
		int index;
	} data;
};

typedef struct sexpr SEXPR;

enum {
	SEXPR_CONS,
	SEXPR_LITERAL,
	SEXPR_NUMBER,
	SEXPR_BUILTIN_FUNCTION,
	SEXPR_BUILTIN_SPECIAL,
	SEXPR_FUNCTION,
	SEXPR_SPECIAL,
};

/* sexpr.c */

SEXPR make_cons(int i);
SEXPR make_literal(LITERAL lit);
SEXPR make_number(float n);
SEXPR make_builtin_function(int i);
SEXPR make_builtin_special(int i);
SEXPR make_function(int args_n_body);
SEXPR make_special(int args_n_body);

int sexpr_type(SEXPR e);
int sexpr_index(SEXPR e);
LITERAL sexpr_literal(SEXPR e);
float sexpr_number(SEXPR e);

/* symtab */

LITERAL new_literal(const char *name, int len);
const char *literal_name(LITERAL lit);
int literals_equal(LITERAL lita, LITERAL litb);
void gc_literals(void);
void gc_mark_literal_used(LITERAL lit);

#endif
