/*
Copyright (c) 2017 Jorge Giner Cordero

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

void op_run(void)
{
	while (op != NULL)
		(*op)();
}

/* in: s_proc, s_args, s_alist
 * out: s_val
 */
void op_apply(void)
{
	switch (sexpr_type(s_proc)) {
	case SEXPR_BUILTIN_FUNCTION:
		s_val = apply_builtin_function(sexpr_index(s_proc), s_args,
					       s_alist);
		op = s_cont;
		break;
	case SEXPR_BUILTIN_SPECIAL:
		s_val = apply_builtin_special(sexpr_index(s_proc), s_args,
					      s_alist);
		op = s_cont;
		break;
	case SEXPR_FUNCTION:
	case SEXPR_SPECIAL:
		push(s_cont);
		push(s_alist);
		cellp(sexpr_index(s_proc), pcell);
		s_unev = pcell->cdr;
		push(s_unev);
		s_params = pcell->car;
		s_cont = op_eval_sequence;
		op = op_pairargs;
		/*
		s_alist = p_pairargs(pcell->car, s_args, s_alist);
		e1 = pcell->cdr;
		while (!p_null(e1)) {
			r = p_eval(p_car(e1), a);
			e1 = p_cdr(e1);
		}
		pop();
		*/
		break;
	default:
		throw_err();
	}
}

/* in: s_params, s_args, s_alist */
void op_pairargs(void)
{
	if (p_null(p_params)) {
		op = s_cont;
		return;
	}
}

void op_eval_sequence(void)
{
	s_unev = pop();
	if (p_null(s_unev)) {
		s_alist = pop();
		s_cont = pop();
		op = s_cont;
	} else {
		s_expr = p_car(s_unev);
		s_unev = p_cdr(s_unev);
		push(s_unev);
		op = op_eval;
	}
}

enum {
	OP_END,
	OP_EVAL,
	OP_EVAL_CONS_CAR,
	OP_EVAL_AFTER_EVLIS,
	OP_EVLIS,
	OP_EVLIS_FIRST,
	OP_EVLIS_LOOP,
	OP_APPLY,
};

/* 
 * in: s_expr, s_alist
 * out: s_val
 */
void exec*******(int op)
{
	int t;

	s_cont = OP_END;
	switch (op) {
	
	case OP_END:

		return;

	case OP_EVAL:

	t = sexpr_type(s_expr);
	if (t == SEXPR_NUMBER) {
		s_val = s_expr;
		goto s_cont;
	} else if (t == SEXPR_LITERAL) {
		s_val = p_assoc(s_expr, s_alist);
		if (p_null(s_val)) {
			s_val = p_assoc(s_expr, s_env);
		}
		s_val = p_cdr(s_val);
		goto s_cont;
	} else if (t == SEXPR_CONS) {
		push(s_cont);
		s_unev = p_cdr(s_expr);
		push(s_unev);
		s_expr = p_car(s_expr);
		s_cont = OP_EVAL_CONS_CAR;
		goto OP_EVAL;
		case OP_EVAL_CONS_CAR:
		s_unev = pop();
		s_proc = s_val;
		t = sexpr_type(s_proc);
		if (t == SEXPR_BUILTIN_SPECIAL || t == SEXPR_SPECIAL) {
			s_args = s_unev;
			s_cont = pop();
			goto OP_APPLY;
		} else if (t == SEXPR_CLOSURE) {
			push(s_proc);
			s_cont = OP_EVAL_CLOSURE_AFTER_EVIS;
			goto OP_EVLIS;
			case OP_EVAL_CLOSURE_AFTER_EVLIS;
			s_proc = pop();
			cellp(sexpr_index(s_proc), pcell);
			s_proc = lambda(pcell->car, s_alist);
			push(s_alist);
			s_alist = pcell->cdr;
			s_cont = OP_EVAL_AFTER_APPLY_CLOSURE;
			goto OP_APPLY;
			case OP_EVAL_AFTER_APPLY_CLOSURE:
			s_alist = pop();
			s_cont = pop();
			goto s_cont;
		} else {
			push(s_proc);
			s_cont = OP_EVAL_AFTER_EVLIS;
			goto OP_EVLIS;
			case OP_EVAL_AFTER_EVLIS:
			s_proc = pop();
			s_cont = pop();
			goto OP_APPLY;
		}
	} else {
		throw_err();
	}

	case OP_EVLIS:

	s_args = s_nil_atom;
	if (p_null(s_unev))
		goto s_cont;

	push(s_cont);
	s_expr = p_car(s_unev);
	s_unev = p_cdr(s_unev);
	push(s_unev);
	s_cont = OP_EVLIS_FIRST;
	goto OP_EVAL;
	case OP_EVLIS_FIRST:
	s_unev = pop();
	s_args = p_cons(s_val, s_nil_atom);
	s_argnode = s_args;
	if (p_null(s_unev)) {
		s_cont = pop();
		goto s_cont;
	} else {
		push(s_args);
		push(s_argnode);
		s_expr = p_car(s_unev);
		s_unev = p_cdr(s_unev);
		push(s_unev);
		s_cont = OP_EVLIS_LOOP;
		goto OP_EVAL;
		case OP_EVLIS_LOOP:
		s_unev = pop();
		s_argnode = pop();
		s_val = p_cons(s_val, s_nil_atom);
		p_setcdr(s_argnode, s_val);
		s_argnode = s_val;
		if (p_null(s_unev)) {
			s_args = pop();
			s_cont = pop();
			goto s_cont;
		} else {
			push(s_argnode);
			s_expr = p_car(s_unev);
			s_unev = p_cdr(s_unev);
			push(s_unev);
			goto OP_EVAL;
		}
	}

	case OP_APPLY:

	t = sexpr_type(s_proc);
	if ( t == SEXPR_BUILTIN_FUNCTION) {
		s_val = apply_builtin_function(sexpr_index(s_proc), s_args,
					       s_alist);
		goto s_cont;
	} else if (t == SEXPR_BUILTIN_SPECIAL) {
		s_val = apply_builtin_special(sexpr_index(s_proc), s_args,
					      s_alist);
		goto s_cont;
	} else if (t == SEXPR_FUNCTION || t == SEXPR_SPECIAL) {
		push(s_cont);
		push(s_alist);
		cellp(sexpr_index(s_proc), pcell);
		s_unev = pcell->cdr;
		push(s_unev);
		s_params = pcell->car;
		s_cont = OP_EVAL_SEQUENCE;
		/* PAIRARGS */
		/* s_alist = p_pairargs(pcell->car, s_args, s_alist); */
		case OP_EVAL_SEQUENCE:
		s_unev = pop();
		if (p_null(s_unev)) {
			s_alist = pop();
			s_cont = pop();
			goto s_cont;
		} else {
			s_expr = p_car(s_unev);
			s_unev = p_cdr(s_unev);
			push(s_unev);
			goto OP_EVAL;
		}
	} else {
		throw_err();
	}

	default:
		throw_err();
	}
}

