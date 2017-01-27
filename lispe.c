#include "config.h"
#include "cellmark.h"
#include "common.h"
#include "lex.h"
#include <assert.h>
#ifndef STDIO_H
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

static int s_debug = 0;

static void print(SEXPR sexpr);
static void println(SEXPR sexpr);

static int listlen(SEXPR e);

static SEXPR p_evlis(SEXPR m, SEXPR a);
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

/*********************************************************
 * Exceptions.
 *********************************************************/

static jmp_buf buf; 

void throw_err(void)
{
	longjmp(buf, 1);
}

/*********************************************************
 * Internal predicates and functions, start with p_ 
 *********************************************************/

static int p_eq(SEXPR x, SEXPR y)
{
	if (p_null(x) && p_null(y)) {
		return 1;
	} else if (p_symbolp(x) && p_symbolp(y)) {
		return sexpr_eq(x, y);
	} else if (p_numberp(x) && p_numberp(y)) {
		return sexpr_number(x) == sexpr_number(y);
	} else {
		throw_err();
	}
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
	if (!p_consp(e))
		throw_err();

	set_cell_car(sexpr_index(e), val);
	return val;
}

static SEXPR p_setcdr(SEXPR e, SEXPR val)
{
	if (!p_consp(e))
		throw_err();

	set_cell_cdr(sexpr_index(e), val);
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
	int celli;

	/* TODO: admit any number of arguments */
	e = p_evlis(e, a);
	e = p_car(e);
	switch (sexpr_type(e)) {
	case SEXPR_CLOSURE:
		return cell_car(sexpr_index(e));
	case SEXPR_FUNCTION:
	case SEXPR_SPECIAL:
		celli = sexpr_index(e);
		return p_cons(cell_car(celli), cell_cdr(celli));
	case SEXPR_BUILTIN_FUNCTION:
	case SEXPR_BUILTIN_SPECIAL:
		return s_nil_atom;
	default:
		throw_err();
	}
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
	int celli;

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
		celli = sexpr_index(fn);
		/* pair parameters with their arguments and append 'a. */
		a = p_pairargs(cell_car(celli), x, a);
		/* eval sequence except the last expression */
		e1 = cell_cdr(celli);
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
	int celli;

	s_evalc++;
#if 0
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
				celli = sexpr_index(c);
				c = lambda(cell_car(celli), a);
				a = cell_cdr(celli);
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
		print(cell_car(i));
		while (!p_null(cell_cdr(i))) {
			sexpr = cell_cdr(i);
			if (sexpr_type(sexpr) == SEXPR_CONS) {
				i = sexpr_index(sexpr);
				printf(" ");
				print(cell_car(i));
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
		print(cell_car(sexpr_index(sexpr)));
		print(cell_cdr(sexpr_index(sexpr))));
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

	cells_init();
	cellmark_init();
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
				println(p_eval(e, s_nil_atom));
			}
		} else {
			printf("lispe: ** error **\n");
			clear_stack();
		}
		// assert(p_equal(s_stack, s_nil_atom));
	}

	return 0;
}
