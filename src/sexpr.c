/* ===========================================================================
 * lispe, Scheme interpreter.
 * ===========================================================================
 */

#include "cfg.h"
#include "sexpr.h"
#include "numbers.h"
#include "symbols.h"
#include <assert.h>

struct number *sexpr_number(SEXPR e)
{
	assert(sexpr_type(e) == SEXPR_NUMBER);
	return get_number(sexpr_index(e));
}

const char* sexpr_name(SEXPR e)
{
	assert(sexpr_type(e) == SEXPR_SYMBOL);
	return get_symbol(sexpr_index(e));
}

SEXPR make_number(struct number *n)
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

