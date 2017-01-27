#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
#include <limits.h>

static int s_debug = 0;

static void p_gc(void);
static void print(SEXPR sexpr);
static void println(SEXPR sexpr);

static int listlen(SEXPR e);

static SEXPR p_evlis(SEXPR m, SEXPR a);
static SEXPR p_add(SEXPR var, SEXPR val, SEXPR a);
static SEXPR p_eval(SEXPR e, SEXPR a);
static SEXPR p_apply(SEXPR fn, SEXPR x, SEXPR a, int *tailrec, SEXPR *a2);
static SEXPR p_evcon(SEXPR c, SEXPR a);

static SEXPR plus(SEXPR e, SEXPR a);
static SEXPR difference(SEXPR e, SEXPR a);
static SEXPR times(SEXPR e, SEXPR a);
static SEXPR quotient(SEXPR e, SEXPR a);
static SEXPR eq(SEXPR e, SEXPR a);
static SEXPR equal(SEXPR e, SEXPR a);
static SEXPR atom(SEXPR e, SEXPR a);
static SEXPR symbolp(SEXPR e, SEXPR a);
static SEXPR cons(SEXPR e, SEXPR a);
static SEXPR car(SEXPR e, SEXPR a);
static SEXPR cdr(SEXPR e, SEXPR a);
static SEXPR setcar(SEXPR e, SEXPR a);
static SEXPR setcdr(SEXPR e, SEXPR a);
static SEXPR quote(SEXPR e, SEXPR a);
static SEXPR cond(SEXPR e, SEXPR a);
static SEXPR eval(SEXPR e, SEXPR a);
static SEXPR list(SEXPR e, SEXPR a);
static SEXPR lambda(SEXPR e, SEXPR a);
static SEXPR special(SEXPR e, SEXPR a);
static SEXPR closure(SEXPR e, SEXPR a);
static SEXPR body(SEXPR e, SEXPR a);
static SEXPR assoc(SEXPR e, SEXPR a);
static SEXPR setq(SEXPR sexpr, SEXPR a);
static SEXPR lessp(SEXPR sexpr, SEXPR a);
static SEXPR greaterp(SEXPR sexpr, SEXPR a);
static SEXPR consp(SEXPR e, SEXPR a);
static SEXPR numberp(SEXPR e, SEXPR a);
static SEXPR null(SEXPR e, SEXPR a);
static SEXPR gc(SEXPR e, SEXPR a);
static SEXPR quit(SEXPR e, SEXPR a);

static int pop_free_cell(void);

enum {
	ERRORC_OK,
	ERRORC_EOF,
	ERRORC_SYNTAX,
};

enum {
	NCELL = 3000,
};

struct cell {
	SEXPR car;
	SEXPR cdr;
};

static struct cell cells[NCELL];

enum { NCELLMARK = (NCELL / 16) + ((NCELL % 16) ? 1 : 0) };

/* We use 2 bits for each cell, with the values:
 * CELL_FREE, CELL_CREATED and CELL_USED.
 */
static unsigned int s_cellmarks[NCELLMARK];

enum {
	CELL_FREE,
	CELL_CREATED,
	CELL_USED
};

static jmp_buf buf; 

static SEXPR s_true_atom;
static SEXPR s_nil_atom;
static SEXPR s_quote_atom;
static SEXPR s_rest_atom;

/* global environment */
static SEXPR s_env;

/* hidden environment, used to not gc quote, &rest, etc. */
static SEXPR s_hidenv;

/* current computation stack */
static SEXPR s_stack;

/* protected current cons computation */
static SEXPR s_cons_car;
static SEXPR s_cons_cdr;

/* protected push */
static SEXPR s_protect_a;
static SEXPR s_protect_b;
static SEXPR s_protect_c;

/* 
 * List of free cells.
 */
static SEXPR s_free_cells;

/*********************************************************
 * Exceptions.
 *********************************************************/

static void throw_err(void)
{
	longjmp(buf, 1);
}

/*********************************************************
 * Cell marking for gc.
 *********************************************************/

/* 'mark must be CELL_FREE, CELL_CREATED or CELL_USED. */
static void mark_cell(int i, int mark)
{
	int w;

	w = i >> 4;
	assert(w >= 0 && w < NCELLMARK);
	i = (i & 15) << 1;
	s_cellmarks[w] = (s_cellmarks[w] & ~(3 << i)) | (mark << i);
}

/* Returns CELL_FREE, CELL_CREATED or CELL_USED. */
static int cell_mark(int i)
{
	int w;

	w = i >> 4;
	assert(w >= 0 && w < NCELLMARK);
	i = (i & 15) << 1;
	return (s_cellmarks[w] & (3 << i)) >> i;
}

static int if_cell_mark(int i, int ifmark, int thenmark)
{
	int w;
	unsigned int mask;

	w = i >> 4;
	assert(w >= 0 && w < NCELLMARK);
	i = (i & 15) << 1;
	mask = 3 << i;
	if ((s_cellmarks[w] & mask) == (ifmark << i)) {
		s_cellmarks[w] = (s_cellmarks[w] & ~mask) | (thenmark << i);
		return 1;
	}

	return 0;
}

/*********************************************************
 * Hash table for literals.
 *********************************************************/

/* Get the cell at index i into pcell. Range checking. */
#if 0
static void cellp(int i, struct cell *pcell)
{
	assert(i >= 0 && i < NCELL);
	return &cells[i];
}
#else
#define cellp(i,cellp) \
	do { \
		int k; \
		k = i; \
		assert(k >= 0 && k < NCELL); \
		cellp = &cells[k]; \
	} while (0);
#endif

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

