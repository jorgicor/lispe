#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>

static void gc(void);
static void print(SEXPR sexpr);
static void println(SEXPR sexpr);
static SEXPR p_assoc(SEXPR x, SEXPR a);
static SEXPR p_evlis(SEXPR m, SEXPR a);
static SEXPR p_add(SEXPR var, SEXPR val, SEXPR a);
static SEXPR p_eval(SEXPR e, SEXPR a);
static SEXPR p_apply(SEXPR fn, SEXPR x, SEXPR a);
static SEXPR plus(SEXPR e, SEXPR a);
static SEXPR difference(SEXPR e, SEXPR a);
static SEXPR times(SEXPR e, SEXPR a);
static SEXPR quotient(SEXPR e, SEXPR a);
static SEXPR eq(SEXPR e, SEXPR a);
static SEXPR equal(SEXPR e, SEXPR a);
static SEXPR atom(SEXPR e, SEXPR a);
static SEXPR cons(SEXPR e, SEXPR a);
static SEXPR car(SEXPR e, SEXPR a);
static SEXPR cdr(SEXPR e, SEXPR a);
static SEXPR quote(SEXPR e, SEXPR a);
static SEXPR cond(SEXPR e, SEXPR a);
static SEXPR p_evcon(SEXPR c, SEXPR a);
static SEXPR lambda(SEXPR e, SEXPR a);
static SEXPR label(SEXPR e, SEXPR a);
static SEXPR setq(SEXPR sexpr, SEXPR a);
static int p_null(SEXPR e);
static int p_atom(SEXPR x);
static int p_number(SEXPR e);
static int p_symbol(SEXPR e);

enum {
	CELL_MARK_FREE,
	CELL_MARK_CREATED,
	CELL_MARK_USED
};

struct cell {
	signed char mark;
	SEXPR car;
	SEXPR cdr;
};

static jmp_buf buf; 

static SEXPR s_true_atom;
static SEXPR s_nil_atom;

/* global environment */
static SEXPR s_env;

/* current computation stack */
static SEXPR s_stack;

/* protected current cons computation */
static SEXPR s_cons_car;
static SEXPR s_cons_cdr;

enum { NCELL = 100 };
static struct cell cells[NCELL];

#if 0
static void cellp(int i, struct cell *pcell)
{
	assert(i >= 0 && i < NCELL);
	return &cells[i];
}
#endif

#define cellp(i,cellp) \
	do { \
		int k; \
		k = i; \
		assert(k >= 0 && k < NCELL); \
		cellp = &cells[k]; \
	} while (0);

static void throw_err(void)
{
	longjmp(buf, 1);
}

static int p_atom(SEXPR x)
{
	return p_number(x) || p_symbol(x);
}

static SEXPR p_car(SEXPR e)
{
	struct cell *pcell;

	switch (sexpr_type(e)) {
	case SEXPR_CONS:
	// case SEXPR_PROCEDURE:
		cellp(sexpr_index(e), pcell);
		return pcell->car;
	default:
		throw_err();
	}
}

static SEXPR p_cdr(SEXPR e)
{
	struct cell *pcell;

	switch (sexpr_type(e)) {
	case SEXPR_CONS:
	// case SEXPR_PROCEDURE:
		cellp(sexpr_index(e), pcell);
		return pcell->cdr;
	default:
		throw_err();
	}
}

static void p_set_car(SEXPR e, SEXPR val)
{
	if (sexpr_type(e) == SEXPR_CONS) {
		cells[sexpr_index(e)].car = val;
	} else {
		throw_err();
	}
}

static void p_set_cdr(SEXPR e, SEXPR val)
{
	if (sexpr_type(e) == SEXPR_CONS) {
		cells[sexpr_index(e)].cdr = val;
	} else {
		throw_err();
	}
}

/* 
 * List of free cells.
 */
static SEXPR s_free_cells;

static int pop_free_cell()
{
	int i;

	if (p_null(s_free_cells)) {
		gc();
		if (p_null(s_free_cells)) {
			printf("lisep: No more free cells.\n");
			exit(EXIT_FAILURE);
		}
	}

	i = sexpr_index(s_free_cells);
	cells[i].mark = CELL_MARK_CREATED;
	s_free_cells = p_cdr(s_free_cells);
	return i;
}

