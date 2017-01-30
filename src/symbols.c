#include "cfg.h"
#include "cbase.h"
#include "gc.h"
#include "cbase.h"
#include <assert.h>
#ifndef STDIO_H
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG_SYMBOLS
#define dprintf(...) printf(__VA_ARGS__) 
#else
#define dprintf(...)
#endif

enum { NSYMBOLS = NCELL };

struct symbol_head {
	int next;
};

struct symbol_node {
	int next;
	char *name;
};

static struct symbol_node s_symbols[NSYMBOLS];
static struct symbol_head s_free_nodes;

static const int s_primes[] = {
	53, 97, 193, 389, 769,
	1543, 3079, 6151, 12289, 24593,
       	49157, 98387, 196613, 393241, 786433, 
	1572869, 3145739, 6291469, 12582917, 25165843,
	50331653, 100663319, 201326611, 402653189, 805306457,
	1610612741	
};

static struct symbol_head *s_hashtab;
static int s_hashtab_size;

enum { NSYM_MARKS = (NSYMBOLS / 32) + ((NSYMBOLS % 32) ? 1 : 0) };

static unsigned int s_sym_marks[NSYM_MARKS];

#ifdef PP_RANGECHECKS
#define check_sloti(i) assert(i >= 0 && i < NSYMBOLS)
#define check_marki(i) assert(i >= 0 && i < NSYM_MARKS)
#else
#define check_sloti(i)
#define check_marki(i)
#endif

void mark_symbol(int i)
{
	int w;

	// printf("marked %d\n", i);
	chkrange(i, NSYMBOLS);
	w = i >> 5;
	chkrange(w, NSYM_MARKS);
	i &= 31;
	s_sym_marks[w] |= (1 << i);
}

static int symbol_marked(int i)
{
	int w;

	chkrange(i, NSYMBOLS);
	w = i >> 5;
	chkrange(w, NSYM_MARKS);
	i &= 31;
	return s_sym_marks[w] & (1 << i);
}

void gc_symbols(void)
{
	int h, prev, si;
	int nused;

	nused = 0;
	for (h = 0; h < s_hashtab_size; h++) {
		prev = -1;
		si = s_hashtab[h].next;
		while (si >= 0) {
			if (!symbol_marked(si)) {
				dprintf("freed %s\n", s_symbols[si].name);
				free(s_symbols[si].name);
				s_symbols[si].name = NULL;
				if (prev == -1) {
					s_hashtab[h].next = s_symbols[si].next;
				} else {
					s_symbols[prev].next =
					       	s_symbols[si].next; 
				}
				s_symbols[si].next = s_free_nodes.next;
				s_free_nodes.next = si;
				if (prev == -1) {
					si = s_hashtab[h].next;
				} else {
					si = s_symbols[prev].next;
				}
			} else {
				nused++;
				prev = si;
				si = s_symbols[prev].next;
			}
		}
	}

	memset(s_sym_marks, 0, sizeof(s_sym_marks));
	printf("[gc: %d/%d symbols]\n", nused, NSYMBOLS);
}

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
static unsigned int hash(const char *p, size_t len)
{
	unsigned int h;

	h = 0;
	while (len--) {
		h = 31 * h + (unsigned char) *p;
		p++;
	}

	return h;
}

const char *get_symbol(int i)
{
	check_sloti(i);
	return s_symbols[i].name;
}

int install_symbol(const char *s, int len)
{
	int si;
	unsigned int h;
	char *pname;

	h = hash(s, len) % s_hashtab_size;
	for (si = s_hashtab[h].next; si >= 0; si = s_symbols[si].next) {
		if (cmpstrlen(s, len, s_symbols[si].name) == 0) {
			return si;
		}
	}

	if (s_free_nodes.next < 0) {
		printf("[gc: need symbols]\n");
		p_gc();
		if (s_free_nodes.next < 0) {
			fprintf(stderr, "lispe: out of symbols\n");
			goto fatal;
		}
	}

	pname = malloc(len + 1);
	if (pname == NULL) {
		fprintf(stderr, "lispe: out of heap space for symbols\n");
		goto fatal;
	}
	memcpy(pname, s, len);
	pname[len] = '\0';

	si = s_free_nodes.next;
	s_free_nodes.next = s_symbols[si].next;

	s_symbols[si].name = pname;	
	s_symbols[si].next = s_hashtab[h].next;
	s_hashtab[h].next = si;

	dprintf("symbol created %s\n", pname);

	return si;

fatal:
	exit(EXIT_FAILURE);
	return -1;
}

static int iabs(int a)
{
	if (a < 0)
		return -a;
	else
		return a;
}

static int dif(int a)
{
	enum { N_PER_BUCKET = 8 };
	return iabs((NCELL / a) - N_PER_BUCKET);
}

void init_symbols(void)
{
	int i, besti, bestabs, nabs;

	besti = 0;
	bestabs = dif(s_primes[besti]);
	for (i = 1; i < NELEMS(s_primes); i++) {
		nabs = dif(s_primes[i]);
		if (nabs < bestabs) {
			besti = i;
			bestabs = nabs;
		}
	}

	s_hashtab_size = s_primes[besti];
	printf("[symbols: %d, %u bytes, marks: %u bytes]\n", NSYMBOLS,
			sizeof(s_symbols),
			sizeof(s_sym_marks));
	printf("[symbols: hash table %u bytes, buckets %d]\n",
			sizeof(*s_hashtab) * s_hashtab_size, s_hashtab_size);
	s_hashtab = calloc(s_hashtab_size, sizeof(*s_hashtab));
	if (s_hashtab == NULL) {
		fprintf(stderr, "lispe: out of heap space for "
			        "the symbols hash table\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < s_hashtab_size; i++) {
		s_hashtab[i].next = -1;
	}

	s_symbols[NSYMBOLS - 1].next = -1;
	for (i = 0; i < NSYMBOLS - 1; i++) {
		s_symbols[i].next = i + 1;
	}
	s_free_nodes.next = 0;
}