/* in: s_expr, s_alist, s_cont
 * out: s_val
 */
void op_eval(void)
{
	switch (sexpr_type(s_expr)) {
	case SEXPR_NUMBER:
		s_val = s_expr;
		s_op = s_cont;
		break;
	case SEXPR_LITERAL:
		s_val = p_assoc(s_expr, s_alist);
		if (p_null(s_val)) {
			s_val = p_assoc(s_expr, s_env);
		}
		s_val = p_cdr(s_val);
		s_op = s_cont;
		break;
	case SEXPR_CONS:
		push(s_cont);
		s_unev = p_cdr(s_expr);
		push(s_unev);
		s_expr = p_car(s_expr);
		s_cont = op_eval_cons_car;
		op = eval;
		break;
	default:
		throw_err();
	}
}

/* in: s_val (evaluated operator) */
void op_eval_cons_car(void)
{
	s_unev = pop();
	s_proc = s_val;
	switch (sexpr_type(s_proc)) {
	case SEXPR_BUILTIN_SPECIAL:
	case SEXPR_SPECIAL:
		s_args = s_unev;
		s_cont = pop();
		op = op_apply;
		return;
	case SEXPR_CLOSURE:
		push(s_proc);
		s_op = op_evlis;
		s_cont = op_eval_closure_after_evlis;
		break;
	default:
		push(s_proc);
		s_op = op_evlis;
		s_cont = op_eval_after_evlis;
	}
}

/* in: s_args */
void op_eval_after_evlis(void)
{
	s_proc = pop();
	s_cont = pop();
	op = op_apply;
}

/* in: s_args */
void op_eval_closure_after_evlis(void)
{
	s_proc = pop();
	cellp(sexpr_index(s_proc), pcell);
	s_proc = lambda(pcell->car, s_alist);
	push(s_alist);
	s_alist = pcell->cdr;
	s_cont = op_eval_after_apply_closure;
	op = op_apply;
}

/* A closure has been applied, s_alist was on the stack... */
void op_eval_after_apply_closure(void)
{
	s_alist = pop();
	s_cont = pop();
	op = s_cont;
}

/* in: s_unev, s_alist
 * out: s_args
 */
void op_evlis(void)
{
	s_args = s_nil_atom;
	if (p_null(s_unev)) {
		op = s_cont;
		return;
	}

	push(s_cont);
	s_expr = p_car(s_unev);
	s_unev = p_cdr(s_unev);
	push(s_unev);
	s_cont = op_evlis_first;
	op = op_eval;
}

void op_evlis_first(void)
{
	s_unev = pop();
	s_args = p_cons(s_val, s_nil_atom);
	s_argnode = s_args;
	if (p_null(s_unev)) {
		s_cont = pop();
		s_op = s_cont;
	} else {
		push(s_args);
		push(s_argnode);
		s_expr = p_car(s_unev);
		s_unev = p_cdr(s_unev);
		push(s_unev);
		s_cont = op_evlis_loop;
		s_op = op_eval;
	}
}

void op_evlis_loop(void)
{
	s_unev = pop();
	s_argnode = pop();
	s_val = p_cons(s_val, s_nil_atom);
	p_setcdr(s_argnode, s_val);
	s_argnode = s_val;
	if (p_null(s_unev)) {
		s_args = pop();
		s_cont = pop();
		s_op = s_cont;
	} else {
		push(s_argnode);
		s_expr = p_car(s_unev);
		s_unev = p_cdr(s_unev);
		push(s_unev);
		s_op = op_eval;
	}
}

#if 0
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
	p_setcar(frame, p_cons(var, p_car(frame)));
	p_setcdr(frame, p_cons(val, p_cdr(frame)));
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
				p_setcar(vals, val);
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
			p_setcar(vals, val);
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
#endif
