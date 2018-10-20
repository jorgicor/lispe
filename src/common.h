/* ===========================================================================
 * lispe, Scheme interpreter.
 * ===========================================================================
 */

#ifndef COMMON_H
#define COMMON_H

/* pred.c */

int s_tailrec;

int p_nullp(SEXPR e);
int p_pairp(SEXPR e);
int p_symbolp(SEXPR e);
int p_numberp(SEXPR e);
int p_complexp(SEXPR e);
int p_realp(SEXPR e);
int p_integerp(SEXPR e);
int p_exactp(SEXPR e);
SEXPR p_car(SEXPR e);
SEXPR p_cdr(SEXPR e);
int p_eqp(SEXPR x, SEXPR y);
int p_eqvp(SEXPR x, SEXPR y);
int p_equalp(SEXPR x, SEXPR y);
SEXPR p_setcar(SEXPR e, SEXPR val);
SEXPR p_setcdr(SEXPR e, SEXPR val);
void p_evlis(void);
void p_eval(void);
void p_evargs(void);
void p_evseq(int eval_last);
void p_apply(void);

void p_print(SEXPR sexpr);
void p_println(SEXPR sexpr);
void p_println_env(SEXPR env);

/* gcbase.c */

extern SEXPR s_topenv;
extern SEXPR s_env;
extern SEXPR s_quote_atom;
extern SEXPR s_expr;
extern SEXPR s_val;
extern SEXPR s_proc;
extern SEXPR s_args;
extern SEXPR s_unev;

int pop_free_cell(void);
SEXPR p_cons(SEXPR first, SEXPR rest);

void clear_stack(void);
SEXPR push(SEXPR e);
void push2(SEXPR e1, SEXPR e2);
void push3(SEXPR e1, SEXPR e2, SEXPR e3);
SEXPR pop(void);
void popn(int n);
int stack_empty(void);

void gcbase_init(void);

/* parse.c */

struct parser {
	struct tokenizer *tokenizer;
	int sp;
};

struct parser *parse(struct tokenizer *t, struct parser *p);

enum {
	ERRORC_OK,
	ERRORC_EOF,
	ERRORC_SYNTAX,
};

SEXPR get_sexpr(struct parser *p, int *errorc);

/* env.c */

SEXPR make_environment(SEXPR parent);
SEXPR lookup_variable(SEXPR var, SEXPR env);
void define_variable(void);
SEXPR set_variable(SEXPR var, SEXPR val, SEXPR env);
void extend_environment(void);

/* lispe.c */

void apply_builtin_function(int i);
void apply_builtin_special(int i);
const char *builtin_function_name(int i);
const char *builtin_special_name(int i);
int builtin_special_tailrec(int i);
int builtin_function_tailrec(int i);

#endif
