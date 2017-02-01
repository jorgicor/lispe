#ifndef COMMON_H
#define COMMON_H

/* pred.c */

int p_null(SEXPR e);
int p_atom(SEXPR x);
int p_consp(SEXPR e);
int p_symbolp(SEXPR e);
int p_numberp(SEXPR e);
SEXPR p_car(SEXPR e);
SEXPR p_cdr(SEXPR e);
int p_eq(SEXPR x, SEXPR y);
int p_equal(SEXPR x, SEXPR y);
SEXPR p_setcar(SEXPR e, SEXPR val);
SEXPR p_setcdr(SEXPR e, SEXPR val);
SEXPR p_assoc(SEXPR x, SEXPR a);
SEXPR p_evcon(SEXPR c, SEXPR a);
SEXPR p_evlis(SEXPR m, SEXPR a);
SEXPR p_eval(SEXPR e, SEXPR a);
SEXPR p_add(SEXPR var, SEXPR val, SEXPR a);

void p_print(SEXPR sexpr);
void p_println(SEXPR sexpr);
void p_println_env(SEXPR env);

/* gcbase.c */

extern SEXPR s_env;
extern SEXPR s_true_atom;
extern SEXPR s_quote_atom;
extern SEXPR s_rest_atom;

int pop_free_cell(void);
SEXPR p_cons(SEXPR first, SEXPR rest);

void clear_stack(void);
SEXPR push(SEXPR e);
void push2(SEXPR e1, SEXPR e2);
void push3(SEXPR e1, SEXPR e2, SEXPR e3);
void pop(void);
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
SEXPR define_variable(SEXPR var, SEXPR val, SEXPR env);
SEXPR set_variable(SEXPR var, SEXPR val, SEXPR env);
void extend_environment(SEXPR env, SEXPR params, SEXPR args);

/* lispe.c */

void throw_err(void);
SEXPR apply_builtin_function(int i, SEXPR args, SEXPR a);
SEXPR apply_builtin_special(int i, SEXPR args, SEXPR a);
const char *builtin_function_name(int i);
const char *builtin_special_name(int i);
int builtin_special_tailrec(int i);
int builtin_function_tailrec(int i);

#endif
