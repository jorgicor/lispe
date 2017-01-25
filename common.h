#ifndef COMMON_H
#define COMMON_H

#define NELEMS(arr) (sizeof(arr)/sizeof(arr[0]))

typedef struct literal * LITERAL;

/*
 * .index is 0xxxx... : xxx... cons cell index.
 *           1xxxx... : xxx... index of cell with atom inside.
 * cells containing atoms:
 * car 0 : cdr .literal pointer to literal
 * car 1 : cdr .float number
 * car 2 : cdr .index of builtin function
 * car 3 : cdr .index of builtin special form
 * car 4 : cdr .index of user function
 * car 5 : cdr .index of user special form
 * car 4 : cdr .index of user closure
 */
union sexpr {
	int index;
	int type;
	float number;
	LITERAL literal;
};

typedef union sexpr SEXPR;

enum {
	SEXPR_CONS,
	SEXPR_LITERAL,
	SEXPR_NUMBER,
	SEXPR_BUILTIN_FUNCTION,
	SEXPR_BUILTIN_SPECIAL,
	SEXPR_FUNCTION,
	SEXPR_SPECIAL,
	SEXPR_CLOSURE,
};

/* sexpr.c */

SEXPR make_cons(int i);
SEXPR make_literal(LITERAL lit);
SEXPR make_number(float n);
SEXPR make_builtin_function(int i);
SEXPR make_builtin_special(int i);
SEXPR make_function(SEXPR args_n_body);
SEXPR make_special(SEXPR args_n_body);
SEXPR make_closure(SEXPR lambda_n_alist);

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
