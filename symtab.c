#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

enum {
	MARK_UNASSIGNED,
	MARK_ASSIGNED,
	MARK_USED,
};

#define USE_TREE 0

#if USE_TREE

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

#else

struct literal {
	struct literal *next;
	char mark_n_name[];
};

static struct literal *s_list;

#define literal_mark(lit) (lit)->mark_n_name[0]

#define mark_literal(lit, mark) literal_mark(lit) = mark

void gc_mark_literal_assigned(LITERAL lit)
{
	mark_literal(lit, MARK_ASSIGNED);
}

void gc_mark_literal_used(LITERAL lit)
{
	mark_literal(lit, MARK_USED);
}

void gc_literals(void)
{
	struct literal **pplit, *pnext;
	int freed;

	freed = 0;
	for (pplit = &s_list; *pplit != NULL; pplit = &(*pplit)->next) {
		switch (literal_mark(*pplit)) {
		case MARK_USED:
			mark_literal(*pplit, MARK_ASSIGNED);
			break;
		case MARK_ASSIGNED:
			freed++;
			pnext = (*pplit)->next;
			free(*pplit);
			*pplit = pnext;
			break;
		}
	}

	printf("[freed literals %d]\n", freed);
}

LITERAL new_literal(const char *s, int len)
{
	struct literal *plit;

	plit = malloc(sizeof(*plit) + 1 + len + 1);
	if (plit == NULL) {
		fprintf(stderr, "lispe: Out of memory!\n");
		exit(EXIT_FAILURE);
	}

	memcpy(plit->mark_n_name + 1, s, len);
	plit->mark_n_name[len + 1] = '\0';

	mark_literal(plit, MARK_UNASSIGNED);

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