/* If celli < 0 a new cell is requested if the literal was not already
 * in the hash table; if it was it is returned.
 * If celli >= 0, the literal is installed on that cell and installed in the
 * hash table (thus potentially producing duplicates).
 * This feature is only used to install "nil" the first time when the cells are
 * not linked on a free list.
 */
SEXPR make_literal_in_cell(const char *s, int len, int celli)
{
	SEXPR e;
	unsigned int h;
	struct lit_hasht_head *q;
	struct literal *p;
	struct cell *pcell;

	h = hash(s, len) % NCHAINS;
	if (celli < 0) {
		for (q = s_hashtab[h].next; q != NULL; q = q->next) {
			p = (struct literal *) q;
			if (cmpstrlen(s, len, p->name) == 0) {
				e.type = SEXPR_LITERAL | p->celli;
				return e;
			}
		}
	}

	p = malloc(sizeof(*p) + len + 1);
	if (p == NULL) {
		if (celli < 0) {
			p_gc();
			p = malloc(sizeof(*p) + len + 1);
			if (p == NULL) {
				goto fatal;
			}
		} else {
			goto fatal;
		}
	}

	if (celli < 0) {
		celli = pop_free_cell();
	}

	memcpy(p->name, s, len);
	p->name[len] = '\0';
	p->celli = celli;

#if 0
	printf("literal created %s\n", p->name);
#endif

	p->next = s_hashtab[h].next;
	s_hashtab[h].next = (struct lit_hasht_head *) p;

	cellp(celli, pcell);
	pcell->car.name = p->name;
	mark_cell(celli, CELL_CREATED);
	e.type = SEXPR_LITERAL | celli;
	pcell->cdr = e;
	return e;

fatal:
	fprintf(stderr, "lispe: out of memory\n");
	exit(EXIT_FAILURE);
	return e;
}

SEXPR make_literal(const char *s, int len)
{
	return make_literal_in_cell(s, len, -1);
}

const char *sexpr_name(SEXPR lit)
{
	struct cell *pcell;

	cellp(sexpr_index(lit), pcell);
	return pcell->car.name;
}

