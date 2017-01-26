#ifndef COMMON_H
#define COMMON_H

#define NELEMS(arr) (sizeof(arr)/sizeof(arr[0]))

typedef struct literal * LITERAL;

/*
 * We look at the 3 left bits of type.
 * This gives SEXPR_CONS, SEXPR_LITERAL, etc.
 * For:
 * SEXPR_CONS: bits(28..0) is index into cells.
 * SEXPR_BUILTIN_FUNCTION: bits(28..0) is index into table of builtin
 *                         functions.
 * SEXPR_BUILTIN_SPECIAL: bits(28..0) is index into table of builtin
 *                             special forms.
 * SEXPR_FUNCTION and
 * SEXPR_SPECIAL and
 * SEXPR_CLOSURE: bits(28..0) is index into cells.
 * SEXPR_NUMBER: bits(28..0) is index to a cell whose car is the floating
 *               number (the cdr we don't care).
 * SEXPR_LITERAL: bits(28..0) is index to a cell whose car is the pointer
 *                to the struct literal (the cdr we don't care).
 */
union sexpr {
	int type;
	float number;
	const char *name;
};

typedef union sexpr SEXPR;

enum {
	SEXPR_CONS = 0 << 29,
	SEXPR_LITERAL = 1 << 29,
	SEXPR_NUMBER = 2 << 29,
	SEXPR_BUILTIN_FUNCTION = 3 << 29,
	SEXPR_BUILTIN_SPECIAL = 4 << 29,
	SEXPR_FUNCTION = 5 << 29,
	SEXPR_SPECIAL = 6 << 29,
	SEXPR_CLOSURE = 7 << 29,
};

/* sexpr.c */

SEXPR make_cons(int i);
SEXPR make_literal(const char *s, int len);
SEXPR make_number(float n);
SEXPR make_builtin_function(int i);
SEXPR make_builtin_special(int i);
SEXPR make_function(int args_n_body);
SEXPR make_special(int args_n_body);
SEXPR make_closure(int lambda_n_alist);

int sexpr_type(SEXPR e);
int sexpr_index(SEXPR e);
LITERAL sexpr_literal(SEXPR e);
float sexpr_number(SEXPR e);

#endif
