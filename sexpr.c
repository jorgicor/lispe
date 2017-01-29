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
	assert(sexpr_type(e) == SEXPR_LITERAL);
	return cell_car(sexpr_index(e)).name;
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

#if 0
SEXPR make_number(float n)
{
	SEXPR e;
	int celli;

	celli = pop_free_cell();
	e.number = n;
	set_cell_car(celli, e);
	e.type = SEXPR_NUMBER | celli;
	return e;
}
#endif

/*********************************************************
 * Hash table for literals.
 *********************************************************/

/* prime */
enum { NCHAINS = 16381 };

/*
 * next: next in hash table.
 * celli: index of literal cell (car: name, cdr: nil)
 */
struct literal {
	struct lit_hasht_head *next;
	int celli;
	char name[];
};

/* struct lit_hasht_head is a subtype of struct literal, that is:
 * struct literal *lit;
 * struct lit_hasht_head *h;
 * h = (struct lit_hasht_head *) lit;
 */
struct lit_hasht_head {
	struct lit_hasht_head *next;
};

static struct lit_hasht_head s_hashtab[NCHAINS];

/* Compares a string 'src of length 'len (not null terminated) with a
 * null terminated string 'cstr.
 */
static int cmpstrlen(const char *src, int len, const char *cstr)
{
	int i;

	for (i = 0; i < len && *cstr != '\0'; i++) {
		if (src[i] < *cstr)
			return -1;
		else if (src[i] > *cstr)
			return 1;
		else
			cstr++;
	}

	if (i == len && *cstr == 0)
		return 0;
	else if (i == len)
		return -1;
	else
		return 1;
}

/* Hash the string in 'p of length 'len. */
static unsigned int hash(const char *p, size_t len)
{
	unsigned int h;

	h = 0;
	while (len--) {
		h = 31 * h + (unsigned char) *p;
		p++;
	}

	return h;
}

SEXPR make_literal(const char *s, int len)
{
	SEXPR e;
	int celli;
	unsigned int h;
	struct lit_hasht_head *q;
	struct literal *p;

	h = hash(s, len) % NCHAINS;
	for (q = s_hashtab[h].next; q != NULL; q = q->next) {
		p = (struct literal *) q;
		if (cmpstrlen(s, len, p->name) == 0) {
			e.type = SEXPR_LITERAL | p->celli;
			return e;
		}
	}

	p = malloc(sizeof(*p) + len + 1);
	if (p == NULL) {
		p_gc();
		p = malloc(sizeof(*p) + len + 1);
		if (p == NULL) {
			goto fatal;
		}
	}

	celli = pop_free_cell();
	memcpy(p->name, s, len);
	p->name[len] = '\0';
	p->celli = celli;

#if 0
	printf("literal created %s\n", p->name);
#endif

	p->next = s_hashtab[h].next;
	s_hashtab[h].next = (struct lit_hasht_head *) p;

	e.name = p->name;
	set_cell_car(celli, e); 
	e.type = SEXPR_LITERAL | celli;
	/* set_cell_cdr(celli, e); */
	return e;

fatal:
	fprintf(stderr, "lispe: out of memory\n");
	exit(EXIT_FAILURE);
	return e;
}

void gc_literals(void)
{
	int i, freed;
	struct lit_hasht_head *prev, *p, *q;
	struct literal *plit;

	freed = 0;
	for (i = 0; i < NCHAINS; i++) {
		for (prev = &s_hashtab[i], p = prev->next;
		     p != NULL;
		     prev = p, p = prev->next)
		{
			plit = (struct literal *) p;
			if (!cell_marked(plit->celli)) {
				freed++;
				printf("freed (%s)\n", plit->name);
				prev->next = p->next;
				free(p);
				p = prev;
			}
		}
	}

	printf("[freed literals %d]\n", freed);
}