/* Makes an sexpr form two sexprs. */
static SEXPR p_cons(SEXPR first, SEXPR rest)
{
	int i;
	
	s_cons_car = first;
	s_cons_cdr = rest;

	i = pop_free_cell();
	cells[i].car = first;
	cells[i].cdr = rest;

	s_cons_car = s_nil_atom;
	s_cons_cdr = s_nil_atom;

	return make_cons(i);
}

/* Protect expression form gc by pushin it to s_stack.
 * Returns e. */
static SEXPR push(SEXPR e)
{
	s_stack = p_cons(e, s_stack);
	return e;
}

/* Pop last expression from stack. */
static void pop(void)
{
	assert(!p_null(s_stack));
	s_stack = p_cdr(s_stack);
}

struct builtin {
	const char* id;
	SEXPR (*fun)(SEXPR, SEXPR);
};

static struct builtin builtin_functions[] = {
	{ "atom", &atom },
	{ "car",  &car },
	{ "cdr", &cdr },
	{ "cons", &cons },
	{ "difference", &difference},
	{ "eq", &eq },
	{ "equal", &equal },
	{ "plus", &plus },
	{ "quotient", &quotient },
	{ "times", &times },
};

static struct builtin builtin_special_forms[] = {
	{ "quote", &quote },
	{ "cond", &cond },
	{ "label", &label },
	{ "lambda", &lambda },
	{ "setq", &setq },
};

static SEXPR apply_builtin_function(int i, SEXPR args, SEXPR a)
{
	return builtin_functions[i].fun(args, a);
}

static SEXPR apply_builtin_special_form(int i, SEXPR args, SEXPR a)
{
	return builtin_special_forms[i].fun(args, a);
}

struct input_channel {
	FILE *file;
};

struct input_channel *read_console(struct input_channel *ic)
{
	ic->file = stdin;
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
	t->peekc = -1;
	return t;
}

struct token *pop_token(struct tokenizer *t)
{
	int c;
	int i;
	float n;

again:	if (t->peekc >= 0) {
		c = t->peekc;
		t->peekc = -1;
	} else {
		c = getc_from_channel(t->in);
	}

