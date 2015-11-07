/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright (c) 2015, Nick Zivkovic
 */


#ifdef UMEM
#include <umem.h>
#endif
#include <stdlib.h>
#include <strings.h>
#include "graph_impl.h"

#define UNUSED(x) (void)(x)
#define CTOR_HEAD       UNUSED(ignored); UNUSED(flags)

umem_cache_t *cache_lg_graph;
umem_cache_t *cache_edge;
umem_cache_t *cache_w_edge;
umem_cache_t *cache_stack_elem;

#ifdef UMEM
//constructors...

int
lg_graph_ctor(void *buf, void *ignored, int flags)
{
	CTOR_HEAD;
	lg_graph_t *r = buf;
	bzero(r, sizeof (lg_graph_t));
	return (0);
}

int
edge_ctor(void *buf, void *ignored, int flags)
{
	CTOR_HEAD;
	edge_t *r = buf;
	bzero(r, sizeof (edge_t));
	return (0);
}

int
w_edge_ctor(void *buf, void *ignored, int flags)
{
	CTOR_HEAD;
	w_edge_t *r = buf;
	bzero(r, sizeof (w_edge_t));
	return (0);
}

int
stack_elem_ctor(void *buf, void *ignored, int flags)
{
	CTOR_HEAD;
	stack_elem_t *r = buf;
	bzero(r, sizeof (stack_elem_t));
	return (0);
}
#endif

int
graph_umem_init()
{
#ifdef UMEM
	cache_lg_graph = umem_cache_create("graph",
		sizeof (lg_graph_t),
		0,
		lg_graph_ctor,
		NULL,
		NULL,
		NULL,
		NULL,
		0);

	cache_edge = umem_cache_create("edge",
		sizeof (edge_t),
		0,
		edge_ctor,
		NULL,
		NULL,
		NULL,
		NULL,
		0);

	cache_w_edge = umem_cache_create("w_edge",
		sizeof (w_edge_t),
		0,
		w_edge_ctor,
		NULL,
		NULL,
		NULL,
		NULL,
		0);

	cache_stack_elem = umem_cache_create("stack_elem",
		sizeof (stack_elem_t),
		0,
		stack_elem_ctor,
		NULL,
		NULL,
		NULL,
		NULL,
		0);

#endif
	return (0);

}

lg_graph_t *
lg_mk_graph()
{
#ifdef UMEM
	return (umem_cache_alloc(cache_lg_graph, UMEM_NOFAIL));
#else
	return (calloc(1, sizeof (lg_graph_t)));
#endif
}

void
lg_rm_graph(lg_graph_t *g)
{
#ifdef UMEM
	bzero(g, sizeof (lg_graph_t));
	umem_cache_free(cache_lg_graph, g);
#else
	bzero(g, sizeof (lg_graph_t));
	free(g);
#endif
}

edge_t *
lg_mk_edge()
{
#ifdef UMEM
	return (umem_cache_alloc(cache_edge, UMEM_NOFAIL));
#else
	return (calloc(1, sizeof (edge_t)));
#endif
}

void
lg_rm_edge(edge_t *e)
{
#ifdef UMEM
	bzero(e, sizeof (edge_t));
	umem_cache_free(cache_edge, e);
#else
	bzero(e, sizeof (edge_t));
	free(e);
#endif
}

w_edge_t *
lg_mk_w_edge()
{
#ifdef UMEM
	return (umem_cache_alloc(cache_w_edge, UMEM_NOFAIL));
#else
	return (calloc(1, sizeof (w_edge_t)));
#endif
}

void
lg_rm_w_edge(w_edge_t *w)
{
#ifdef UMEM
	bzero(w, sizeof (w_edge_t));
	umem_cache_free(cache_w_edge, w);
#else
	bzero(w, sizeof (w_edge_t));
	free(w);
#endif
}

stack_elem_t *
lg_mk_stack_elem()
{
#ifdef UMEM
	return (umem_cache_alloc(cache_stack_elem, UMEM_NOFAIL));
#else
	return (calloc(1, sizeof (stack_elem_t)));
#endif
}

void
lg_rm_stack_elem(stack_elem_t *s)
{
#ifdef UMEM
	bzero(s, sizeof (stack_elem_t));
	umem_cache_free(cache_stack_elem, s);
#else
	bzero(s, sizeof (stack_elem_t));
	free(s);
#endif
}
