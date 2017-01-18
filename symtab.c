#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define USE_TREE 0

#if USE_TREE

struct literal {
	char *name;
	struct literal *left;
	struct literal *right;
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
		pnode->name = (unsigned char *) pnode + sizeof(*pnode);
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

LITERAL new_literal(const char *s, int len)
{
	char *p;

	p = malloc(len + 1);
	if (p == NULL) {
		fprintf(stderr, "lispe: error: out of memory!\n");
		exit(EXIT_FAILURE);
	}
	memcpy(p, s, len);
	p[len] = '\0';
	return p;
}

const char *literal_name(LITERAL lit)
{
	return lit;
}

int literals_equal(LITERAL lita, LITERAL litb)
{
	return strcmp(lita, litb) == 0;
}

#endif
