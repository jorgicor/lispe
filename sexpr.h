#ifndef SEXPR_H
#define SEXPR_H

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

#endif
