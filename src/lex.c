#include "lex.h"
#include <string.h>
#include <ctype.h>
#ifndef STDIO_H
#include <stdio.h>
#endif

/************************************************************/
/* input channels                                           */
/************************************************************/

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

/************************************************************/
/* tokenizer for s-expressions                              */
/************************************************************/

struct tokenizer *tokenize(struct input_channel *ic, struct tokenizer *t)
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
