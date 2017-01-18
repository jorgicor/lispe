#ifndef COMMON_H
#define COMMON_H

#define NELEMS(arr) (sizeof(arr)/sizeof(arr[0]))

#define USE_TREE 0

#if USE_TREE
typedef struct literal * LITERAL;
#else
typedef char * LITERAL;
#endif

/* literal will be 4 bytes on 32 bit systems, and 8 on 64 bit systems! */
struct sexpr {
	int type;
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
	SEXPR_BUILTIN_SPECIAL_FORM,
	SEXPR_PROCEDURE,
};

/* sexpr.c */

SEXPR make_cons(int i);
SEXPR make_literal(LITERAL lit);
SEXPR make_number(float n);
SEXPR make_builtin_function(int i);
SEXPR make_builtin_special_form(int i);
SEXPR make_procedure(int args_n_body);

int get_sexpr_type(SEXPR e);
int get_sexpr_index(SEXPR e);
LITERAL get_sexpr_literal(SEXPR e);
float get_sexpr_number(SEXPR e);

/* symtab */

LITERAL new_literal(const char *name, int len);
const char *literal_name(LITERAL lit);
int literals_equal(LITERAL lita, LITERAL litb);

#endif
