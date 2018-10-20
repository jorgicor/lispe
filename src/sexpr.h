/* ===========================================================================
 * lispe, Scheme interpreter.
 * ===========================================================================
 */

#ifndef SEXPR_H
#define SEXPR_H

/*
 * We look at the 4 left bits for type.
 * This gives SEXPR_CONS, SEXPR_SYMBOL, etc.
 * For:
 * SEXPR_NIL: 0.
 * SEXPR_CONS: bits(28..0) is index into cells.
 * SEXPR_BUILTIN_FUNCTION: bits(28..0) is index into table of builtin
 *                         functions.
 * SEXPR_BUILTIN_SPECIAL: bits(28..0) is index into table of builtin
 *                             special forms.
 * SEXPR_FUNCTION and
 * SEXPR_SPECIAL and
 * SEXPR_CLOSURE: bits(28..0) is index into cells.
 * SEXPR_NUMBER: bits(28..0) is index of number.
 * SEXPR_SYMBOL: bits(28..0) is index to a cell whose car is the pointer
 *                to the struct literal (the cdr we don't care).
 *
 * All the code uses SEXPRs through the functions and macros here listed.
 * They don't mess with the bits directly.
 * TODO: The exception is SEXPR_NIL, but it would be better to change it to
 * something like make_nil().
 */

enum { SHIFT_SEXPR = 28 };

enum {
	TYPE_MASK_SEXPR = ((unsigned int) -1) << SHIFT_SEXPR,
	INDEX_MASK_SEXPR = ~TYPE_MASK_SEXPR,
};

typedef int SEXPR;

enum {
	SEXPR_NIL = 0,
	SEXPR_TRUE = 1 << SHIFT_SEXPR,
	SEXPR_FALSE = 2 << SHIFT_SEXPR,
	SEXPR_CONS = 3 << SHIFT_SEXPR,
	SEXPR_SYMBOL = 4 << SHIFT_SEXPR,
	SEXPR_NUMBER = 5 << SHIFT_SEXPR,
	SEXPR_BUILTIN_FUNCTION = 6 << SHIFT_SEXPR,
	SEXPR_BUILTIN_SPECIAL = 7 << SHIFT_SEXPR,
	SEXPR_FUNCTION = 8 << SHIFT_SEXPR,
	SEXPR_SPECIAL = 9 << SHIFT_SEXPR,
	SEXPR_DYN_FUNCTION = 10 << SHIFT_SEXPR,
};

#define sexpr_type(e) ((e) & TYPE_MASK_SEXPR)

#define sexpr_index(e) ((e) & INDEX_MASK_SEXPR)

#define make_cons(celli) (SEXPR_CONS | (celli))

#define sexpr_eq(e1, e2) ((e1) == (e2))

#define make_builtin_function(table_index) \
	(SEXPR_BUILTIN_FUNCTION | (table_index))

#define make_builtin_special(table_index) \
	(SEXPR_BUILTIN_SPECIAL | (table_index))

#define make_function(lambda_n_alist_celli) \
	(SEXPR_FUNCTION | (lambda_n_alist_celli))

#define make_special(args_n_body_celli) \
	(SEXPR_SPECIAL | (args_n_body_celli))

struct number;

struct number *sexpr_number(SEXPR e);
const char* sexpr_name(SEXPR e);

SEXPR make_symbol(const char *s, int len);
SEXPR make_number(struct number *n);

#endif