	if (c == EOF) {
		t->tok.type = EOF;
	} if (c == '(' || c == ')' || c == '.' || c == '\'') {
		t->tok.type = c;
	} else if (isalpha(c)) {
		t->tok.type = T_ATOM;
		i = 0;
		while (isalnum(c)) {
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
	char stack[PARSER_NSTACK];
	SEXPR retval[PARSER_NSTACK];
	int sp;
	int state;
};

struct parser *parse(struct tokenizer *t, struct parser *p)
{
	p->tokenizer = t;
	p->sp = 0;
	p->state = 0;
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

static SEXPR parse_sexpr(struct parser *p, int *iserror);

static SEXPR parse_list(struct parser *p, int *iserror)
{
	struct token *tok;
	SEXPR car, cdr;

	tok = peek_token(p->tokenizer);
	if (tok->type == ')') {
		return s_nil_atom;
	}

	car = parse_sexpr(p, iserror);
	if (*iserror)
		goto error;

	push(car);
	if (tok->type == '.') {
		pop_token(p->tokenizer);
		cdr = parse_sexpr(p, iserror);
	} else {
		cdr = parse_list(p, iserror);
	}
	pop();

	if (*iserror)
		goto error;

	return p_cons(car, cdr);

error:	*iserror = 1;
	return s_nil_atom;
}

static SEXPR parse_quote(struct parser *p, int *iserror)
{
	SEXPR sexpr, e1;

	sexpr = parse_sexpr(p, iserror);
	if (*iserror)
		return s_nil_atom;

	sexpr = push(p_cons(sexpr, s_nil_atom));
	sexpr = p_cons(make_literal(new_literal("quote", 5)), sexpr);
	pop();
	return sexpr;
}

static SEXPR parse_sexpr(struct parser *p, int *iserror)
{
	struct token *tok;
	SEXPR sexpr;
	LITERAL lit;

	tok = peek_token(p->tokenizer);
	if (tok->type == T_ATOM) {	
		lit = new_literal(tok->value.atom.name, tok->value.atom.len);
		sexpr = make_literal(lit);
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
		return parse_quote(p, iserror);
	} else if (tok->type == '(') {
		p->sp++;
		pop_token(p->tokenizer);
		sexpr = parse_list(p, iserror);
		if (*iserror) {
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

error:	*iserror = 1;
	return s_nil_atom;
}

static SEXPR get_sexpr(struct parser *p, int *iserror)
{
	pop_token(p->tokenizer);
	return parse_sexpr(p, iserror);
}

static SEXPR read(void)
{
	struct input_channel ic;
	struct tokenizer t;
	struct parser p;
	SEXPR sexpr;
	int iserror;

again:	printf("lispe> ");
	iserror = 0;
	sexpr = get_sexpr(
			parse(tokenize(read_console(&ic), &t), &p),
			&iserror);
	if (iserror) {
		printf("Syntax error\n");
		goto again;
	} else {
		return sexpr;
	}
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

static SEXPR setq(SEXPR sexpr, SEXPR a)
{
	SEXPR var;
	SEXPR val;
	SEXPR bind;

	var = p_car(sexpr);
	if (!p_symbol(var))
		throw_err();

	val = p_eval(p_car(p_cdr(sexpr)), a);
	bind = p_assoc(var, a);
	if (p_null(bind)) {
		bind = p_assoc(var, s_env);
		if (p_null(bind)) {
			bind = p_cons(var, val);
			s_env = p_cons(bind, s_env);
		} else {
			p_set_cdr(bind, val);
		}
	} else {
		p_set_cdr(bind, val);
	}
	return val;
}

static SEXPR cons(SEXPR e, SEXPR a)
{
	return p_cons(
		p_car(e),
		p_car(p_cdr(e)));
}

static SEXPR plus(SEXPR e, SEXPR a)
{
	return make_number(
		sexpr_number(p_car(e)) +
		sexpr_number(p_car(p_cdr(e))));
}

static SEXPR difference(SEXPR e, SEXPR a) 
{
	return make_number(
		sexpr_number(p_car(e)) -
		sexpr_number(p_car(p_cdr(e))));
}

static SEXPR times(SEXPR e, SEXPR a)
{
	return make_number(
		sexpr_number(p_car(e)) *
		sexpr_number(p_car(p_cdr(e))));
}

static SEXPR quotient(SEXPR e, SEXPR a)
{
	return make_number(
		sexpr_number(p_car(e)) /
		sexpr_number(p_car(p_cdr(e))));
}

static int p_symbol(SEXPR e)
{
	return sexpr_type(e) == SEXPR_LITERAL;
}

static int p_number(SEXPR e)
{
	return sexpr_type(e) == SEXPR_NUMBER;
}

static int p_null(SEXPR e)
{
	return sexpr_type(e) == SEXPR_LITERAL &&
		sexpr_literal(e) == sexpr_literal(s_nil_atom);
}

static SEXPR car(SEXPR e, SEXPR a)
{
	return p_car(p_car(e));
}

static SEXPR cdr(SEXPR e, SEXPR a)
{
	return p_cdr(p_car(e));
}

static SEXPR atom(SEXPR e, SEXPR a)
{
	if (p_atom(p_car(e))) {
		return s_true_atom;
	} else {
		return s_nil_atom;
	}
}

static int p_eq(SEXPR x, SEXPR y)
{
	if (p_null(x) && p_null(y)) {
		return 1;
	} else if (p_symbol(x) && p_symbol(y)) {
		return literals_equal(sexpr_literal(x), sexpr_literal(y));
	} else if (p_number(x) && p_number(y)) {
		return sexpr_number(x) == sexpr_number(y);
	} else {
		throw_err();
	}
}

static SEXPR eq(SEXPR e, SEXPR a)
{
	if (p_eq(p_car(e), p_car(p_cdr(e)))) {
		return s_true_atom;
	} else {
		return s_nil_atom;
	}
}

static SEXPR label(SEXPR e, SEXPR a)
{
	SEXPR name, proc, args, r;

	name = p_car(e);
	if (!p_symbol(name))
		throw_err();
	proc = push(p_eval(p_car(p_cdr(e)), a));
	args = push(p_evlis(p_cdr(p_cdr(e)), a));
	a = push(p_add(name, proc, a)); 
	r = p_apply(proc, args, a); 
	pop(); pop(); pop();
	return r;
	/*
	proc = p_eval(p_car(p_cdr(e)), a);
	args = p_evlis(p_cdr(p_cdr(e)), a);
	return p_apply(proc, args, p_add(name, proc, a)); 
	*/
}

static SEXPR lambda(SEXPR e, SEXPR a)
{
	return make_procedure(sexpr_index(e));
}

static SEXPR p_add(SEXPR var, SEXPR val, SEXPR a)
{
	return p_cons(p_cons(var, val), a);
}

/* If two S-expressions are equal. */
static int p_equal(SEXPR x, SEXPR y)
{
	for (;;) {
		if (p_atom(x)) {
			if (p_atom(y)) {
				return p_eq(x, y);
			} else {
				return 0;
			}
		} else if (p_equal(p_car(x), p_car(y))) {
			x = p_cdr(x);
			y = p_cdr(y);
		} else {
			return 0;
		}
	}
}

static SEXPR equal(SEXPR e, SEXPR a)
{
	if (p_equal(p_car(e), p_car(p_cdr(e))))
		return s_true_atom;
	else
		return s_nil_atom;
}

/* eval each list member.
 * TODO: can be made non-recursive? 
 */
static SEXPR p_evlis(SEXPR m, SEXPR a)
{
	SEXPR e1;

	if (p_null(m)) {
		return m;
	} else {
		e1 = push(p_eval(p_car(m), a));
		e1 = p_cons(e1, p_evlis(p_cdr(m), a));
		pop();
		return e1;
	}
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

/* Given two lists, returns a list with pairs of each list, i.e.:
 * (A B) (C D) -> ((A C) (B D))
 */
static SEXPR p_pairlis(SEXPR x, SEXPR y, SEXPR a)
{
	if (p_null(x)) {
		return a;
	} else {	
		return p_cons(
			p_cons(p_car(x), p_car(y)),
			p_pairlis(p_cdr(x), p_cdr(y), a));
	}
}

/* a is an association list such as the one produced by p_pairlis;
 * returns the first pair whose CAR is x.
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

static SEXPR first_frame(SEXPR env)
{
	return p_car(env);
}

static SEXPR enclosing_environment(SEXPR env)
{
	return p_cdr(env);
}

static SEXPR make_frame(SEXPR vars, SEXPR vals)
{
	return p_cons(vars, vals);
}

static SEXPR frame_variables(SEXPR frame)
{
	return p_car(frame);
}

static SEXPR frame_values(SEXPR frame)
{
	return p_cdr(frame);
}

static void add_binding_to_frame(SEXPR var, SEXPR val, SEXPR frame)
{
	p_set_car(frame, p_cons(var, p_car(frame)));
	p_set_cdr(frame, p_cons(val, p_cdr(frame)));
}

static SEXPR lookup_variable_value(SEXPR var, SEXPR env)
{
	SEXPR frame;
	SEXPR vars;
	SEXPR vals;

	while (!p_null(env)) {
		frame = first_frame(env);
		vars = frame_variables(frame);
		vals = frame_values(frame);
		while (!p_null(vars)) {
			if (p_eq(var, p_car(vars))) {
				return p_car(vals);
			} else {
				vars = p_cdr(vars);
				vals = p_cdr(vals);
			}
		}
		env = enclosing_environment(env);
	}

	throw_err();
	return s_nil_atom;
}

static void set_variable_value(SEXPR var, SEXPR val, SEXPR env)
{
	SEXPR frame;
	SEXPR vars;
	SEXPR vals;

	while (!p_null(env)) {
		frame = first_frame(env);
		vars = frame_variables(frame);
		vals = frame_values(frame);
		while (!p_null(env)) {
			if (p_eq(var, p_car(vars))) {
				p_set_car(vals, val);
				return;
			}
			vars = p_cdr(vars);
			vals = p_cdr(vals);
		}
		env = enclosing_environment(env);
	}

	/* error */
}

static void define_variable(SEXPR var, SEXPR val, SEXPR env)
{
	SEXPR frame;
	SEXPR vars;
	SEXPR vals;

	frame = first_frame(env);
	vars = frame_variables(frame);
	vals = frame_values(frame);
	while (!p_null(env)) {
		if (p_null(vars))
			return;
		if (p_eq(var, p_car(vars))) {
			p_set_car(vals, val);
			return;
		}
		vars = p_cdr(vars);
		vals = p_cdr(vals);
	}
}

static int p_length(SEXPR e)
{
	int n;

	n = 0;
	while (!p_null(e)) {
		n++;
		e = p_cdr(e);
	}
	return n;
}

static SEXPR extend_environment(SEXPR vars, SEXPR vals, SEXPR base_env)
{
	if (p_length(vars) == p_length(vals)) {
		return p_cons(make_frame(vars, vals), base_env);
	}

	/* error */
	return base_env;
}

static SEXPR p_apply(SEXPR fn, SEXPR x, SEXPR a)
{
	SEXPR e1, e2, r;

#if 0
	printf("apply fn: ");
	println(fn);
	printf("x: ");
	println(x);
	printf("a: ");
	println(a);
#endif
	switch (sexpr_type(fn)) {
	case SEXPR_BUILTIN_FUNCTION:
		return apply_builtin_function(sexpr_index(fn), x, a);
	case SEXPR_PROCEDURE:
		e1 = p_car(cells[sexpr_index(fn)].cdr);
		e2 = push(p_pairlis(cells[sexpr_index(fn)].car, x, a));
		r = p_eval(e1, e2);
		pop();
		return r;
	default:
		throw_err();
	}
}

static SEXPR p_eval(SEXPR e, SEXPR a)
{
	SEXPR c, e1;

	switch (sexpr_type(e)) {
	case SEXPR_NUMBER:
		return e;
	case SEXPR_LITERAL:
		c = p_assoc(e, a);
		if (p_null(c)) {
			c = p_assoc(e, s_env);
		}
		c = p_cdr(c);
		return c;
	case SEXPR_CONS:
		printf("eval ");
		println(e);
		printf("a: ");
		println(a);
		push(e);
		push(a);
		c = p_eval(p_car(e), a);
		push(c);
		if (sexpr_type(c) == SEXPR_BUILTIN_SPECIAL_FORM) {
			c = apply_builtin_special_form(
				sexpr_index(c),
				p_cdr(e), a);
		} else {
			e1 = push(p_evlis(p_cdr(e), a));
			c = p_apply(c, e1, a);
			pop();
		}
		pop(); pop(); pop();
		return c;
	default:
		throw_err();
	}
}

static void print(SEXPR sexpr)
{
	int i;

	switch (sexpr_type(sexpr)) {
	case SEXPR_CONS:
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
		printf("%s", literal_name(sexpr_literal(sexpr)));
		break;
	case SEXPR_NUMBER:
		printf("%g", sexpr_number(sexpr));
		break;
	case SEXPR_BUILTIN_FUNCTION:
		printf("{builtin function %s}",
			builtin_functions[sexpr_index(sexpr)].id);
		break;
	case SEXPR_BUILTIN_SPECIAL_FORM:
		printf("{builtin special form %s}",
			builtin_special_forms[sexpr_index(sexpr)].id);
		break;
	case SEXPR_PROCEDURE:
		printf("{lambda}");
#if 0
		printf("(lambda ");
		print(cells[sexpr_index(sexpr)].car);
		print(cells[sexpr_index(sexpr)].cdr);
		printf(")");
#endif
		break;
	}
}

static void println(SEXPR sexpr)
{
	print(sexpr);
	printf("\n");
}

static SEXPR install_builtin(const char *name, SEXPR e, SEXPR a)
{
	LITERAL lit;

	lit = new_literal(name, strlen(name));
	return p_add(make_literal(lit), e, a);
}

static SEXPR install_builtin_functions(SEXPR a)
{
	int i;

	for (i = 0; i < NELEMS(builtin_functions); i++) {
		a = install_builtin(builtin_functions[i].id,
			make_builtin_function(i), a);
	}
	return a;
}

static SEXPR install_builtin_special_forms(SEXPR a)
{
	int i;

	for (i = 0; i < NELEMS(builtin_special_forms); i++) {
		a = install_builtin(builtin_special_forms[i].id,
			make_builtin_special_form(i), a);
	}
	return a;
}

static SEXPR install_literals(SEXPR a)
{
	LITERAL lit;

	a = p_add(s_nil_atom, s_nil_atom, a);

	lit = new_literal("t", 1);
	s_true_atom = make_literal(lit);
	a = p_add(s_true_atom, s_true_atom, a);

	return a;
}

static void make_nil_atom(void)
{
	LITERAL lit;

	lit = new_literal("nil", 3);
	s_nil_atom = make_literal(lit);
}

static void gc_mark(SEXPR e)
{
	struct cell *pcell;

	switch (sexpr_type(e)) {
	case SEXPR_LITERAL:
		gc_mark_literal_used(sexpr_literal(e)); 
		break;
	case SEXPR_CONS:
	case SEXPR_BUILTIN_FUNCTION:
	case SEXPR_BUILTIN_SPECIAL_FORM:
	case SEXPR_PROCEDURE:
		cellp(sexpr_index(e), pcell);
		if (pcell->mark == CELL_MARK_CREATED)
			pcell->mark = CELL_MARK_USED;
		gc_mark(pcell->car);
		gc_mark(pcell->cdr);
		break;
	}
}

static void gc(void)
{
	int i, used, freed;
	SEXPR e;
	struct cell *pcell;

	gc_mark(s_env);
	gc_mark(s_stack);
	gc_mark(s_cons_car);
	gc_mark(s_cons_cdr);

	/* Return to s_free_cells those marked as CELL_MARK_CREATED */
	used = freed = 0;
	for (i = 0; i < NCELL; i++) {
		if (cells[i].mark == CELL_MARK_CREATED) {
			freed++;
			cells[i].mark = CELL_MARK_FREE;
			cells[i].car = s_nil_atom;
			if (p_null(s_free_cells)) {
				cells[i].cdr = s_nil_atom;
			} else {
				cells[i].cdr = make_cons(
						sexpr_index(s_free_cells));
			}
			s_free_cells = make_cons(i);
		} else if (cells[i].mark == CELL_MARK_USED) {
			used++;
			cells[i].mark = CELL_MARK_CREATED;
		}
	}

	gc_literals();

	printf("[used %d, freed %d]\n", used, freed);
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

int main(int argc, char* argv[])
{
	int i;

	make_nil_atom();
	s_stack = s_nil_atom;
	s_cons_car = s_nil_atom;
	s_cons_cdr = s_nil_atom;

	/* link cells for the free cells list */
	cells[NCELL - 1].car = s_nil_atom;
	cells[NCELL - 1].cdr = s_nil_atom;
	for (i = 0; i < NCELL - 1; i++) {
		cells[i].car = s_nil_atom;
		cells[i].cdr = make_cons(i + 1);
	}
	s_free_cells = make_cons(0);

	s_env = s_nil_atom;
	s_env = install_literals(s_env);
	s_env = install_builtin_functions(s_env);
	s_env = install_builtin_special_forms(s_env);

	/* REPL */
	for (;;) {
		if (!setjmp(buf)) {
			println(p_eval(read(), s_nil_atom));
		} else {
			printf("** ERROR **\n");
			s_stack = s_nil_atom;
			s_cons_car = s_nil_atom;
			s_cons_cdr = s_nil_atom;
		}
		// gc();
	}

	return 0;
}
