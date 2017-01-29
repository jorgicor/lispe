#include "config.h"
#include "cellmark.h"
#include "common.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const SEXPR s_nil = SEXPR_NIL;

#if 0
int sexpr_type(SEXPR e)
{
	return e & TYPE_MASK;
}

int sexpr_index(SEXPR e)
{
	return e & INDEX_MASK;
}
#endif

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

#if 0
int sexpr_eq(SEXPR e1, SEXPR e2)
{
	return e1 == e2;
}
#endif

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

#if 0
SEXPR make_cons(int celli)
{
	return SEXPR_CONS | celli;
}

SEXPR make_builtin_function(int table_index)
{
	return SEXPR_BUILTIN_FUNCTION | table_index;
}

SEXPR make_builtin_special(int table_index)
{
	return SEXPR_BUILTIN_SPECIAL | table_index;
}

SEXPR make_closure(int lambda_n_alist_celli)
{
	return SEXPR_CLOSURE | lambda_n_alist_celli;
}

SEXPR make_function(int args_n_body_celli)
{
	return SEXPR_FUNCTION | args_n_body_celli;
}

SEXPR make_special(int args_n_body_celli)
{
	return SEXPR_SPECIAL | args_n_body_celli;
}
#endif

SEXPR make_number(float n)
{
	int i;

	i = install_number(n);
	return SEXPR_NUMBER | i;
}

SEXPR make_symbol(const char *s, int len)
{
	int i;

	i = install_symbol(s, len);
	return SEXPR_SYMBOL | i;
}