int literals_equal(SEXPR lita, SEXPR litb)
{
	return lita.type == litb.type;
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
			if (cell_mark(plit->celli) == CELL_CREATED) {
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

/*********************************************************
 * Making s-expressions.
 *********************************************************/

#define MASK (((unsigned int) -1) >> 3)

int sexpr_type(SEXPR e)
{
	return e.type & ~MASK;
}

int sexpr_index(SEXPR e)
{
	return e.type & MASK;
}

float sexpr_number(SEXPR e)
{
	struct cell *pcell;

	cellp(sexpr_index(e), pcell);
	return pcell->car.number;
}

SEXPR make_cons(int i)
{
	SEXPR e;

	e.type = SEXPR_CONS | i;
	return e;
}

SEXPR make_number(float n)
{
	SEXPR e;
	int celli;

	celli = pop_free_cell();
	cells[celli].car.number = n;
	e.type = SEXPR_NUMBER | celli;
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

SEXPR make_closure(int celli)
{
	SEXPR e;

	e.type = SEXPR_CLOSURE | celli;
	return e;
}

SEXPR make_function(int celli)
{
	SEXPR e;

	e.type = SEXPR_FUNCTION | celli;
	return e;
}

SEXPR make_special(int celli)
{
	SEXPR e;

	e.type = SEXPR_SPECIAL | celli;
	return e;
}

/*********************************************************
 * Internal predicates and functions, start with p_ 
 *********************************************************/

static int p_symbolp(SEXPR e)
{
	return sexpr_type(e) == SEXPR_LITERAL;
}

static int p_numberp(SEXPR e)
{
	return sexpr_type(e) == SEXPR_NUMBER;
}

static int p_null(SEXPR e)
{
	return sexpr_type(e) == SEXPR_LITERAL && literals_equal(e, s_nil_atom);
}

/* TODO: atom x = not (consp x) ?? */
static int p_atom(SEXPR x)
{
	return p_numberp(x) || p_symbolp(x);
}

static int p_consp(SEXPR e)
{
	return sexpr_type(e) == SEXPR_CONS;
}

static int p_eq(SEXPR x, SEXPR y)
{
	if (p_null(x) && p_null(y)) {
		return 1;
	} else if (p_symbolp(x) && p_symbolp(y)) {
		return literals_equal(x, y);
	} else if (p_numberp(x) && p_numberp(y)) {
		return sexpr_number(x) == sexpr_number(y);
	} else {
		throw_err();
	}
}

static SEXPR p_car(SEXPR e)
{
	struct cell *pcell;

	if (!p_consp(e))
		throw_err();

	cellp(sexpr_index(e), pcell);
	return pcell->car;
}

static SEXPR p_cdr(SEXPR e)
{
	struct cell *pcell;

	if (!p_consp(e))
		throw_err();

	cellp(sexpr_index(e), pcell);
	return pcell->cdr;
}

static int p_equal(SEXPR x, SEXPR y)
{
	for (;;) {
		if (p_atom(x) && p_atom(y)) {
			return p_eq(x, y);
		} else if (p_consp(x) && p_consp(y) &&
			   p_equal(p_car(x), p_car(y)))
	       	{
			x = p_cdr(x);
			y = p_cdr(y);
		} else {
			return 0;
		}
	}
}

static SEXPR p_setcar(SEXPR e, SEXPR val)
{
	struct cell *pcell;

	if (!p_consp(e))
		throw_err();

	cellp(sexpr_index(e), pcell);
	pcell->car = val;
	return val;
}

static SEXPR p_setcdr(SEXPR e, SEXPR val)
{
	struct cell *pcell;

	if (!p_consp(e))
		throw_err();

	cellp(sexpr_index(e), pcell);
	pcell->cdr = val;
	return val;
}

/* 'a is an association list ((A . B) (C . D) ... ).
 * Return the first pair whose 'car is 'x.
 */
static SEXPR p_assoc(SEXPR x, SEXPR a)
{
	for (;;) {
		if (p_null(a)) {
			return s_nil_atom;
		} else if (p_equal(p_car(p_car(a)), x)) {
			return p_car(a);
		} else {
			a = p_cdr(a);
		}
	}
}

static int pop_free_cell(void)
{
	int i;

	if (p_null(s_free_cells)) {
		p_gc();
		if (p_null(s_free_cells)) {
			printf("lisep: No more free cells.\n");
			exit(EXIT_FAILURE);
		}
	}

	i = sexpr_index(s_free_cells);
	mark_cell(i, CELL_CREATED);
	s_free_cells = p_cdr(s_free_cells);
	return i;
}

/* Makes an sexpr form two sexprs. */
static SEXPR p_cons(SEXPR first, SEXPR rest)
{
	int i;
	
	/* protect */
	s_cons_car = first;
	s_cons_cdr = rest;

	i = pop_free_cell();
	cells[i].car = first;
	cells[i].cdr = rest;

	s_cons_car = s_nil_atom;
	s_cons_cdr = s_nil_atom;

	return make_cons(i);
}

/*********************************************************
 * push and pop to stack to protect from gc.
 *********************************************************/

/* Protect expression form gc by pushin it to s_stack. Return e. */
static SEXPR push(SEXPR e)
{
	s_stack = p_cons(e, s_stack);
	return e;
}

static void push2(SEXPR e1, SEXPR e2)
{
	s_protect_a = e1;
	s_protect_b = e2;
	push(e1);
	push(e2);
	s_protect_a = s_nil_atom;
	s_protect_b = s_nil_atom;
}

static void push3(SEXPR e1, SEXPR e2, SEXPR e3)
{
	s_protect_a = e1;
	s_protect_b = e2;
	s_protect_c = e3;
	push(e1);
	push(e2);
	push(e3);
	s_protect_a = s_nil_atom;
	s_protect_b = s_nil_atom;
	s_protect_c = s_nil_atom;
}

/* Pop last expression from stack. */
static void pop(void)
{
	assert(!p_null(s_stack));
	s_stack = p_cdr(s_stack);
}

static void popn(int n)
{
	assert(n >= 0);
	while (n--) {
		pop();
	}
}

/* Given two lists, 'x and 'y, returns a list with pairs of each list, i.e.:
 * (A B) (C D) -> ((A C) (B D))
 * and adds it to the start of the list 'a.
 * The list 'x must contain symbols.
 * The list 'x can be shorter than 'y; in that case, if the last element of 'x
 * is the symbol &rest, it will be paired with a list containing the remaining
 * elements of 'y.
 */
static SEXPR p_pairargs(SEXPR x, SEXPR y, SEXPR a)
{
	SEXPR head, node, node2;

	if (p_null(x))
		return a;

	push3(x, y, a);

	/* Handle the case of only one parameter called &rest */
	if (p_null(p_cdr(x)) && p_eq(p_car(x), s_rest_atom)) {
		node = p_cons(p_cons(p_car(x), y), s_nil_atom);
		p_setcdr(node, a);
		popn(3);
		return node;
	}

	head = push(p_cons(p_cons(p_car(x), p_car(y)), s_nil_atom));
	node = head;
	x = p_cdr(x);
	y = p_cdr(y);
	while (!p_null(x)) {
		if (p_null(p_cdr(x)) && p_eq(p_car(x), s_rest_atom)) {
			node2 = p_cons(p_cons(p_car(x), y), s_nil_atom);
			p_setcdr(node, node2);
			node = node2;
		} else {
			node2 = p_cons(p_cons(p_car(x), p_car(y)), s_nil_atom);
			p_setcdr(node, node2);
			node = node2;
			y = p_cdr(y);
		}
		x = p_cdr(x);
	}
	p_setcdr(node, a);
	popn(4);

	return head;
}

struct builtin {
	const char* id;
	SEXPR (*fun)(SEXPR, SEXPR);
};

static struct builtin builtin_functions[] = {
	{ "atom", &atom },
	{ "assoc", &assoc },
	{ "car",  &car },
	{ "cdr", &cdr },
	{ "cons", &cons },
	{ "consp", &consp },
	{ "-", &difference},
	{ "eq", &eq },
	{ "equal", &equal },
	{ ">", &greaterp },
	{ "gc", &gc },
	{ "<", &lessp },
	{ "null", &null },
	{ "numberp", &numberp },
	{ "+", &plus },
	{ "setcar", &setcar },
	{ "setcdr", &setcdr },
	{ "symbolp", &symbolp },
	{ "*", &times },
	{ "quit", &quit },
	{ "/", &quotient },
	/* modulo and remainder */
};

static struct builtin builtin_specials[] = {
	{ "body", &body },
	{ "cond", &cond },
	{ "eval", &eval },
	{ "lambda", &lambda },
	{ "closure", &closure },
	{ "list", &list },
	{ "quote", &quote },
	{ "setq", &setq },
	{ "special", &special },
};

static SEXPR apply_builtin_function(int i, SEXPR args, SEXPR a)
{
	return builtin_functions[i].fun(args, a);
}

static SEXPR apply_builtin_special(int i, SEXPR args, SEXPR a)
{
	return builtin_specials[i].fun(args, a);
}

struct input_channel {
	FILE *file;
};

struct input_channel *read_console(struct input_channel *ic)
{
	ic->file = stdin;
	return ic;
}

struct input_channel *read_file(struct input_channel *ic, FILE *fp)
{
	ic->file = fp;
	return ic;
}

int getc_from_channel(struct input_channel *ic)
{
	return fgetc(ic->file);
}

enum {
	T_ATOM = 1024,
	T_NUMBER,
};

enum {
	MAX_NAME = 32
};

struct token {
	int type;
	union {
		struct {
			char name[MAX_NAME];
			int len;
		} atom;
		float number;
	} value;
};

struct tokenizer {
	struct input_channel *in;
	struct token tok;
	int peekc;
};

struct tokenizer *tokenize(struct input_channel *ic,
	struct tokenizer *t)
{
	t->in = ic;
	t->tok.type = EOF;
	t->peekc = EOF;
	return t;
}

static int is_id_start(int c)
{
	return isalpha(c) || (strchr("?!+-*/<=>:$%^&_~@", c) != NULL);
}

static int is_id_next(int c)
{
	return isdigit(c) || isalpha(c) ||
	       	(strchr("?!+-*/<=>:$%^&_~@", c) != NULL);
}

struct token *pop_token(struct tokenizer *t)
{
	int c;
	int i;
	float n;

again:	
	if (t->peekc >= 0) {
		c = t->peekc;
		t->peekc = EOF;
	} else {
		c = getc_from_channel(t->in);
	}

	if (c == EOF) {
		t->tok.type = EOF;
	} else if (c == '(' || c == ')' || c == '.' || c == '\'') {
		t->tok.type = c;
	} else if (is_id_start(c)) {
		t->tok.type = T_ATOM;
		i = 0;
		while (is_id_next(c)) {
			if (i < MAX_NAME) {
				t->tok.value.atom.name[i++] = c;
			}
			c = getc_from_channel(t->in);
		}
		t->tok.value.atom.len = i;
		t->peekc = c;
	} else if (isdigit(c)) {
		t->tok.type = T_NUMBER;
		n = 0.0f;
		while (c == '0') {
			c = getc_from_channel(t->in);
		}
		while (isdigit(c)) {
			n = n * 10 + (c - '0');
			c = getc_from_channel(t->in);
		}
		t->tok.value.number = n;
		t->peekc = c;
	} else if (isspace(c)) {
		while (isspace(c)) {
			c = getc_from_channel(t->in);
		}
		t->peekc = c;
		goto again;
	} else {
		/* skip wrong token for now */
		goto again;
	}

	return &t->tok;
}

struct token *peek_token(struct tokenizer *t)
{
	return &t->tok;
}

enum {
	PARSER_NSTACK = 1024
};

struct parser {
	struct tokenizer *tokenizer;
	// char stack[PARSER_NSTACK];
	// SEXPR retval[PARSER_NSTACK];
	int sp;
	// int state;
};

struct parser *parse(struct tokenizer *t, struct parser *p)
{
	p->tokenizer = t;
	p->sp = 0;
	// p->state = 0;
	return p;
}

#define PARSER_PUSH(s)				\
	if (p->sp == PARSER_NSTACK) {		\
		goto error;			\
	} else {				\
		p->stack[p->sp++] = (s);	\
		p->state = 0;			\
		goto again;			\
	}

#define PARSER_POP(retv)			\
	if (p->sp > 0)	{			\
		pop_token(p->tokenizer);	\
	}					\
	if (p->sp == 0)	{			\
		return retv;			\
	} else {				\
		p->state = p->stack[--p->sp];	\
		p->retval[p->sp] = retv;	\
		goto again;			\
	}

/*
 * Parses S-Expressions non recursively.
 * We manage our own stack.
 * Exits immediatly on error without needing stack unwinding.
 */
#if 0
SEXPR parse_sexpr(struct parser *p, int *iserror)
{
	struct token *tok;
	SEXPR sexpr, car, cdr;

	*iserror = 0;
again:	switch (p->state) {
	case 0:	tok = peek_token(p->tokenizer);
		if (tok->type == T_ATOM) {			
			sexpr = new_symbol(tok->value.atom.name, tok->value.atom.len);
			PARSER_POP(sexpr);
		} else if (tok->type == T_NUMBER) {
			sexpr = make_number(tok->value.number);
			PARSER_POP(sexpr);
		} else if (tok->type == '(') {
			tok = pop_token(p->tokenizer);
			if (tok->type == ')') {
				PARSER_POP(make_nil());
			} else {
				head = make_nil();
			do {
				PARSER_PUSH(1);
	case 1:			if (is_nil(head)) {
					head = make_cons(p->retval[p->sp], make_nil());
					list = head;
				} else {
					list.cdr = make_cons(p->retval[p->sp], make_nil());
					list = list.cdr;
				}
				if (tok->type == '.') {
					pop_token(p->tokenizer);
					PARSER_PUSH(2);
	case 2:				cdr = p->retval[p->sp];
					if (tok->type != ')') {
						goto error;
					}
					sexpr = make_cons(car, cdr);
					PARSER_POP(sexpr);
				} else if (tok->type == ')') {
					sexpr = make_cons(car, make_nil());
					PARSER_POP(sexpr);
				}
			} while (1);
		}
	}

error:	*iserror = 1;
	return make_nil();
}
#endif

static SEXPR parse_sexpr(struct parser *p, int *errorc);

static SEXPR parse_list(struct parser *p, int *errorc)
{
	struct token *tok;
	SEXPR car, cdr;

	tok = peek_token(p->tokenizer);
	if (tok->type == ')') {
		return s_nil_atom;
	}

	car = parse_sexpr(p, errorc);
	if (*errorc != ERRORC_OK)
		goto error;

	push(car);
	if (tok->type == '.') {
		pop_token(p->tokenizer);
		cdr = parse_sexpr(p, errorc);
	} else {
		cdr = parse_list(p, errorc);
	}
	pop();

	if (*errorc != ERRORC_OK)
		goto error;

	return p_cons(car, cdr);

error:	if (*errorc == ERRORC_OK)
	       	ERRORC_SYNTAX;
	return s_nil_atom;
}

static SEXPR parse_quote(struct parser *p, int *errorc)
{
	SEXPR sexpr;

	sexpr = parse_sexpr(p, errorc);
	if (*errorc != ERRORC_OK)
		return s_nil_atom;

	sexpr = p_cons(sexpr, s_nil_atom);
	sexpr = p_cons(s_quote_atom, sexpr);
	return sexpr;
}

static SEXPR parse_sexpr(struct parser *p, int *errorc)
{
	struct token *tok;
	SEXPR sexpr;

	tok = peek_token(p->tokenizer);
	if (tok->type == EOF) {
		*errorc = ERRORC_EOF;
		return s_nil_atom;
	} else if (tok->type == T_ATOM) {	
		sexpr = make_literal(tok->value.atom.name,
			       	     tok->value.atom.len);
		if (p->sp > 0) {
			pop_token(p->tokenizer);
		}
		return sexpr;
	} else if (tok->type == T_NUMBER) {
		sexpr = make_number(tok->value.number);
		if (p->sp > 0) {
			pop_token(p->tokenizer);
		}
		return sexpr;
	} else if (tok->type == '\'') {
		pop_token(p->tokenizer);
		return parse_quote(p, errorc);
	} else if (tok->type == '(') {
		p->sp++;
		pop_token(p->tokenizer);
		sexpr = parse_list(p, errorc);
		if (*errorc != ERRORC_OK) {
			goto error;
		}
		tok = peek_token(p->tokenizer);
		if (tok->type != ')') {
			goto error;
		}
		p->sp--;
		if (p->sp > 0) {
			pop_token(p->tokenizer);
		}
		return sexpr;
	}

error:	if (*errorc != ERRORC_OK)
		*errorc = ERRORC_SYNTAX;
	return s_nil_atom;
}

static SEXPR get_sexpr(struct parser *p, int *errorc)
{
	*errorc = ERRORC_OK;
	pop_token(p->tokenizer);
	return parse_sexpr(p, errorc);
}

static void clear_stack(void)
{
	s_stack = s_nil_atom;
	s_cons_car = s_nil_atom;
	s_cons_cdr = s_nil_atom;
	s_protect_a = s_nil_atom;
	s_protect_b = s_nil_atom;
	s_protect_c = s_nil_atom;
}

static void load_init_file(void)
{
	struct input_channel ic;
	struct tokenizer t;
	struct parser p;
	SEXPR sexpr;
	int errorc;
	FILE *fp;

	if (setjmp(buf)) {
		printf("lispe: error loading init.lisp\n");
		clear_stack();
		goto end;
	}

	fp = fopen("init.lisp", "r");
	if (!fp)
		return;

	tokenize(read_file(&ic, fp), &t);
	do {
		sexpr = get_sexpr(parse(&t, &p), &errorc);
		if (errorc == ERRORC_OK) {
			p_eval(sexpr, s_nil_atom); 
		}
	} while (errorc == ERRORC_OK);

	if (errorc == ERRORC_SYNTAX) {
		printf("lispe: syntax error on init.lisp\n");
	} else {
		printf("[init.lisp loaded ok]\n");
	}

end:	fclose(fp);
}

static SEXPR read(int *errorc)
{
	struct input_channel ic;
	struct tokenizer t;
	struct parser p;
	SEXPR sexpr;

	return get_sexpr(parse(tokenize(read_console(&ic), &t), &p),
			 errorc);
}

/****************************************************/

static SEXPR quote(SEXPR e, SEXPR a)
{
	return p_car(e);
}

static SEXPR cond(SEXPR e, SEXPR a)
{
	return p_evcon(e, a);
}

/* TODO: make builtin function so the arguments eval automatically? */
static SEXPR list(SEXPR e, SEXPR a)
{
	return p_evlis(e, a);
}

static SEXPR assoc(SEXPR e, SEXPR a)
{
	return p_assoc(p_car(e), p_car(p_cdr(e)));
}

static SEXPR quit(SEXPR e, SEXPR a)
{
	exit(EXIT_SUCCESS);
}

static SEXPR setq(SEXPR sexpr, SEXPR a)
{
	SEXPR var;
	SEXPR val;
	SEXPR bind;

	val = s_nil_atom;
	while (!p_null(sexpr)) {
		var = p_car(sexpr);
		if (!p_symbolp(var))
			throw_err();

		sexpr = p_cdr(sexpr);
		val = p_eval(p_car(sexpr), a);
		bind = p_assoc(var, a);
		if (p_null(bind)) {
			bind = p_assoc(var, s_env);
			if (p_null(bind)) {
				bind = p_cons(var, val);
				s_env = p_cons(bind, s_env);
			} else {
				p_setcdr(bind, val);
			}
		} else {
			p_setcdr(bind, val);
		}
		sexpr = p_cdr(sexpr);
	}

	return val;
}

static SEXPR cons(SEXPR e, SEXPR a)
{
	return p_cons(p_car(e), p_car(p_cdr(e)));
}

static SEXPR arith(SEXPR e, SEXPR a, float n, float (*fun)(float, float))
{
	e = p_evlis(e, a);
	while (!p_null(e)) {
		if (!p_numberp(p_car(e)))
			throw_err();
		n = fun(n, sexpr_number(p_car(e)));
		e = p_cdr(e);
	}
	return make_number(n);
}

static SEXPR logic(SEXPR e, SEXPR a, int (*fun)(float, float))
{
	int b;
	float n, n2;

	e = p_evlis(e, a);
	if (p_null(e) || !p_numberp(p_car(e)))
		throw_err();

	n = sexpr_number(p_car(e));
	e = p_cdr(e);
	while (!p_null(e)) {
		if (!p_numberp(p_car(e)))
			throw_err();
		n2 = sexpr_number(p_car(e));
		if (!fun(n, n2))
			return s_nil_atom;
		n = n2;
		e = p_cdr(e);
	}

	return s_true_atom;
}

static int lessp_fun(float a, float b)
{
	return a < b;
}

static SEXPR lessp(SEXPR e, SEXPR a)
{
	return logic(e, a, lessp_fun);
}

static int greaterp_fun(float a, float b)
{
	return a > b;
}

static SEXPR greaterp(SEXPR e, SEXPR a)
{
	return logic(e, a, greaterp_fun);
}

static float plus_fun(float a, float b)
{
	return a + b;
}

static SEXPR plus(SEXPR e, SEXPR a)
{
	return arith(e, a, 0, plus_fun);
}

static float difference_fun(float a, float b)
{
	return a - b;
}

static SEXPR difference(SEXPR e, SEXPR a) 
{
	float n;

	e = p_evlis(e, a);
	if (p_null(e) || !p_numberp(p_car(e)))
		throw_err();

	n = sexpr_number(p_car(e));
	if (p_null(p_cdr(e)))
		return make_number(-n);

	e = push(p_cdr(e));
	e = arith(e, a, n, difference_fun);
	pop();
	return e;
}

static float times_fun(float a, float b)
{
	return a * b;
}

static SEXPR times(SEXPR e, SEXPR a)
{
	return arith(e, a, 1, times_fun);
}

static float quotient_fun(float a, float b)
{
	return a / b;
}

static SEXPR quotient(SEXPR e, SEXPR a)
{
	float n;

	e = p_evlis(e, a);
	if (p_null(e) || !p_numberp(p_car(e)))
		throw_err();

	n = sexpr_number(p_car(e));
	if (p_null(p_cdr(e)))
		return make_number(1.0f / n);

	e = push(p_cdr(e));
	e = arith(e, a, n, quotient_fun);
	pop();
	return e;
}

static SEXPR car(SEXPR e, SEXPR a)
{
	return p_car(p_car(e));
}

static SEXPR cdr(SEXPR e, SEXPR a)
{
	return p_cdr(p_car(e));
}

static SEXPR setcar(SEXPR e, SEXPR a)
{
	SEXPR p, q;

	p = p_car(e);
	q = p_car(p_cdr(e));
	return p_setcar(p, q); 
}

static SEXPR setcdr(SEXPR e, SEXPR a)
{
	SEXPR p, q;

	p = p_car(e);
	q = p_car(p_cdr(e));
	return p_setcdr(p, q); 
}

static SEXPR atom(SEXPR e, SEXPR a)
{
	return p_atom(p_car(e)) ? s_true_atom : s_nil_atom;
}

static SEXPR symbolp(SEXPR e, SEXPR a)
{
	return p_symbolp(p_car(e)) ? s_true_atom : s_nil_atom;
}

static SEXPR consp(SEXPR e, SEXPR a)
{
	return p_consp(p_car(e)) ? s_true_atom : s_nil_atom;
}

static SEXPR numberp(SEXPR e, SEXPR a)
{
	return p_numberp(p_car(e)) ? s_true_atom : s_nil_atom;
}

static SEXPR null(SEXPR e, SEXPR a)
{
	return p_null(p_car(e)) ? s_true_atom : s_nil_atom;
}

static SEXPR eq(SEXPR e, SEXPR a)
{
	return p_eq(p_car(e), p_car(p_cdr(e))) ? s_true_atom : s_nil_atom;
}

static SEXPR lambda(SEXPR e, SEXPR a)
{
	return make_function(sexpr_index(e));
}

static SEXPR special(SEXPR e, SEXPR a)
{
	return make_special(sexpr_index(e));
}

static SEXPR closure(SEXPR e, SEXPR a)
{
	return make_closure(sexpr_index(p_cons(e, a)));
}

/* TODO: change for symbol-function and print like (lambda (x) (nc ...)) */
static SEXPR body(SEXPR e, SEXPR a)
{
	struct cell *pcell;

	/* TODO: admit any number of arguments */
	e = p_evlis(e, a);
	e = p_car(e);
	switch (sexpr_type(e)) {
	case SEXPR_CLOSURE:
		cellp(sexpr_index(e), pcell);
		e = pcell->car;
		return e;
	case SEXPR_FUNCTION:
	case SEXPR_SPECIAL:
		cellp(sexpr_index(e), pcell);
		e = p_cons(pcell->car, pcell->cdr);
		return e;
	case SEXPR_BUILTIN_FUNCTION:
	case SEXPR_BUILTIN_SPECIAL:
		return s_nil_atom;
	default:
		throw_err();
	}
}

static SEXPR p_add(SEXPR var, SEXPR val, SEXPR a)
{
	return p_cons(p_cons(var, val), a);
}

static SEXPR equal(SEXPR e, SEXPR a)
{
	if (p_equal(p_car(e), p_car(p_cdr(e))))
		return s_true_atom;
	else
		return s_nil_atom;
}

/* eval each list member.  */
static SEXPR p_evlis(SEXPR m, SEXPR a)
{
	SEXPR head, node, node2;
	
	if (p_null(m))
		return m;

	push2(m, a);
	head = push(p_cons(p_eval(p_car(m), a), s_nil_atom));
	node = head;
	m = p_cdr(m);
	while (!p_null(m)) {
		node2 = p_cons(p_eval(p_car(m), a), s_nil_atom);
		p_setcdr(node, node2);
		node = node2;
		m = p_cdr(m);
	}
	popn(3);
	return head;
}

/* eval COND */
static SEXPR p_evcon(SEXPR c, SEXPR a)
{
	for (;;) {
		if (p_eq(p_eval(p_car(p_car(c)), a), s_true_atom)) {
			return p_eval(p_car(p_cdr(p_car(c))), a);
		} else {
			c = p_cdr(c);
		}
	}
}

/* If tailrec != NULL, we will act tail recursive, that is, the last
 * expression in the function to apply will be returned unevaluated.
 * In that case, *tailrec will be set to 1 or 0.
 * If 1, it means that the SEXPR returned is the last expression of a
 * sequence and has not been evaluated. Eval can evaluate it and 'a2
 * will be the environment in which to evaluate.
 * This is to implement tail recursion, until a better idea.
 * If 0, the return value is already the final one, eval does not need to
 * evaluate anything.
 */
static SEXPR p_apply(SEXPR fn, SEXPR x, SEXPR a, int *tailrec, SEXPR *a2)
{
	SEXPR e1, r;
	struct cell *pcell;

#if 0
	printf("apply fn: ");
	println(fn);
	printf("x: ");
	println(x);
	printf("a: ");
	println(a);
#endif
	if (tailrec) {
		assert(a2);
		*tailrec = 0;
	}

	push3(a, x, fn);
	switch (sexpr_type(fn)) {
	case SEXPR_BUILTIN_FUNCTION:
		r = apply_builtin_function(sexpr_index(fn), x, a);
		popn(3);
		break;
	case SEXPR_BUILTIN_SPECIAL:
		r = apply_builtin_special(sexpr_index(fn), x, a);
		popn(3);
		break;
	case SEXPR_FUNCTION:
	case SEXPR_SPECIAL:
		cellp(sexpr_index(fn), pcell);
		/* pair parameters with their arguments and append 'a. */
		a = p_pairargs(pcell->car, x, a);
		/* eval sequence except the last expression */
		e1 = pcell->cdr;
		while (!p_null(e1)) {
			/* last expr */
			if (p_null(p_cdr(e1))) {
				break;
			}
			r = p_eval(p_car(e1), a);
			e1 = p_cdr(e1);
		}
		if (!p_null(e1)) {
			r = p_car(e1);
		} else {
			r = s_nil_atom;
		}
		popn(3);
		if (!p_null(r)) {
			if (tailrec == NULL) {
				r = p_eval(r, a);
			} else {
				*tailrec = 1;
				*a2 = a;
			}
		}
		break;
	default:
		throw_err();
	}

	return r;
}

static int s_evalc = 0;

static int listlen(SEXPR e)
{
	int n;

	if (!p_consp(e))
		return 0;

	n = 0;
	while (!p_null(e)) {
		n++;
		e = p_cdr(e);
	}
	return n;
}

static SEXPR p_eval(SEXPR e, SEXPR a)
{
	int tailrec, t;
	SEXPR c, e1, a2;
	struct cell *pcell;

	s_evalc++;
#if 1
	printf("eval stack %d\n", s_evalc);
	println(s_stack);
#endif

again:  switch (sexpr_type(e)) {
	case SEXPR_NUMBER:
		s_evalc--;
		return e;
	case SEXPR_LITERAL:
		c = p_assoc(e, a);
		if (p_null(c)) {
			c = p_assoc(e, s_env);
		}
		c = p_cdr(c);
		s_evalc--;
		return c;
	case SEXPR_CONS:
		if (s_debug) {
			printf("eval: ");
			println(e);
			printf("a: ");
			println(a);
		}
		/* evaluate the operator */
		push2(a, e);
		c = p_eval(p_car(e), a);
		t = sexpr_type(c);
		switch (t) {
		case SEXPR_BUILTIN_SPECIAL:
		case SEXPR_SPECIAL:
			/* special forms evaluate the arguments on demand */
			popn(2);
			c = p_apply(c, p_cdr(e), a, &tailrec, &a2);
			break;
		default:
			/* evaluate the arguments */
			push(c);
			e1 = p_evlis(p_cdr(e), a);
			popn(3);
			if (t == SEXPR_CLOSURE) {
				/* a closure overrides the environment
				 * with its saved environment.
				 */
				cellp(sexpr_index(c), pcell);
				c = lambda(pcell->car, a);
				a = pcell->cdr;
			}
			c = p_apply(c, e1, a, &tailrec, &a2);
		}
		if (tailrec) {
			e = c;
			a = a2;
			goto again;
		}
		if(s_debug) {
			printf("r: ");
			println(c);
		}
		s_evalc--;
		return c;

	default:
		throw_err();
	}
}

static SEXPR eval(SEXPR e, SEXPR a)
{
	e = p_eval(p_car(e), a);
	e = p_eval(e, a);
	return e;
}

static void print(SEXPR sexpr)
{
	int i;

	switch (sexpr_type(sexpr)) {
	case SEXPR_CONS:
		/* TODO: avoid infinite recursion */
		i = sexpr_index(sexpr);
		printf("(");
		print(cells[i].car);
		while (!p_null(cells[i].cdr)) {
			sexpr = cells[i].cdr;
			if (sexpr_type(sexpr) == SEXPR_CONS) {
				i = sexpr_index(sexpr);
				printf(" ");
				print(cells[i].car);
			} else {
				printf(" . ");
				print(sexpr);
				break;
			}
		}
		printf(")");
		break;
	case SEXPR_LITERAL:
		printf("%s", sexpr_name(sexpr));
		break;
	case SEXPR_NUMBER:
		printf("%g", sexpr_number(sexpr));
		break;
	case SEXPR_BUILTIN_FUNCTION:
		printf("{builtin function %s}",
			builtin_functions[sexpr_index(sexpr)].id);
		break;
	case SEXPR_BUILTIN_SPECIAL:
		printf("{builtin special %s}",
			builtin_specials[sexpr_index(sexpr)].id);
		break;
	case SEXPR_FUNCTION:
		printf("{function}");
#if 0
		printf("(lambda ");
		print(cells[sexpr_index(sexpr)].car);
		print(cells[sexpr_index(sexpr)].cdr);
		printf(")");
#endif
		break;
	case SEXPR_SPECIAL:
		printf("{special}");
		break;
	case SEXPR_CLOSURE:
		printf("{closure}");
		break;
	}
}

static void println(SEXPR sexpr)
{
	print(sexpr);
	printf("\n");
}

static void install_builtin(const char *name, SEXPR e)
{
	SEXPR var;

	var = make_literal(name, strlen(name));
	s_env = p_add(var, e, s_env);
}

static void install_builtin_functions(void)
{
	int i;

	for (i = 0; i < NELEMS(builtin_functions); i++) {
		install_builtin(builtin_functions[i].id,
			make_builtin_function(i));
	}
}

static void install_builtin_specials(void)
{
	int i;

	for (i = 0; i < NELEMS(builtin_specials); i++) {
		install_builtin(builtin_specials[i].id,
			make_builtin_special(i));
	}
}

static void install_literals(void)
{
	s_env = p_add(s_nil_atom, s_nil_atom, s_env);

	s_true_atom = make_literal("t", 1);
	s_env = p_add(s_true_atom, s_true_atom, s_env);

	s_quote_atom = make_literal("quote", 5);
	s_hidenv = p_add(s_quote_atom, s_quote_atom, s_hidenv);

	s_rest_atom = make_literal("&rest", 5);
	s_hidenv = p_add(s_rest_atom, s_rest_atom, s_hidenv);
}

static void gc_mark(SEXPR e)
{
	int celli;
	struct cell *pcell;

	switch (sexpr_type(e)) {
	case SEXPR_NUMBER:
	case SEXPR_LITERAL:
		celli = sexpr_index(e);
		if_cell_mark(celli, CELL_CREATED, CELL_USED);
		break;
	case SEXPR_FUNCTION:
	case SEXPR_SPECIAL:
	case SEXPR_CLOSURE:
	case SEXPR_CONS:
		celli = sexpr_index(e);
		cellp(celli, pcell);
		if (if_cell_mark(celli, CELL_CREATED, CELL_USED)) {
			gc_mark(pcell->car);
			gc_mark(pcell->cdr);
		}
		break;
	}
}

static void p_gc(void)
{
	int i, used, freed;
	SEXPR e;
	struct cell *pcell;

	/* Mark used. */
	gc_mark(s_hidenv);
	gc_mark(s_env);
	gc_mark(s_stack);
	gc_mark(s_cons_car);
	gc_mark(s_cons_cdr);
	gc_mark(s_protect_a);
	gc_mark(s_protect_b);
	gc_mark(s_protect_c);

	gc_literals();

	/* Return to s_free_cells those marked as CELL_CREATED and set
	 * them to CELL_FREE. Set all marked as CELL_USED to CELL_CREATED.
	 */
	used = freed = 0;
	for (i = 0; i < NCELL; i++) {
		switch (cell_mark(i)) {
		case CELL_CREATED:
			freed++;
			mark_cell(i, CELL_FREE);
			cells[i].car = s_nil_atom;
			if (p_null(s_free_cells)) {
				cells[i].cdr = s_nil_atom;
			} else {
				cells[i].cdr = make_cons(
					sexpr_index(s_free_cells));
			}
			s_free_cells = make_cons(i);
			break;
		case CELL_USED:
			used++;
			mark_cell(i, CELL_CREATED);
			break;
		}
	}

	printf("[cells used %d, freed %d]\n", used, freed);

#if 0
	i = 0;
	e = s_free_cells;
	while (!p_null(e)) {
		i++;
		e = p_cdr(e);
	}

	printf("on free list %d\n", i);
#endif
}

static SEXPR gc(SEXPR e, SEXPR a)
{
	p_gc();
	return s_nil_atom;
}

int main(int argc, char* argv[])
{
	int i, errorc;
	SEXPR e;

	printf("lispe minimal lisp 1.0\n\n");
	printf("[ncells: %d, ncellmarks: %d]\n", NCELL, NCELLMARK);
	printf("[bytes sexpr: %u, cell: %u, cells: %u, cellmarks: %u]\n",
		sizeof(SEXPR),
		sizeof(cells[0]),
		NCELL * sizeof(cells[0]),
	       	NCELLMARK * sizeof(s_cellmarks[0]));

	s_nil_atom = make_literal_in_cell("nil", 3, 0);
	s_stack = s_nil_atom;
	s_cons_car = s_nil_atom;
	s_cons_cdr = s_nil_atom;

	/* link cells for the free cells list */
	cells[NCELL - 1].car = s_nil_atom;
	cells[NCELL - 1].cdr = s_nil_atom;
	/* skip nil atom cell */
	for (i = 1; i < NCELL - 1; i++) {
		cells[i].car = s_nil_atom;
		cells[i].cdr = make_cons(i + 1);
	}
	s_free_cells = make_cons(1);

	s_env = s_nil_atom;
	s_hidenv = s_nil_atom;
	install_literals();
	install_builtin_functions();
	install_builtin_specials();

	load_init_file();

	/* REPL */
	for (;;) {
		printf("lispe> ");
		if (!setjmp(buf)) {
			e = read(&errorc);
			if (errorc == ERRORC_EOF) {
				break;
			} else if (errorc == ERRORC_SYNTAX) {
				printf("lispe: syntax error\n");
			} else {
				println(p_eval(e, s_nil_atom));
			}
		} else {
			printf("lispe: ** error **\n");
			clear_stack();
		}
		assert(p_equal(s_stack, s_nil_atom));
	}

	return 0;
}
