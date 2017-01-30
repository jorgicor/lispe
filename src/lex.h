#ifndef LEX_H
#define LEX_H

#ifndef STDIO_H
#define STDIO_H
#include <stdio.h>
#endif

struct input_channel {
	FILE *file;
};

struct input_channel *read_console(struct input_channel *ic);
struct input_channel *read_file(struct input_channel *ic, FILE *fp);
int getc_from_channel(struct input_channel *ic);

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

struct tokenizer *tokenize(struct input_channel *ic, struct tokenizer *t);
struct token *pop_token(struct tokenizer *t);
struct token *peek_token(struct tokenizer *t);

#endif
