#include "config.h"
#include "common.h"
#include "lex.h"

enum {
	PARSER_NSTACK = 1024
};

#if 0
struct parser {
	struct tokenizer *tokenizer;
	// char stack[PARSER_NSTACK];
	// SEXPR retval[PARSER_NSTACK];
	int sp;
	// int state;
};
#endif

/************************************************************/
/* parser for s-expressions                                 */
/************************************************************/

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
	case 1:			if (is_SEXPR_NIL(head)) {
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
		return SEXPR_NIL;
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
	return SEXPR_NIL;
}

static SEXPR parse_quote(struct parser *p, int *errorc)
{
	SEXPR sexpr;

	sexpr = parse_sexpr(p, errorc);
	if (*errorc != ERRORC_OK)
		return SEXPR_NIL;

	sexpr = p_cons(sexpr, SEXPR_NIL);
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
		return SEXPR_NIL;
	} else if (tok->type == T_ATOM) {	
		sexpr = make_symbol(tok->value.atom.name,
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
	return SEXPR_NIL;
}

SEXPR get_sexpr(struct parser *p, int *errorc)
{
	*errorc = ERRORC_OK;
	pop_token(p->tokenizer);
	return parse_sexpr(p, errorc);
}

