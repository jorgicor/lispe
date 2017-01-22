#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

enum {
	LITERAL_CREATED,
	LITERAL_USED
};

/* #define USE_DUP_LITERALS */
/* #define USE_TREE */
#define USE_HASH

#ifdef USE_HASH

/* prime */
enum { NCHAINS = 16381 };

/*
 * mark_n_name[0] contains the mark.
 * &mark_n_name[1] contains the null terminated literal name.
 */
struct literal {
	struct literal *next;
	char mark_n_name[];
};

/* Each node here is a header, only 'next is valid.
 * We can use it because sizeof(struct literal) == sizeof(struct literal *)
 * so we don't waste space actually. */
struct literal s_hashtab[NCHAINS];

#define literal_mark(lit) (lit)->mark_n_name[0]

#define mark_literal(lit, mark) literal_mark(lit) = mark

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
unsigned int hash(const char *p, size_t len)
{
	unsigned int h;

	h = 0;
	while (len--) {
		h = 31 * h + (unsigned char) *p;
		p++;
	}

	return h;
}

LITERAL new_literal(const char *s, int len)
{
	unsigned int h;
	struct literal *p;

	h = hash(s, len) % NCHAINS;
	for (p = s_hashtab[h].next; p != NULL; p = p->next) {
		if (cmpstrlen(s, len, literal_name(p)) == 0)
			return p;
	}

	p = malloc(sizeof(*p) +  1 + len + 1);
	if (p == NULL) {
		fprintf(stderr, "lispe: out of memory\n");
		exit(EXIT_FAILURE);
	}

	memcpy(p->mark_n_name + 1, s, len);
	p->mark_n_name[len + 1] = '\0';
	mark_literal(p, LITERAL_CREATED);

	p->next = s_hashtab[h].next;
	s_hashtab[h].next = p;
	return p;
}

const char *literal_name(LITERAL lit)
{
	return lit->mark_n_name + 1;
}

int literals_equal(LITERAL lita, LITERAL litb)
{
	return lita == litb;
}

void gc_mark_literal_used(LITERAL lit)
{
	mark_literal(lit, LITERAL_USED);
}

void gc_literals(void)
{
	int i, freed;
	struct literal *prev, *p, *q;

	freed = 0;
	for (i = 0; i < NCHAINS; i++) {
		for (prev = &s_hashtab[i], p = prev->next;
		     p != NULL;
		     prev = p, p = prev->next)
		{
			if (literal_mark(p) == LITERAL_USED) {
				mark_literal(p, LITERAL_CREATED);
			} else {
				freed++;
				printf("freed (%s)\n", literal_name(p));
				prev->next = p->next;
				free(p);
				p = prev;
			}
		}
	}

	printf("[freed literals %d]\n", freed);
}

#endif

#ifdef USE_TREE

/* To gc:
 * 1. Keep track of the number of nodes of the tree (n);
 * 2. Mark the tree nodes and count how many used (m).
 * 3. Alloc an array (a) of size (m) of pointers to literal to point to each
 * node of the tree that is marked and another (b) of size (n - m) to point to
 * those not marked.
 * 4. Trasverse the tree in order and fill the array (a) with the marked nodes
 * and the array (b) with those not marked.
 * 5. Free the nodes in (b) and free (b).
 * 6. Rebuild the tree from (a) and free (a). The tree will be balanced.
 */

struct literal {
	struct literal *left;
	struct literal *right;
	char name[];
};

/* We use a simple binary tree for now... */
static struct literal *s_head;

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

LITERAL new_literal(const char *s, int len)
{
	int insert, r;
	struct literal *pnode, *par;

	assert(len > 0);

	insert = 1;
	par = NULL;
	pnode = s_head;
	while (pnode) {
		par = pnode;
		r = cmpstrlen(s, len, pnode->name);
		if (r < 0)
		       	pnode = pnode->left;
		else if (r > 0)
			pnode = pnode->right;
		else
			return pnode;
	}
	
	if (insert) {
		/* TODO: check this size for overflow */
		pnode = malloc(sizeof(*pnode) + len + 1);
		if (pnode == NULL) {
			fprintf(stderr, "lispe: error: out of memory!\n");
			exit(EXIT_FAILURE);
		}
		memcpy(pnode->name, s, len);
		pnode->name[len] = '\0';
		pnode->left = NULL;
		pnode->right = NULL;
		if (par == NULL)
			s_head = pnode;
		else if (r < 0)
			par->left = pnode;
		else
			par->right = pnode;
	}

	return pnode;
}

const char *literal_name(LITERAL lit)
{
	return lit->name;
}

int literals_equal(LITERAL lita, LITERAL litb)
{
	return lita == litb;
}

#endif

#ifdef USE_DUP_LITERALS

struct literal {
	struct literal *next;
	char mark_n_name[];
};

static struct literal *s_list;

#define literal_mark(lit) (lit)->mark_n_name[0]

#define mark_literal(lit, mark) literal_mark(lit) = mark

void gc_mark_literal_used(LITERAL lit)
{
	mark_literal(lit, LITERAL_USED);
}

void gc_literals(void)
{
	struct literal *pnode, *prev, *q;
	int freed;

	freed = 0;
	prev = NULL;
	pnode = s_list;
	while (pnode) {
		if (literal_mark(pnode) == LITERAL_USED) {
			mark_literal(pnode, LITERAL_CREATED);
			prev = pnode;
			pnode = pnode->next;
		} else {
			freed++;
			printf("freed (%s)\n", literal_name(pnode));
			if (prev == NULL)
				s_list = pnode->next;
			else
				prev->next = pnode->next;
			q = pnode->next;
			free(pnode);
			pnode = q;
		}
	}

#if 0
	struct literal **pplit, *pnext;
	int freed;

	freed = 0;
	for (pplit = &s_list; *pplit != NULL; pplit = &(*pplit)->next) {
		switch (literal_mark(*pplit)) {
		case LITERAL_USED:
			mark_literal(*pplit, LITERAL_CREATED);
			break;
		case LITERAL_CREATED:
			freed++;
			pnext = (*pplit)->next;
			printf("freed (%s)\n", literal_name(*pplit));
			free(*pplit);
			*pplit = pnext;
			break;
		}
	}
#endif

	printf("[freed literals %d]\n", freed);
}

LITERAL new_literal(const char *s, int len)
{
	struct literal *plit;

	plit = malloc(sizeof(*plit) + 1 + len + 1);
	if (plit == NULL) {
		fprintf(stderr, "lispe: out of memory\n");
		exit(EXIT_FAILURE);
	}

	memcpy(plit->mark_n_name + 1, s, len);
	plit->mark_n_name[len + 1] = '\0';

	mark_literal(plit, LITERAL_CREATED);

	plit->next = s_list;
	s_list = plit;
	return plit;
}

const char *literal_name(LITERAL lit)
{
	return lit->mark_n_name + 1;
}

int literals_equal(LITERAL lita, LITERAL litb)
{
	return strcmp(literal_name(lita), literal_name(litb)) == 0;
}

#endif
