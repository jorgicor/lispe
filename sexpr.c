#include "config.h"
#include "cellmark.h"
#include "common.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const SEXPR s_nil = { .type = SEXPR_NIL };

enum {
	TYPE_MASK = ((unsigned int) -1) << SHIFT_SEXPR,
	INDEX_MASK = ~TYPE_MASK,
};

int sexpr_type(SEXPR e)
{
	return e.type & TYPE_MASK;
}

int sexpr_index(SEXPR e)
{
	return e.type & INDEX_MASK;
}

float sexpr_number(SEXPR e)
{
	assert(sexpr_type(e) == SEXPR_NUMBER);
	return get_number(sexpr_index(e));
}

const char* sexpr_name(SEXPR e)
{
	assert(sexpr_type(e) == SEXPR_SYMBOL);
	return get_symbol(sexpr_index(e));
}

int sexpr_eq(SEXPR e1, SEXPR e2)
{
	return e1.type == e2.type;
}

int sexpr_equal(SEXPR e1, SEXPR e2)
{
	int t;

	t = sexpr_type(e1);
	if (t != sexpr_type(e2)) {
		return 0;
	} else if (t == SEXPR_NUMBER) {
		return sexpr_number(e1) == sexpr_number(e2);
	} else {
		return sexpr_eq(e1, e2);
	}
}

SEXPR make_cons(int celli)
{
	SEXPR e;

	e.type = SEXPR_CONS | celli;
	return e;
}

SEXPR make_builtin_function(int table_index)
{
	SEXPR e;

	e.type = SEXPR_BUILTIN_FUNCTION | table_index;
	return e;
}

SEXPR make_builtin_special(int table_index)
{
	SEXPR e;

	e.type = SEXPR_BUILTIN_SPECIAL | table_index;
	return e;
}

SEXPR make_closure(int lambda_n_alist_celli)
{
	SEXPR e;

	e.type = SEXPR_CLOSURE | lambda_n_alist_celli;
	return e;
}

SEXPR make_function(int args_n_body_celli)
{
	SEXPR e;

	e.type = SEXPR_FUNCTION | args_n_body_celli;
	return e;
}

SEXPR make_special(int args_n_body_celli)
{
	SEXPR e;

	e.type = SEXPR_SPECIAL | args_n_body_celli;
	return e;
}

SEXPR make_number(float n)
{
	int i;
	SEXPR e;

	i = install_number(n);
	e.type = SEXPR_NUMBER | i;
	return e;
}

SEXPR make_symbol(const char *s, int len)
{
	int i;
	SEXPR e;

	i = install_symbol(s, len);
	e.type = SEXPR_SYMBOL | i;
	return e;
}

