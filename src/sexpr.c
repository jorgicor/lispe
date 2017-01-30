#include "config.h"
#include "sexpr.h"
#include "numbers.h"
#include "symbols.h"
#include <assert.h>

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

