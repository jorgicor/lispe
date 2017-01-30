#include "cfg.h"
#include "cbase.h"
#include "gc.h"
#ifndef SEXPR_H
#include "sexpr.h"
#endif
#include "cells.h"
#include "cellmark.h"
#include "numbers.h"
#include "symbols.h"
#include "common.h"
#include "lex.h"
#include <assert.h>
#ifndef STDIO_H
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

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
static SEXPR dyn_lambda(SEXPR e, SEXPR a);
static SEXPR special(SEXPR e, SEXPR a);
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

/*********************************************************
 * Exceptions.
 *********************************************************/

static jmp_buf buf; 

void throw_err(void)
{
	longjmp(buf, 1);
}

/*********************************************************/

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
	{ "dyn-lambda", &dyn_lambda },
	{ "list", &list },
	{ "quote", &quote },
	{ "setq", &setq },
	{ "special", &special },
};

static void install_builtin(const char *name, SEXPR e)
{
	SEXPR var;

	var = make_symbol(name, strlen(name));
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

static SEXPR apply_builtin(struct builtin *pbin, SEXPR args, SEXPR a)
{
	return pbin->fun(args, a);
}

SEXPR apply_builtin_function(int i, SEXPR args, SEXPR a)
{
	assert(i >= 0 && i < NELEMS(builtin_functions));
	return apply_builtin(&builtin_functions[i], args, a);
}

SEXPR apply_builtin_special(int i, SEXPR args, SEXPR a)
{
	assert(i >= 0 && i < NELEMS(builtin_specials));
	return apply_builtin(&builtin_specials[i], args, a);
}

static const char *builtin_name(struct builtin *pbin)
{
	return pbin->id;
}

const char *builtin_function_name(int i)
{
	assert(i >= 0 && i < NELEMS(builtin_functions));
	return builtin_name(&builtin_functions[i]);
}

const char *builtin_special_name(int i)
{
	assert(i >= 0 && i < NELEMS(builtin_specials));
	return builtin_name(&builtin_specials[i]);
}

/*********************************************************/

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
			p_eval(sexpr, SEXPR_NIL); 
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

	val = SEXPR_NIL;
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
			return SEXPR_NIL;
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
	return p_atom(p_car(e)) ? s_true_atom : SEXPR_NIL;
}

static SEXPR symbolp(SEXPR e, SEXPR a)
{
	return p_symbolp(p_car(e)) ? s_true_atom : SEXPR_NIL;
}

static SEXPR consp(SEXPR e, SEXPR a)
{
	return p_consp(p_car(e)) ? s_true_atom : SEXPR_NIL;
}

static SEXPR numberp(SEXPR e, SEXPR a)
{
	return p_numberp(p_car(e)) ? s_true_atom : SEXPR_NIL;
}

static SEXPR null(SEXPR e, SEXPR a)
{
	return p_null(p_car(e)) ? s_true_atom : SEXPR_NIL;
}

static SEXPR eq(SEXPR e, SEXPR a)
{
	return p_eq(p_car(e), p_car(p_cdr(e))) ? s_true_atom : SEXPR_NIL;
}

static SEXPR special(SEXPR e, SEXPR a)
{
	return make_special(sexpr_index(e));
	// return make_special(sexpr_index(p_cons(e, a)));
}

static SEXPR lambda(SEXPR e, SEXPR a)
{
	/* e is the arguments and body */
	return make_function(sexpr_index(p_cons(e, a)));
}

static SEXPR dyn_lambda(SEXPR e, SEXPR a)
{
	return make_dyn_function(sexpr_index(e));
}

/* TODO: change for symbol-function and print like (lambda (x) (nc ...)) */
static SEXPR body(SEXPR e, SEXPR a)
{
	int celli;

	/* TODO: admit any number of arguments */
	e = p_evlis(e, a);
	e = p_car(e);
	switch (sexpr_type(e)) {
	case SEXPR_FUNCTION:
		return cell_car(sexpr_index(e));
	case SEXPR_DYN_FUNCTION:
	case SEXPR_SPECIAL:
		celli = sexpr_index(e);
		return p_cons(cell_car(celli), cell_cdr(celli));
	case SEXPR_BUILTIN_FUNCTION:
	case SEXPR_BUILTIN_SPECIAL:
		return SEXPR_NIL;
	default:
		throw_err();
		return SEXPR_NIL;
	}
}

static SEXPR equal(SEXPR e, SEXPR a)
{
	if (p_equal(p_car(e), p_car(p_cdr(e))))
		return s_true_atom;
	else
		return SEXPR_NIL;
}

static SEXPR eval(SEXPR e, SEXPR a)
{
	e = p_eval(p_car(e), a);
	e = p_eval(e, a);
	return e;
}

static SEXPR gc(SEXPR e, SEXPR a)
{
	p_gc();
	return SEXPR_NIL;
}

int main(int argc, char* argv[])
{
	int errorc;
	SEXPR e;

	printf("lispe minimal lisp 1.0\n\n");

	cells_init();
	cellmark_init();
	init_numbers();
	init_symbols();
	gcbase_init();

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
				p_println(p_eval(e, SEXPR_NIL));
			}
		} else {
			printf("lispe: ** error **\n");
			clear_stack();
		}
		assert(stack_empty());
	}

	return 0;
}
