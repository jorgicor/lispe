#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct literal {
	const char *name;
	int len;
	SEXPR value;
};

enum { NLITERAL = 2048 };
static struct literal literals[NLITERAL];
static int nliteral = 0;

LITERAL new_literal(const char *name, int len)
{
	int i, j;
	struct literal *lit;
	char *dname;

	for (i = 0; i < nliteral; i++) {
		lit = &literals[i];
		if (lit->len != len)
			continue;
		for (j = 0; j < len; j++) {
			if (lit->name[j] != name[j]) {
				break;
			}
		}
		if (j == len) {
			/* found */
			return lit;
		}
	}

	if (nliteral == NLITERAL) {
		printf("Too many symbols\n");
		exit(EXIT_FAILURE);
	}

	lit = &literals[nliteral];
	dname = (char *) malloc(len + 1);
	memcpy(dname, name, len);
	dname[len] = '\0';
	lit->name = dname;
	lit->len = len;
	lit->value = make_undefined();
	nliteral++;
	return lit;
}

#if 0
LITERAL find_literal(const char *name)
{
	return new_literal(name, strlen(name));
}
#endif

SEXPR literal_value(LITERAL lit)
{
	return lit->value;
}

void set_literal_value(LITERAL lit, SEXPR val)
{
	lit->value = val;
}

const char *literal_name(LITERAL lit)
{
	return lit->name;
}
