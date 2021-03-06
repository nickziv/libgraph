/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright (c) 2015, Nick Zivkovic
 * Copyright (c) 2015, Joyent, Inc.
 */

#include "graph_impl.h"
#include "graph_provider.h"

static int init = 0;
extern int graph_umem_init();
selem_t ignored;


/*
 * A gelem is basically treated as an integer. 
 */
int
gelem_cmp(selem_t e1, selem_t e2)
{
	if (e1.sle_u < e2.sle_u) {
		return (-1);
	}
	if (e1.sle_u > e2.sle_u) {
		return (1);
	}
	return (0);
}

int
gelem_bnd(selem_t e, selem_t min, selem_t max)
{
	if (e.sle_u > max.sle_u) {
		return (1);
	}
	if (e.sle_u < min.sle_u) {
		return (-1);
	}
	return (0);
}

/*
 * We basically compare the bits of the gelem_t's. This has the very obvious
 * flaw of turning string-comparisons into pointer-comparisons. If done right,
 * this can be fast. But it can also result non-obvious mistakes. Try using
 * numerical ID's via Macros instead of strings. This is unambiguous and fast.
 * If you don't know your strings before runtime, you'll have to practice
 * pointer-comparison discipline.
 */
int
graph_edge_cmp(selem_t e1, selem_t e2)
{
	edge_t *edge1 = e1.sle_p;
	edge_t *edge2 = e2.sle_p;
	if (edge1->ed_from.ge_u < edge2->ed_from.ge_u) {
		return (-1);
	}
	if (edge1->ed_from.ge_u > edge2->ed_from.ge_u) {
		return (1);
	}
	if (edge1->ed_to.ge_u < edge2->ed_to.ge_u) {
		return (-1);
	}
	if (edge1->ed_to.ge_u > edge2->ed_to.ge_u) {
		return (1);
	}
	return (0);
}

/*
 * TODO rewrite this function to not be in terms of edge_cmp, but to rather be
 * independent and therefor faster. First compare then from fields and then the
 * to fields. But between three elements instead of two.
 */
int
graph_edge_bnd(selem_t e, selem_t min, selem_t max)
{
	if (graph_edge_cmp(e, min) < 0) {
		return (-1);
	}
	if (graph_edge_cmp(e, max) > 0) {
		return (1);
	}
	return (0);
}

int
w_edge_cmp(selem_t e1, selem_t e2)
{
	w_edge_t *edge1 = e1.sle_p;
	w_edge_t *edge2 = e2.sle_p;
	if (edge1->wed_from.ge_u < edge2->wed_from.ge_u) {
		return (-1);
	}
	if (edge1->wed_from.ge_u > edge2->wed_from.ge_u) {
		return (1);
	}
	if (edge1->wed_weight.ge_u < edge2->wed_weight.ge_u) {
		return (-1);
	}
	if (edge1->wed_weight.ge_u > edge2->wed_weight.ge_u) {
		return (1);
	}
	if (edge1->wed_to.ge_u < edge2->wed_to.ge_u) {
		return (-1);
	}
	if (edge1->wed_to.ge_u > edge2->wed_to.ge_u) {
		return (1);
	}
	return (0);

}

int
w_edge_bnd(selem_t e, selem_t min, selem_t max)
{
	if (w_edge_cmp(e, min) < 0) {
		return (-1);
	}
	if (w_edge_cmp(e, max) > 0) {
		return (1);
	}
	return (0);
}

int
lg_is_graph(lg_graph_t *g)
{
	return (g->gr_type == GRAPH);
}

int
lg_is_digraph(lg_graph_t *g)
{
	return (g->gr_type == DIGRAPH);
}

int
lg_is_wgraph(lg_graph_t *g)
{
	return (g->gr_type == GRAPH_WE);
}

int
lg_is_wdigraph(lg_graph_t *g)
{
	return (g->gr_type == DIGRAPH_WE);
}

lg_graph_t *
lg_create_graph()
{
	if (!init) {
		graph_umem_init();
		init = 1;
	}
	lg_graph_t *g = lg_mk_graph();
	g->gr_type = GRAPH;
	g->gr_edges = slablist_create("graph_edges", graph_edge_cmp,
	    graph_edge_bnd, SL_SORTED);
	return (g);
}

lg_graph_t *
lg_create_wgraph()
{
	if (!init) {
		graph_umem_init();
		init = 1;
	}
	lg_graph_t *g = lg_mk_graph();
	g->gr_type = GRAPH_WE;
	g->gr_edges = slablist_create("wgraph_edges", w_edge_cmp, w_edge_bnd,
	    SL_SORTED);
	return (g);
}

lg_graph_t *
lg_create_digraph()
{
	if (!init) {
		graph_umem_init();
		init = 1;
	}
	lg_graph_t *g = lg_mk_graph();
	g->gr_type = DIGRAPH;
	g->gr_edges = slablist_create("digraph_edges", graph_edge_cmp,
	    graph_edge_bnd, SL_SORTED);
	return (g);
}

lg_graph_t *
lg_create_wdigraph()
{
	if (!init) {
		graph_umem_init();
		init = 1;
	}
	lg_graph_t *g = lg_mk_graph();
	g->gr_type = DIGRAPH_WE;
	g->gr_edges = slablist_create("wdigraph_edges", w_edge_cmp,
	    w_edge_bnd, SL_SORTED);
	return (g);
}

void
free_edge_cb(selem_t e)
{
	edge_t *edge = e.sle_p;
	lg_rm_edge(edge);
}

void
free_w_edge_cb(selem_t e)
{
	w_edge_t *edge = e.sle_p;
	lg_rm_w_edge(edge);
}

/*
 * This function destroys a graph (of any type) and frees it and its
 * subordinate structures from memory.
 */
void
lg_destroy_graph(lg_graph_t *g)
{
	slablist_t *edges = g->gr_edges;
	if (g->gr_type == DIGRAPH || g->gr_type == GRAPH) {
		slablist_destroy(edges, free_edge_cb);
		return;
	}
	slablist_destroy(edges, free_w_edge_cb);
}

void
snap_connect(lg_graph_t *g, gelem_t from, gelem_t to)
{
	if (g->gr_snaps == NULL) {
		return;
	}
	gelem_t ignored;
	change_t *c = lg_mk_change();
	c->ch_snap = g->gr_snap - 1;
	c->ch_graph = g;
	c->ch_op = CONNECT;
	c->ch_from = from;
	c->ch_to = to;
	GRAPH_CHANGE_ADD(g, c, c->ch_snap, c->ch_op, c->ch_from, c->ch_to,
	    c->ch_weight);
	/*
	 * Called twice because the nodes are being referred to by the change
	 * and their edge.
	 */
	if (g->gr_snap_cb != NULL) {
		g->gr_snap_cb(1, EDGE, from, to, ignored);
		g->gr_snap_cb(1, SNAP, from, to, ignored);
	}
	selem_t sc;
	sc.sle_p = c;
	slablist_add(g->gr_snaps, sc, 0);
}

void
snap_wconnect(lg_graph_t *g, gelem_t from, gelem_t to, gelem_t weight)
{
	if (g->gr_snaps == NULL) {
		return;
	}
	change_t *c = lg_mk_change();
	c->ch_snap = g->gr_snap - 1;
	c->ch_graph = g;
	c->ch_op = CONNECT;
	c->ch_from = from;
	c->ch_to = to;
	c->ch_weight = weight;
	GRAPH_CHANGE_ADD(g, c, c->ch_snap, c->ch_op, c->ch_from, c->ch_to,
	    c->ch_weight);
	/*
	 * Called twice because the nodes are being referred to by the change
	 * and their edge.
	 */
	if (g->gr_snap_cb != NULL) {
		g->gr_snap_cb(1, EDGE, from, to, weight);
		g->gr_snap_cb(1, SNAP, from, to, weight);
	}
	selem_t sc;
	sc.sle_p = c;
	slablist_add(g->gr_snaps, sc, 0);
}

void
snap_disconnect(lg_graph_t *g, gelem_t from, gelem_t to)
{
	if (g->gr_snaps == NULL) {
		return;
	}
	gelem_t ignored;
	change_t *c = lg_mk_change();
	c->ch_snap = g->gr_snap - 1;
	c->ch_graph = g;
	c->ch_op = DISCONNECT;
	c->ch_from = from;
	c->ch_to = to;
	GRAPH_CHANGE_ADD(g, c, c->ch_snap, c->ch_op, c->ch_from, c->ch_to,
	    c->ch_weight);
	/*
	 * We increment because we are adding to a change, but decrement
	 * because we are removing from an edge. Order matters. If we decrement
	 * first, the consumer's refcount may drop to zero causing a node to be
	 * reaped.
	 */
	if (g->gr_snap_cb != NULL) {
		g->gr_snap_cb(1, SNAP, from, to, ignored);
		g->gr_snap_cb(0, EDGE, from, to, ignored);
	}
	selem_t sc;
	sc.sle_p = c;
	slablist_add(g->gr_snaps, sc, 0);
}

void
snap_wdisconnect(lg_graph_t *g, gelem_t from, gelem_t to, gelem_t weight)
{
	if (g->gr_snaps == NULL) {
		return;
	}
	change_t *c = lg_mk_change();
	c->ch_snap = g->gr_snap - 1;
	c->ch_graph = g;
	c->ch_op = DISCONNECT;
	c->ch_from = from;
	c->ch_to = to;
	c->ch_weight = weight;
	GRAPH_CHANGE_ADD(g, c, c->ch_snap, c->ch_op, c->ch_from, c->ch_to,
	    c->ch_weight);
	/* see comments in snap_disconnect() */
	if (g->gr_snap_cb != NULL) {
		g->gr_snap_cb(1, SNAP, from, to, weight);
		g->gr_snap_cb(0, EDGE, from, to, weight);
	}
	selem_t sc;
	sc.sle_p = c;
	slablist_add(g->gr_snaps, sc, 0);
}

int
lg_connect(lg_graph_t *g, gelem_t from, gelem_t to)
{
	selem_t se1;
	selem_t se2;
	edge_t *e1;
	edge_t *e2;
	int r;
	if (from.ge_u == to.ge_u) {
		return (G_ERR_SELF_CONNECT);
	}
	switch (g->gr_type) {

	case DIGRAPH:
		e1 = lg_mk_edge();
		se1.sle_p = e1;
		e1->ed_from = from;
		e1->ed_to = to;
		r = slablist_add(g->gr_edges, se1, 0);
		if (r == SL_EDUP) {
			lg_rm_edge(e1);
			return (G_ERR_EDGE_EXISTS);
		}
		break;
	/*
	 * A GRAPH is just like a DIGRAPH, except all connections have to be
	 * full duplex.
	 */
	case GRAPH:
		e1 = lg_mk_edge();
		e2 = lg_mk_edge();
		se1.sle_p = e1;
		se2.sle_p = e2;
		e1->ed_from = from;
		e1->ed_to = to;
		e2->ed_from = to;
		e2->ed_to = from;
		r = slablist_add(g->gr_edges, se1, 0);
		if (r == SL_EDUP) {
			lg_rm_edge(e1);
			lg_rm_edge(e2);
			return (G_ERR_EDGE_EXISTS);
		}
		r = slablist_add(g->gr_edges, se2, 0);
		if (r == SL_EDUP) {
			lg_rm_edge(e1);
			lg_rm_edge(e2);
			return (G_ERR_EDGE_EXISTS);
		}
		break;

	default:
		break;
	}
	if (!g->gr_rollingback) {
		snap_connect(g, from, to);
	}
	return (0);
}

void
disconnect_cb(selem_t e)
{
	edge_t *edge = e.sle_p;
	lg_rm_edge(edge);
}

int
lg_disconnect(lg_graph_t *g, gelem_t from, gelem_t to)
{
	selem_t se1;
	selem_t se2;
	edge_t e1;
	edge_t e2;
	int r;
	if (from.ge_u == to.ge_u) {
		return (G_ERR_SELF_DISCONNECT);
	}
	switch (g->gr_type) {

	case DIGRAPH:
		se1.sle_p = &e1;
		e1.ed_from = from;
		e1.ed_to = to;
		r = slablist_rem(g->gr_edges, se1, 0, disconnect_cb);
		if (r == SL_ENFOUND) {
			return (G_ERR_NFOUND_DISCONNECT);
		}
		break;
	/*
	 * A GRAPH is just like a DIGRAPH, except all connections have to be
	 * full duplex.
	 */
	case GRAPH:
		se1.sle_p = &e1;
		se2.sle_p = &e2;
		e1.ed_from = from;
		e1.ed_to = to;
		e2.ed_from = to;
		e2.ed_to = from;
		r = slablist_rem(g->gr_edges, se1, 0, disconnect_cb);
		if (r == SL_ENFOUND) {
			return (G_ERR_NFOUND_DISCONNECT);
		}

		r = slablist_rem(g->gr_edges, se2, 0, disconnect_cb);
		if (r == SL_ENFOUND) {
			return (G_ERR_NFOUND_DISCONNECT);
		}
		break;

	default:
		break;
	}
	if (!g->gr_rollingback) {
		snap_disconnect(g, from, to);
	}
	return (0);
}


int
lg_wconnect(lg_graph_t *g, gelem_t from, gelem_t to, gelem_t weight)
{
	selem_t swe1;
	selem_t swe2;
	w_edge_t *we1;
	w_edge_t *we2;
	if (from.ge_u == to.ge_u) {
		return (G_ERR_SELF_CONNECT);
	}
	int r;
	switch (g->gr_type) {

	case DIGRAPH_WE:
		we1 = lg_mk_w_edge();
		swe1.sle_p = we1;
		we1->wed_from = from;
		we1->wed_to = to;
		we1->wed_weight = weight;
		r = slablist_add(g->gr_edges, swe1, 0);
		if (r == SL_EDUP) {
			lg_rm_w_edge(we1);
			return (G_ERR_EDGE_EXISTS);
		}
		break;

	case GRAPH_WE:
		we1 = lg_mk_w_edge();
		we2 = lg_mk_w_edge();
		swe1.sle_p = we1;
		swe2.sle_p = we2;
		we1->wed_from = from;
		we1->wed_to = to;
		we1->wed_weight = weight;
		we2->wed_from = from;
		we2->wed_to = to;
		we2->wed_weight = weight;

		r = slablist_add(g->gr_edges, swe1, 0);
		if (r == SL_EDUP) {
			lg_rm_w_edge(we1);
			lg_rm_w_edge(we2);
			return (G_ERR_EDGE_EXISTS);
		}

		r = slablist_add(g->gr_edges, swe2, 0);
		if (r == SL_EDUP) {
			lg_rm_w_edge(we1);
			lg_rm_w_edge(we2);
			return (G_ERR_EDGE_EXISTS);
		}
		break;
	default:
		break;
	}
	if (!g->gr_rollingback) {
		snap_wconnect(g, from, to, weight);
	}
	return (0);
}

void
wdisconnect_cb(selem_t e)
{
	w_edge_t *edge = e.sle_p;
	lg_rm_w_edge(edge);
}

int
lg_wdisconnect(lg_graph_t *g, gelem_t from, gelem_t to, gelem_t weight)
{
	selem_t swe1;
	selem_t swe2;
	w_edge_t we1;
	w_edge_t we2;
	if (from.ge_u == to.ge_u) {
		return (G_ERR_SELF_DISCONNECT);
	}
	int r;
	switch (g->gr_type) {

	case DIGRAPH_WE:
		swe1.sle_p = &we1;
		we1.wed_from = from;
		we1.wed_to = to;
		we1.wed_weight = weight;
		r = slablist_rem(g->gr_edges, swe1, 0, wdisconnect_cb);
		if (r == SL_ENFOUND) {
			return (G_ERR_NFOUND_DISCONNECT);
		}
		break;

	case GRAPH_WE:
		swe1.sle_p = &we1;
		swe2.sle_p = &we2;
		we1.wed_from = from;
		we1.wed_to = to;
		we1.wed_weight = weight;
		we2.wed_from = from;
		we2.wed_to = to;
		we2.wed_weight = weight;

		r = slablist_rem(g->gr_edges, swe1, 0, wdisconnect_cb);
		if (r == SL_ENFOUND) {
			return (G_ERR_NFOUND_DISCONNECT);
		}

		r = slablist_rem(g->gr_edges, swe2, 0, wdisconnect_cb);
		if (r == SL_ENFOUND) {
			return (G_ERR_NFOUND_DISCONNECT);
		}
		break;
	default:
		break;
	}
	if (!g->gr_rollingback) {
		snap_wdisconnect(g, from, to, weight);
	}
	return (0);
}

typedef struct args {
	fold_cb_t		*a_cb;
	adj_cb_t		*a_acb;
	lg_graph_t		*a_g;
	gelem_t			a_agg;
	slablist_t 		*a_q;
	slablist_t 		*a_v;
	int			a_stat;
} args_t;

/*
 * Checks to see if a node, encoded as a slablist_elem was visited.
 */
int
visited(slablist_t *V, selem_t enq)
{
	selem_t fnd;
	int r = slablist_find(V, enq, &fnd);
	if (r == SL_ENFOUND) {
		return (0);
	}
	return (1);
}

/*
 * Same as above, but the node is encoded as a gelem.
 */
int
gvisited(slablist_t *V, gelem_t node)
{
	selem_t fnd;
	selem_t snode;
	snode.sle_u = node.ge_u;
	int r = slablist_find(V, snode, &fnd);
	if (r == SL_ENFOUND) {
		return (0);
	}
	return (1);
}


selem_t
visit_and_q(selem_t z, selem_t *e, uint64_t sz)
{
	args_t *args = z.sle_p;
	gtype_t type = args->a_g->gr_type;
	slablist_t *Q = args->a_q;
	slablist_t *V = args->a_v;
	uint64_t i = 0;
	while (i < sz) {
		selem_t c = e[i];
		edge_t *edge = NULL;
		w_edge_t *w_edge = NULL;
		selem_t enq;
		gelem_t from;
		gelem_t weight;
		weight.ge_u = 0;
		if (type == DIGRAPH || type == GRAPH) {
			edge = c.sle_p;
			from.ge_u = edge->ed_from.ge_u;
			enq.sle_u = edge->ed_to.ge_u;
		} else {
			w_edge = c.sle_p;
			from.ge_u = w_edge->wed_from.ge_u;
			enq.sle_u = w_edge->wed_to.ge_u;
			weight.ge_u = w_edge->wed_weight.ge_u;
		}
		if (!visited(V, enq)) {
			gelem_t to;
			to.ge_u = enq.sle_u;
			if (args->a_acb != NULL) {
				args->a_acb(to, from, weight, args->a_agg);
			}
			GRAPH_BFS_ENQ(to);
			slablist_add(Q, enq, 0);
			GRAPH_BFS_VISIT(to);
			slablist_add(V, enq, 0);
		}
		i++;
	}
	return (z);
}

/*
 * This is the redundant version of the previous. It doesn't add anything to
 * the visited set, since we assume that such a set is unneeded.
 */
selem_t
just_q(selem_t z, selem_t *e, uint64_t sz)
{
	args_t *args = z.sle_p;
	gtype_t type = args->a_g->gr_type;
	slablist_t *Q = args->a_q;
	uint64_t i = 0;
	while (i < sz) {
		selem_t c = e[i];
		edge_t *edge = NULL;
		w_edge_t *w_edge = NULL;
		selem_t enq;
		gelem_t from;
		gelem_t weight;
		weight.ge_u = 0;
		if (type == DIGRAPH || type == GRAPH) {
			edge = c.sle_p;
			from.ge_u = edge->ed_from.ge_u;
			enq.sle_u = edge->ed_to.ge_u;
		} else {
			w_edge = c.sle_p;
			from.ge_u = w_edge->wed_from.ge_u;
			enq.sle_u = w_edge->wed_to.ge_u;
			weight.ge_u = w_edge->wed_weight.ge_u;
		}
		gelem_t to;
		to.ge_u = enq.sle_u;
		if (args->a_acb != NULL) {
			args->a_acb(to, from, weight, args->a_agg);
		}
		GRAPH_BFS_RDNT_ENQ(to);
		slablist_add(Q, enq, 0);
		i++;
	}
	return (z);
}


/*
 * This is an implementation function that can be used by the queue and stack
 * code, but takes an appropriate callback.
 */
void
add_connected(lg_graph_t *g, gelem_t origin, selem_t zero, slablist_fold_t cb)
{
	gelem_t min_to;
	gelem_t max_to;
	selem_t min_edge;
	selem_t max_edge;

	min_to.ge_u = 0;
	max_to.ge_u = UINT64_MAX;
	edge_t min;
	edge_t max;
	w_edge_t w_min;
	w_edge_t w_max;

	if (g->gr_type == GRAPH || g->gr_type == DIGRAPH) {
		min.ed_from = origin;
		min.ed_to = min_to;

		max.ed_from = origin;
		max.ed_to = max_to;

		min_edge.sle_p = &min;
		max_edge.sle_p = &max;
	} else {
		w_min.wed_from = origin;
		w_min.wed_to = min_to;
		w_min.wed_weight.ge_u = 0;

		w_max.wed_from = origin;
		w_max.wed_to = max_to;
		w_max.wed_weight.ge_u = UINT64_MAX;

		min_edge.sle_p = &w_min;
		max_edge.sle_p = &w_max;
	}
	slablist_foldr_range(g->gr_edges, cb, min_edge,
	max_edge, zero);
}

/*
 * We enqueue all nodes that are reachable from `origin`. Unless we've already
 * visisted them.
 */
void
enq_connected(lg_graph_t *g, gelem_t origin, selem_t zero)
{
	add_connected(g, origin, zero, visit_and_q);
}

/*
 * This is the 'redundant' version of the previous function. It queues the
 * nodes regardless of whether we've visited them or not.
 */
void
enq_rdnt_connected(lg_graph_t *g, gelem_t origin, selem_t zero)
{
	add_connected(g, origin, zero, just_q);
}

/*
 * Sometimes we just want a enqueue a single node (usually the start-node).
 */
void
enq_origin(slablist_t *Q, slablist_t *V, gelem_t origin)
{
	GRAPH_BFS_ENQ(origin);
	selem_t enq;
	enq.sle_u = origin.ge_u;
	slablist_add(Q, enq, 0);
	slablist_add(V, enq, 0);
}

/*
 * Same as previous function, but we don't assume a visited-set.
 */
void
enq_rdnt_origin(slablist_t *Q, gelem_t origin)
{
	GRAPH_BFS_RDNT_ENQ(origin);
	selem_t enq;
	enq.sle_u = origin.ge_u;
	slablist_add(Q, enq, 0);
}

/*
 * Returns the first elem, and dequeues it.
 */
gelem_t
deq(slablist_t *Q)
{
	selem_t sfirst = slablist_head(Q);
	slablist_rem(Q, ignored, 0, NULL);
	gelem_t first;
	first.ge_u = sfirst.sle_u;
	return (first);
}

/*
 * Similar in principle to enq_origin, but we push a stack_elem, which may
 * contain a bookmark, which is pointing to the first 'edge' that connects the
 * origin to another gelem.
 */
void
push(slablist_t *S, stack_elem_t *s)
{
	selem_t push;
	push.sle_p = s;
	slablist_add(S, push, 0);
}

/*
 * This gives us the pointer to the last bmark on the stack, but _doesn't_
 * remove it.
 */
stack_elem_t *
last_se(slablist_t *S)
{
	selem_t slast = slablist_end(S);
	uint64_t elems = slablist_get_elems(S);
	if (!elems) {
		return (NULL);
	}
	stack_elem_t *last;
	last = slast.sle_p;
	return (last);
}

/*
 * Returns the last stack_elem, and pops it.
 */
static stack_elem_t *
pop(lg_graph_t *g, slablist_t *S)
{
	(void)g;
	selem_t slast = slablist_end(S);
	uint64_t elems = slablist_get_elems(S);
	slablist_rem(S, ignored, (elems - 1), NULL);
	stack_elem_t *last;
	last = slast.sle_p;
	return (last);
}

/*
 * Returns the last stack_elem, pops it from the regular stack, and from the
 * branch-stack if present.
 */
static stack_elem_t *
br_pop(lg_graph_t *g, slablist_t *S, slablist_t *B)
{
	(void)g;
	selem_t slast = slablist_end(S);
	uint64_t elems = slablist_get_elems(S);
	slablist_rem(S, ignored, (elems - 1), NULL);

	stack_elem_t *last;
	last = slast.sle_p;

	/*
	 * We can safely assume that if the node on top of stack S is present
	 * in slack B, it will also be at the end -- if it's not, we've screwed
	 * up the stack handling code.
	 */
	elems = slablist_get_elems(B);
	if (elems) {
		selem_t blast = slablist_end(B);
		if (blast.sle_p == last->se_node.ge_p) {
			slablist_rem(B, ignored, (elems - 1), NULL);
		}
	}

	return (last);
}

/*
 * For a BFS, we begin at the node `start`. We do ranged slablist_fold from
 * edge [start:0x0] to [start:0xffffffffffffffff]. Even though 0x0 and 0xff..
 * might not exist, the slablist_fold function will start and the smallest elem
 * in the range and end at the largest. As we fold, we add the `to` elems to a
 * queue and to a `visited` set. We then redo this to every elem in the queue,
 * adding all of their `to` elems to the visited set, until we can't reach a
 * single node that hasn't been visited.
 *
 * So this function executes the BFS algorithm above, however it takes two
 * callbacks: the first callback is called whenever we are about visit the
 * neighbors of some node X, while the second callback aggregates over the
 * nodes and indicates if the function should terminate its search. The first
 * callback is particularly useful when one needs to know what the 'parent' of
 * nodes passed to the second callback is -- which itself is useful for finding
 * shortest paths.
 *
 * This aggregation can be used to compute arbitrary things like shortest path,
 * centrality values, and so on.
 */
gelem_t
lg_bfs_fold(lg_graph_t *g, gelem_t start, adj_cb_t *acb, fold_cb_t *cb, gelem_t gzero)
{
	/*
	 * First we create an appropriate queue and visited-set.
	 */
	GRAPH_BFS_BEGIN(g);
	uint64_t nedges = slablist_get_elems(g->gr_edges);
	if (nedges == 0) {
		return (gzero);
	}
	args_t args;
	selem_t zero;
	zero.sle_p = &args;
	slablist_t *Q;
	slablist_t *V;
	Q = slablist_create("graph_bfs_queue", NULL, NULL, SL_ORDERED);
	V = slablist_create("graph_bfs_vset", gelem_cmp, gelem_bnd,
	    SL_SORTED);

	args.a_g = g;
	args.a_cb = cb;
	args.a_acb = acb;
	args.a_q = Q;
	args.a_v = V;
	args.a_agg = gzero;
	/*
	 * Here we execute BFS. We enqueue all the nodes connected to start,
	 * and we loop through BFS, until we reach the terminating condition --
	 * or visit all of the nodes.
	 */
	enq_origin(Q, V, start);
	if (cb != NULL) {
		while (slablist_get_elems(Q) > 0) {
			gelem_t last = deq(Q);
			GRAPH_BFS_DEQ(last);
			int stat = cb(args.a_agg, last, &(args.a_agg));
			if (stat) {
				/*
				 * The user should save what he's looking for
				 * in a_agg.
				 */
				slablist_destroy(Q, NULL);
				slablist_destroy(V, NULL);
				GRAPH_BFS_END(g);
				return (args.a_agg);
			}
			enq_connected(g, last, zero);
		}
	} else {
		while (slablist_get_elems(Q) > 0) {
			gelem_t last = deq(Q);
			GRAPH_BFS_DEQ(last);
			enq_connected(g, last, zero);
		}
	}
	slablist_destroy(Q, NULL);
	slablist_destroy(V, NULL);
	GRAPH_BFS_END(g);
	return (args.a_agg);
}

/*
 * A 'redundant' version of BFS. It walks over nodes in level-order just like
 * normal BFS, however it doesn't skip nodes that it has already visited. This
 * can be used, for example,  to convert a directed acyclic graph into a tree,
 * where some parents share the same children. If the graph is undirected or
 * has cycles, this bfs will loop forever. So use this with caution!
 */
gelem_t
lg_bfs_rdnt_fold(lg_graph_t *g, gelem_t start, adj_cb_t *acb, fold_cb_t *cb,
    gelem_t gzero)
{
	/*
	 * First we create an appropriate queue.
	 */
	GRAPH_BFS_RDNT_BEGIN(g);
	uint64_t nedges = slablist_get_elems(g->gr_edges);
	if (nedges == 0) {
		return (gzero);
	}

	args_t args;
	selem_t zero;
	zero.sle_p = &args;
	slablist_t *Q;
	Q = slablist_create("graph_bfs_rdnt_queue", NULL, NULL, SL_ORDERED);

	args.a_g = g;
	args.a_cb = cb;
	args.a_acb = acb;
	args.a_q = Q;
	args.a_v = NULL;
	args.a_agg = gzero;
	/*
	 * Here we execute BFS. We enqueue all the nodes connected to start,
	 * and we loop through BFS, until we reach the terminating condition --
	 * or reach the lowest level.
	 */
	enq_rdnt_origin(Q, start);
	if (cb != NULL) {
		while (slablist_get_elems(Q) > 0) {
			gelem_t last = deq(Q);
			GRAPH_BFS_RDNT_DEQ(last);
			int stat = cb(args.a_agg, last, &(args.a_agg));
			if (stat) {
				/*
				 * The user should save what he's looking for
				 * in a_agg.
				 */
				slablist_destroy(Q, NULL);
				GRAPH_BFS_RDNT_END(g);
				return (args.a_agg);
			}
			enq_rdnt_connected(g, last, zero);
		}
	} else {
		while (slablist_get_elems(Q) > 0) {
			gelem_t last = deq(Q);
			GRAPH_BFS_RDNT_DEQ(last);
			enq_rdnt_connected(g, last, zero);
		}
	}
	slablist_destroy(Q, NULL);
	GRAPH_BFS_RDNT_END(g);
	return (args.a_agg);

}

slablist_bm_t *
edge_bm(lg_graph_t *g, gelem_t start)
{
	slablist_bm_t *bm = slablist_bm_create();
	selem_t slret;
	edge_t start_e;
	edge_t end_e;
	w_edge_t start_we;
	w_edge_t end_we;
	selem_t start_edge;
	selem_t end_edge;

	switch (g->gr_type) {

	case GRAPH:
	case DIGRAPH:
		start_e.ed_from = start;
		start_e.ed_to.ge_u = 0;
		end_e.ed_from = start;
		end_e.ed_to.ge_u = UINT64_MAX;
		start_edge.sle_p = &start_e;
		end_edge.sle_p = &end_e;
		break;
	case GRAPH_WE:
	case DIGRAPH_WE:
		start_we.wed_from = start;
		start_we.wed_to.ge_u = 0;
		start_we.wed_weight.ge_u = 0;
		end_we.wed_from = start;
		end_we.wed_to.ge_u = UINT64_MAX;
		end_we.wed_weight.ge_u = UINT64_MAX;
		start_edge.sle_p = &start_we;
		end_edge.sle_p = &end_we;
		break;
	}

	int sr = slablist_range_min(g->gr_edges, bm, start_edge, end_edge,
	    &slret);
	if (sr != 0) {
		slablist_bm_destroy(bm);
		return (NULL);
	}
	return (bm);
}

/*
 * This function returns a bookmark to an edge A->B, if A is pushable. A is
 * pushable if it hasn't been visited before, and if it is reachable from the
 * edge pointed to by `last`. If this function doesn't find any unvisited nodes
 * from the edges emanating from the node in `last`, it returns NULL.
 *
 * We should also go over some constraints on what the value of the bookmark
 * `last` can be. `last` begins life, point to an edge A->B. It is incremented
 * to point to other edges contain A in the `from` member -- A->X, A->Y, A->Z,
 * and so forth. It can _never_ point to any edge that doesn't contain A in the
 * `from` member.
 */
stack_elem_t *
get_pushable(lg_graph_t *g, slablist_t *V, stack_elem_t *last_se)
{
	slablist_bm_t *last = last_se->se_bm;
	/*
	 * We're trying to find an unvisited neighbor of a node that has no
	 * outgoing neighbors.
	 */
	if (last == NULL) {
		return (NULL);
	}
	slablist_t *edges = g->gr_edges;
	selem_t curelem;
	slablist_cur(edges, last, &curelem);
	edge_t *e;
	w_edge_t *we;
	gelem_t adj;
	gelem_t parent;
	/*
	 * We try to find a child that wasn't visited.
	 */
	if (g->gr_type == GRAPH || g->gr_type == DIGRAPH) {
		e = curelem.sle_p;
		GRAPH_DFS_BM(g, last, e->ed_from, e->ed_to);
		parent.ge_u = e->ed_from.ge_u;
		adj.ge_u = e->ed_to.ge_u;
		while (1) {
			/*
			 * If we've never visited `adj`, we break out
			 * of this loop and return a bookmark to it.
			 */
			if (!gvisited(V, adj)) {
				break;
			}
			/* We check out the next edge, if we have one */
			int end = slablist_next(edges, last, &curelem);
			if (end) {
				slablist_prev(edges, last, &curelem);
				e = curelem.sle_p;
				GRAPH_DFS_BM(g, last, e->ed_from, e->ed_to);
				return (NULL);
			}
			e = curelem.sle_p;
			GRAPH_DFS_BM(g, last, e->ed_from, e->ed_to);
			/*
			 * If this new edge doesn't contain a `from` node that
			 * is identical to `parent`, we rewind the bookmark so
			 * that it points the last node that contained
			 * `parent`, and we return NULL, indicating that there
			 * are no adjacent nodes that we can visit.
			 */
			if (parent.ge_u != e->ed_from.ge_u) {
				slablist_prev(edges, last, &curelem);
				e = curelem.sle_p;
				GRAPH_DFS_BM(g, last, e->ed_from, e->ed_to);
				return (NULL);
			}
			/* otherwise, we update `parent`, and `adj`. */
			parent.ge_u = e->ed_from.ge_u;
			adj.ge_u = e->ed_to.ge_u;
			e = curelem.sle_p;
			GRAPH_DFS_BM(g, last, e->ed_from, e->ed_to);
		}
	} else {
		we = curelem.sle_p;
		GRAPH_DFS_BM(g, last, we->wed_from, we->wed_to);
		parent.ge_u = we->wed_from.ge_u;
		adj.ge_u = we->wed_to.ge_u;
		while (1) {
			/*
			 * If we've never visited `adj`, we break out
			 * of this loop and return a bookmark to it.
			 */
			if (!gvisited(V, adj)) {
				break;
			}
			/* We check out the next edge, if we have one */
			int end = slablist_next(edges, last, &curelem);
			if (end) {
				slablist_prev(edges, last, &curelem);
				e = curelem.sle_p;
				GRAPH_DFS_BM(g, last, we->wed_from, we->wed_to);
				return (NULL);
			}
			we = curelem.sle_p;
			GRAPH_DFS_BM(g, last, we->wed_from, we->wed_to);
			/*
			 * If this new edge doesn't contain a `from` node that
			 * is identical to `parent`, we rewind the bookmark so
			 * that it points the last node that contained
			 * `parent`, and we return NULL, indicating that there
			 * are no adjacent nodes that we can visit.
			 */
			if (parent.ge_u != we->wed_from.ge_u) {
				slablist_prev(edges, last, &curelem);
				we = curelem.sle_p;
				GRAPH_DFS_BM(g, last, we->wed_from, we->wed_to);
				return (NULL);
			}
			/* otherwise, we update `parent`, and `adj`. */
			parent.ge_u = we->wed_from.ge_u;
			adj.ge_u = we->wed_to.ge_u;
			we = curelem.sle_p;
			GRAPH_DFS_BM(g, last, we->wed_from, we->wed_to);
		}
	}
	slablist_bm_t *bm = edge_bm(g, adj);
	stack_elem_t *pushable = lg_mk_stack_elem();
	pushable->se_node = adj;
	pushable->se_bm = bm;
	return (pushable);
}

/*
 * This is the redundant variant of the previous function. It doesn't have to
 * search for an unvisited child-node -- it merely returns the next child-node,
 * until there are no more child-nodes to return. Therefor, there is no need
 * for loops and such.
 */
stack_elem_t *
get_pushable_rdnt(lg_graph_t *g, stack_elem_t *last_se)
{
	if (last_se == NULL) {
		return (NULL);
	}
	slablist_bm_t *last = last_se->se_bm;
	/*
	 * We're trying to find an unvisited neighbor of a node that has no
	 * outgoing neighbors.
	 */
	if (last == NULL || last_se->se_end) {
		return (NULL);
	}
	slablist_t *edges = g->gr_edges;
	selem_t curelem;
	slablist_cur(edges, last, &curelem);
	edge_t *e;
	w_edge_t *we;
	gelem_t adj;
	if (g->gr_type == GRAPH || g->gr_type == DIGRAPH) {
		e = curelem.sle_p;
		GRAPH_DFS_RDNT_BM(g, last, e->ed_from, e->ed_to);
		adj.ge_u = e->ed_to.ge_u;
		/*
		 * If `adj` isn't a child of `se_node`, we rewind and return
		 * NULL.
		 */
		if (last_se->se_node.ge_u != e->ed_from.ge_u) {
			slablist_prev(edges, last, &curelem);
			e = curelem.sle_p;
			GRAPH_DFS_RDNT_BM(g, last, e->ed_from, e->ed_to);
			return (NULL);
		}
		/* We advance the bookmark for the next call */
		int end = slablist_next(edges, last, &curelem);
		if (end) {
			slablist_prev(edges, last, &curelem);
			e = curelem.sle_p;
			GRAPH_DFS_RDNT_BM(g, last, e->ed_from, e->ed_to);
			last_se->se_end = end;
		}
		e = curelem.sle_p;
		GRAPH_DFS_RDNT_BM(g, last, e->ed_from, e->ed_to);
	} else {
		we = curelem.sle_p;
		GRAPH_DFS_RDNT_BM(g, last, we->wed_from, we->wed_to);
		adj.ge_u = we->wed_to.ge_u;
		if (last_se->se_node.ge_u != we->wed_from.ge_u) {
			slablist_prev(edges, last, &curelem);
			we = curelem.sle_p;
			GRAPH_DFS_RDNT_BM(g, last, we->wed_from, we->wed_to);
			return (NULL);
		}
		/* We check out the next edge, if we have one */
		int end = slablist_next(edges, last, &curelem);
		if (end) {
			slablist_prev(edges, last, &curelem);
			e = curelem.sle_p;
			GRAPH_DFS_RDNT_BM(g, last, we->wed_from, we->wed_to);
			last_se->se_end = end;
		}
		we = curelem.sle_p;
		GRAPH_DFS_RDNT_BM(g, last, we->wed_from, we->wed_to);
	}
	slablist_bm_t *bm = edge_bm(g, adj);
	stack_elem_t *pushable = lg_mk_stack_elem();
	pushable->se_node = adj;
	pushable->se_bm = bm;
	return (pushable);
}

/*
 * Used by slablist_map to free stack_elem_t's and destroy their bookmarks.
 */
void
free_stack_elem(selem_t *e, uint64_t sz)
{
	uint64_t i = 0;
	while (i < sz) {
		stack_elem_t *se = e[i].sle_p;
		slablist_bm_destroy(se->se_bm);
		lg_rm_stack_elem(se);
		i++;
	}
}

/*
 * For a DFS, we begin at node `start`. We add `start` to a stack, visit its
 * first child and add it to the stack. Visit child's first child and so on.
 * When we hit bottom, or an already visited node, we go back up the stack go
 * the next child (which is supported the slablist's 'bookmark' feature --- we
 * have bookmarks on the edge-list, 1 bookmark for each gelem on the stack).
 * The book marks are stored in a slablist the 'mirror' the stack (which is
 * also a slablist).
 */
gelem_t
lg_dfs_fold(lg_graph_t *g, gelem_t start, pop_cb_t *pcb, fold_cb_t *cb,
    gelem_t gzero)
{
	GRAPH_DFS_BEGIN(g);
	uint64_t nedges = slablist_get_elems(g->gr_edges);
	if (nedges == 0) {
		return (gzero);
	}

	args_t args;

	slablist_t *S;
	slablist_t *V;

	S = slablist_create("graph_dfs_stack", NULL, NULL, SL_ORDERED);
	V = slablist_create("graph_dfs_vset", gelem_cmp, gelem_bnd, SL_SORTED);

	args.a_g = g;
	args.a_cb = cb;
	args.a_q = S;
	args.a_v = V;
	args.a_agg = gzero;

	slablist_bm_t *bm = edge_bm(g, start);
	stack_elem_t *last_pushed = lg_mk_stack_elem();
	last_pushed->se_node = start;
	last_pushed->se_bm = bm;
	/*
	 * `start` has no outgoing edges, so we just call `cb` on `start`, and
	 * return.
	 */
	int stat = 0;
	if (bm == NULL) {
		stat = cb(args.a_agg, start, &(args.a_agg));
		return (args.a_agg);
	}

	selem_t sedge;
	edge_t *e;
	w_edge_t *we;
	GRAPH_DFS_PUSH(g, start);
	push(S, last_pushed);
	gelem_t par_pushed;
	slablist_cur(g->gr_edges, bm, &sedge);
	if (g->gr_type == GRAPH || g->gr_type == DIGRAPH) {
		e = sedge.sle_p;
		par_pushed.ge_u = e->ed_from.ge_u;
	} else {
		we = sedge.sle_p;
		par_pushed.ge_u = we->wed_from.ge_u;
	}
	stat = cb(args.a_agg, par_pushed, &(args.a_agg));
	if (stat) {
		slablist_map(S, free_stack_elem);
		slablist_destroy(S, NULL);
		slablist_destroy(V, NULL);
		GRAPH_DFS_END(g);
		return (args.a_agg);
	}
	selem_t visited;
	visited.sle_u = par_pushed.ge_u;
	slablist_add(V, visited, 0);
	stack_elem_t *pushable;

try_continue:;
	while ((pushable =
	    get_pushable(g, V, last_pushed)) != NULL) {
		GRAPH_DFS_PUSH(g, pushable->se_node);
		push(S, pushable);
		last_pushed = last_se(S);
		stat = cb(args.a_agg, pushable->se_node,
		    &(args.a_agg));
		/* we've met our terminating condition */
		if (stat) {
			slablist_map(S, free_stack_elem);
			slablist_destroy(S, NULL);
			slablist_destroy(V, NULL);
			GRAPH_DFS_END(g);
			return (args.a_agg);
		}
		visited.sle_u = pushable->se_node.ge_u;
		slablist_add(V, visited, 0);
	}

	/*
	 * We've reached the bottom. We want to pop one element off of the
	 * stack, and then we want to try to continue to DFS search. We keep
	 * doing this until the stack is empty.
	 */
pop_again:;
	int popstat = 0;
	stack_elem_t *popped = NULL;
	uint64_t depth = slablist_get_elems(S);
	if (depth > 1) {
		popped = pop(g, S);
		GRAPH_DFS_POP(g, popped->se_node);
		if (pcb != NULL) {
			popstat = pcb(popped->se_node, args.a_agg);
		}
		if (popped->se_bm != NULL) {
			slablist_bm_destroy(popped->se_bm);
		}
		lg_rm_stack_elem(popped);
		last_pushed = last_se(S);
		if (popstat == 1) {
			goto pop_again;
		}
		goto try_continue;
	} else if (depth == 1) {
		popped = pop(g, S);
		GRAPH_DFS_POP(g, popped->se_node);
		if (pcb != NULL) {
			(void)pcb(popped->se_node, args.a_agg);
		}
		if (popped->se_bm != NULL) {
			slablist_bm_destroy(popped->se_bm);
		}
		lg_rm_stack_elem(popped);
	}
	slablist_destroy(S, NULL);
	slablist_destroy(V, NULL);
	GRAPH_DFS_END(g);
	return (args.a_agg);
}

/*
 * This is a 'redundant' version of DFS. It visits nodes more than once. Will
 * loop infinitely on graphs with cycles, so be careful.
 */
gelem_t
lg_dfs_rdnt_fold(lg_graph_t *g, gelem_t start, pop_cb_t *pcb, fold_cb_t *cb,
    gelem_t gzero)
{
	GRAPH_DFS_RDNT_BEGIN(g);
	uint64_t nedges = slablist_get_elems(g->gr_edges);
	if (nedges == 0) {
		return (gzero);
	}

	args_t args;

	slablist_t *S;

	S = slablist_create("graph_dfs_stack", NULL, NULL, SL_ORDERED);

	args.a_g = g;
	args.a_cb = cb;
	args.a_q = S;
	args.a_v = NULL;
	args.a_agg = gzero;

	slablist_bm_t *bm = edge_bm(g, start);
	stack_elem_t *last_pushed = lg_mk_stack_elem();
	last_pushed->se_node = start;
	last_pushed->se_bm = bm;
	/*
	 * `start` has no outgoing edges, so we just call `cb` on `start`, and
	 * return.
	 */
	int stat = 0;
	if (bm == NULL) {
		stat = cb(args.a_agg, start, &(args.a_agg));
		return (args.a_agg);
	}

	selem_t sedge;
	edge_t *e;
	w_edge_t *we;
	GRAPH_DFS_RDNT_PUSH(g, start);
	push(S, last_pushed);
	gelem_t par_pushed;
	slablist_cur(g->gr_edges, bm, &sedge);
	if (g->gr_type == GRAPH || g->gr_type == DIGRAPH) {
		e = sedge.sle_p;
		par_pushed.ge_u = e->ed_from.ge_u;
	} else {
		we = sedge.sle_p;
		par_pushed.ge_u = we->wed_from.ge_u;
	}
	stat = cb(args.a_agg, par_pushed, &(args.a_agg));
	if (stat) {
		slablist_map(S, free_stack_elem);
		slablist_destroy(S, NULL);
		GRAPH_DFS_RDNT_END(g);
		return (args.a_agg);
	}
	stack_elem_t *pushable;

try_continue:;
	while ((pushable =
	    get_pushable_rdnt(g, last_pushed)) != NULL) {
		GRAPH_DFS_RDNT_PUSH(g, pushable->se_node);
		push(S, pushable);
		last_pushed = last_se(S);
		stat = cb(args.a_agg, pushable->se_node,
		    &(args.a_agg));
		/* we've met our terminating condition */
		if (stat) {
			slablist_map(S, free_stack_elem);
			slablist_destroy(S, NULL);
			GRAPH_DFS_RDNT_END(g);
			return (args.a_agg);
		}
	}

	/*
	 * We've reached the bottom. We want to pop one element off of the
	 * stack, and then we want to try to continue to DFS search. We keep
	 * doing this until the stack is empty.
	 */
pop_again:;
	int popstat = 0;
	stack_elem_t *popped = NULL;
	uint64_t depth = slablist_get_elems(S);
	if (depth > 1) {
		popped = pop(g, S);
		GRAPH_DFS_RDNT_POP(g, popped->se_node);
		if (pcb != NULL) {
			popstat = pcb(popped->se_node, args.a_agg);
		}
		if (popped->se_bm != NULL) {
			slablist_bm_destroy(popped->se_bm);
		}
		lg_rm_stack_elem(popped);
		last_pushed = last_se(S);
		if (popstat) {
			goto pop_again;
		}
		goto try_continue;
	} else if (depth == 1) {
		popped = pop(g, S);
		GRAPH_DFS_RDNT_POP(g, popped->se_node);
		if (pcb != NULL) {
			(void)pcb(popped->se_node, args.a_agg);
		}
		if (popped->se_bm != NULL) {
			slablist_bm_destroy(popped->se_bm);
		}
		lg_rm_stack_elem(popped);
	}
	slablist_destroy(S, NULL);
	GRAPH_DFS_RDNT_END(g);
	return (args.a_agg);
}

/*
 * We just add to the brancher stack, but want to hide the boilerplate.
 */
void
add_as_branch(lg_graph_t *g, slablist_t *branchers, gelem_t n)
{
	/*
	 * We don't need a bookmark, because the regular stack would have the
	 * same exact bookmark. But we leave the commented code here just in
	 * case minds change.
	 */
	//slablist_bm_t *bm = edge_bm(g, n);
	//stack_elem_t *stack_elem = lg_mk_stack_elem();
	//last_pushed->se_node = n;
	//last_pushed->se_bm = bm;
	(void)g;
	selem_t s;
	s.sle_p = n.ge_p;
	(void)slablist_add(branchers, s, 0);
}

/*
 * This gives us the pointer to the last gelem_t on the branching stack stack,
 * but _doesn't_ remove it.
 */
gelem_t
last_be(slablist_t *B)
{
	selem_t slast = slablist_end(B);
	uint64_t elems = slablist_get_elems(B);
	gelem_t last;
	if (!elems) {
		last.ge_p = NULL;
		return (last);
	}
	last.ge_p = slast.sle_p;
	return (last);
}

/*
 * This is a 'branching redundant' version of DFS. It visits nodes more than
 * once, just like the vanilla redundant DFS. However it takes an extra
 * callback that indicates whether a node that's being walked over is a
 * 'branching' node. Branching nodes are placed in a parallel stack. The idea
 * is that a callback can indicate if a branch has 'failed' and instead of
 * popping to the parent, we pop to the ancestor that's a branching node. We
 * then continue DFSing down the next child (which we get to by incrementing
 * that sl_bmark).
 */
gelem_t
lg_dfs_br_rdnt_fold(lg_graph_t *g, gelem_t start, br_cb_t *brcb, pop_cb_t *pcb,
    fold_cb_t *cb, gelem_t gzero)
{
	GRAPH_DFS_RDNT_BEGIN(g);
	uint64_t nedges = slablist_get_elems(g->gr_edges);
	if (nedges == 0) {
		return (gzero);
	}

	args_t args;

	slablist_t *S;
	slablist_t *B;

	S = slablist_create("graph_dfs_stack", NULL, NULL, SL_ORDERED);
	B = slablist_create("graph_br_stack", NULL, NULL, SL_ORDERED);

	args.a_g = g;
	args.a_cb = cb;
	args.a_q = S;
	args.a_v = NULL;
	args.a_agg = gzero;

	slablist_bm_t *bm = edge_bm(g, start);
	stack_elem_t *last_pushed = lg_mk_stack_elem();
	last_pushed->se_node = start;
	last_pushed->se_bm = bm;
	/*
	 * `start` has no outgoing edges, so we just call `cb` on `start`, and
	 * return.
	 */
	int stat = 0;
	if (bm == NULL) {
		stat = cb(args.a_agg, start, &(args.a_agg));
		return (args.a_agg);
	}

	selem_t sedge;
	edge_t *e;
	w_edge_t *we;
	GRAPH_DFS_RDNT_PUSH(g, start);
	push(S, last_pushed);
	gelem_t par_pushed;
	slablist_cur(g->gr_edges, bm, &sedge);
	if (g->gr_type == GRAPH || g->gr_type == DIGRAPH) {
		e = sedge.sle_p;
		par_pushed.ge_u = e->ed_from.ge_u;
	} else {
		we = sedge.sle_p;
		par_pushed.ge_u = we->wed_from.ge_u;
	}
	stat = cb(args.a_agg, par_pushed, &(args.a_agg));
	if (stat) {
		slablist_map(S, free_stack_elem);
		slablist_destroy(S, NULL);
		GRAPH_DFS_RDNT_END(g);
		return (args.a_agg);
	}
	int is_br = brcb(par_pushed);
	if (is_br) {
		add_as_branch(g, B, par_pushed);
	}

	stack_elem_t *pushable;

try_continue:;
	while ((pushable =
	    get_pushable_rdnt(g, last_pushed)) != NULL) {
		GRAPH_DFS_RDNT_PUSH(g, pushable->se_node);
		push(S, pushable);
		last_pushed = last_se(S);
		stat = cb(args.a_agg, pushable->se_node,
		    &(args.a_agg));
		/* we've met our terminating condition */
		if (stat) {
			slablist_map(S, free_stack_elem);
			slablist_destroy(S, NULL);
			GRAPH_DFS_RDNT_END(g);
			return (args.a_agg);
		}
		is_br = brcb(pushable->se_node);
		if (is_br) {
			add_as_branch(g, B, pushable->se_node);
		}
	}

	/*
	 * We've reached the bottom. We want to pop one element off of the
	 * stack, and then we want to try to continue to DFS search. We keep
	 * doing this until the stack is empty.
	 */
pop_again:;
	int popstat = 0;
	stack_elem_t *popped = NULL;
	uint64_t depth = slablist_get_elems(S);
	GRAPH_GOT_HERE(depth);
	if (depth > 1) {
		popped = br_pop(g, S, B);
		GRAPH_DFS_RDNT_POP(g, popped->se_node);
		if (pcb != NULL) {
			popstat = pcb(popped->se_node, args.a_agg);
		}
		if (popped->se_bm != NULL) {
			slablist_bm_destroy(popped->se_bm);
		}
		lg_rm_stack_elem(popped);
		last_pushed = last_se(S);
		if (popstat == 1) {
			goto pop_again;
		} else if (popstat == 2) {
			/* pop to branch */
			/* TODO make this a separate function */
			stack_elem_t *tmp_se = last_se(S);
			gelem_t tmp_be = last_be(B);
			while (tmp_se != NULL &&
			    tmp_se->se_node.ge_p != tmp_be.ge_p) {
				popped = br_pop(g, S, B);
				GRAPH_DFS_RDNT_POP(g, popped->se_node);
				if (popped->se_bm != NULL) {
					slablist_bm_destroy(popped->se_bm);
				}
				lg_rm_stack_elem(popped);
				tmp_se = last_se(S);
			}
			last_pushed = tmp_se;
		}
		goto try_continue;
	} else if (depth == 1) {
		popped = br_pop(g, S, B);
		GRAPH_DFS_RDNT_POP(g, popped->se_node);
		if (pcb != NULL) {
			(void)pcb(popped->se_node, args.a_agg);
		}
		if (popped->se_bm != NULL) {
			slablist_bm_destroy(popped->se_bm);
		}
		lg_rm_stack_elem(popped);
	}
	slablist_destroy(S, NULL);
	GRAPH_DFS_RDNT_END(g);
	return (args.a_agg);
}

typedef struct edges_args {
	edges_cb_t	*ea_cb;
	edges_arg_cb_t	*ea_acb;
	gelem_t		ea_arg;
	slablist_t	*ea_dict;
} edges_args_t;

selem_t
graph_foldr_w_edges_cb(selem_t zero, selem_t *e, uint64_t sz)
{
	edges_args_t *a = zero.sle_p;
	gelem_t arg = a->ea_arg;
	edges_arg_cb_t *acb = a->ea_acb;
	edges_cb_t *cb = a->ea_cb;
	slablist_t *sl = a->ea_dict;
	uint64_t i = 0;
	while (i < sz) {
		int touched = slablist_add(sl, e[i], 0);
		if (!touched) {
			w_edge_t *edge = e[i].sle_p;
			gelem_t weight;
			weight = edge->wed_weight;
			if (acb != NULL) {
				acb(edge->wed_from, edge->wed_to, weight, arg);
			} else {
				cb(edge->wed_from, edge->wed_to, weight);
			}
		}
		i++;
	}
	return (zero);
}

selem_t
graph_foldr_edges_cb(selem_t zero, selem_t *e, uint64_t sz)
{
	edges_args_t *a = zero.sle_p;
	gelem_t arg = a->ea_arg;
	edges_cb_t *cb = a->ea_cb;
	edges_arg_cb_t *acb = a->ea_acb;
	slablist_t *sl = a->ea_dict;
	uint64_t i = 0;
	gelem_t weight;
	weight.ge_d = 0;
	while (i < sz) {
		int touched = slablist_add(sl, e[i], 0);
		if (!touched) {
			edge_t *edge = e[i].sle_p;
			if (acb != NULL) {
				acb(edge->ed_from, edge->ed_to, weight, arg);
			} else {
				cb(edge->ed_from, edge->ed_to, weight);
			}
		}
		i++;
	}
	return (zero);
}

selem_t
digraph_foldr_w_edges_cb(selem_t zero, selem_t *e, uint64_t sz)
{
	edges_args_t *a = zero.sle_p;
	edges_cb_t *cb = a->ea_cb;
	edges_arg_cb_t *acb = a->ea_acb;
	gelem_t arg = a->ea_arg;
	uint64_t i = 0;
	while (i < sz) {
		w_edge_t *edge = e[i].sle_p;
		gelem_t weight;
		weight = edge->wed_weight;
		if (acb != NULL) {
			acb(edge->wed_from, edge->wed_to, weight, arg);
		} else {
			cb(edge->wed_from, edge->wed_to, weight);
		}
		i++;
	}
	return (zero);
}

selem_t
digraph_foldr_edges_cb(selem_t zero, selem_t *e, uint64_t sz)
{
	edges_args_t *a = zero.sle_p;
	edges_cb_t *cb = a->ea_cb;
	edges_arg_cb_t *acb = a->ea_acb;
	gelem_t arg = a->ea_arg;
	uint64_t i = 0;
	gelem_t weight;
	weight.ge_d = 0;
	while (i < sz) {
		edge_t *edge = e[i].sle_p;
		if (acb != NULL) {
			acb(edge->ed_from, edge->ed_to, weight, arg);
		} else {
			cb(edge->ed_from, edge->ed_to, weight);
		}
		i++;
	}
	return (zero);
}

int
uniq_edge_cmp(selem_t e1, selem_t e2)
{
	edge_t *edge1 = e1.sle_p;
	edge_t *edge2 = e2.sle_p;
	edge_t tmp1;
	edge_t tmp2;
	/*
	 * By convention, we place the numerically smaller pointer/integer in
	 * the `ed_to` field while the larger one is in the `ed_from` field.
	 * This way, we effectively "deduplicate" the edges. Deduplicating the
	 * edges, causes us to walk over the same edge only once. This is the
	 * illusion we want to preserve -- that we are dealing with an
	 * undirected graph.
	 */
	if (edge1->ed_from.ge_u < edge1->ed_to.ge_u) {
		tmp1.ed_from.ge_u = edge1->ed_to.ge_u;
		tmp1.ed_to.ge_u = edge1->ed_from.ge_u;
	} else {
		tmp1.ed_from.ge_u = edge1->ed_from.ge_u;
		tmp1.ed_to.ge_u = edge1->ed_to.ge_u;
	}
	if (edge2->ed_from.ge_u < edge2->ed_to.ge_u) {
		tmp2.ed_from.ge_u = edge2->ed_to.ge_u;
		tmp2.ed_to.ge_u = edge2->ed_from.ge_u;
	} else {
		tmp2.ed_from.ge_u = edge2->ed_from.ge_u;
		tmp2.ed_to.ge_u = edge2->ed_to.ge_u;
	}
	if (tmp1.ed_from.ge_u < tmp2.ed_from.ge_u) {
		return (-1);
	}
	if (tmp1.ed_from.ge_u > tmp2.ed_from.ge_u) {
		return (1);
	}
	if (tmp1.ed_to.ge_u < tmp2.ed_to.ge_u) {
		return (-1);
	}
	if (tmp1.ed_to.ge_u > tmp2.ed_to.ge_u) {
		return (1);
	}
	return (0);
}

int
uniq_w_edge_cmp(selem_t e1, selem_t e2)
{
	w_edge_t *edge1 = e1.sle_p;
	w_edge_t *edge2 = e2.sle_p;
	w_edge_t tmp1;
	w_edge_t tmp2;
	if (edge1->wed_from.ge_u > edge1->wed_from.ge_u) {
		tmp1.wed_from.ge_u = edge1->wed_to.ge_u;
		tmp1.wed_to.ge_u = edge1->wed_from.ge_u;
	} else {
		tmp1.wed_from.ge_u = edge1->wed_from.ge_u;
		tmp1.wed_to.ge_u = edge1->wed_to.ge_u;
	}
	if (edge2->wed_from.ge_u > edge2->wed_from.ge_u) {
		tmp2.wed_from.ge_u = edge2->wed_to.ge_u;
		tmp2.wed_to.ge_u = edge2->wed_from.ge_u;
	} else {
		tmp2.wed_from.ge_u = edge2->wed_from.ge_u;
		tmp2.wed_to.ge_u = edge2->wed_to.ge_u;
	}
	if (tmp1.wed_from.ge_u < tmp2.wed_from.ge_u) {
		return (-1);
	}
	if (tmp1.wed_from.ge_u > tmp2.wed_from.ge_u) {
		return (1);
	}
	if (tmp1.wed_to.ge_u < tmp2.wed_to.ge_u) {
		return (-1);
	}
	if (tmp1.wed_to.ge_u > tmp2.wed_to.ge_u) {
		return (1);
	}
	if (edge1->wed_weight.ge_u < edge2->wed_weight.ge_u) {
		return (-1);
	}
	if (edge1->wed_weight.ge_u > edge2->wed_weight.ge_u) {
		return (1);
	}
	return (0);
}

int
uniq_edge_bnd(selem_t e, selem_t min, selem_t max)
{
	if (uniq_edge_cmp(e, min) < 0) {
		return (-1);
	}
	if (uniq_edge_cmp(e, max) > 0) {
		return (1);
	}
	return (0);
}

int
uniq_w_edge_bnd(selem_t e, selem_t min, selem_t max)
{
	if (uniq_w_edge_cmp(e, min) < 0) {
		return (-1);
	}
	if (uniq_w_edge_cmp(e, max) > 0) {
		return (1);
	}
	return (0);
}

void
lg_edges(lg_graph_t *g, edges_cb_t *cb)
{
	/*
	 * If this is just a directed graph, then it's a matter of a simple
	 * foldr.
	 */
	edges_args_t args;
	args.ea_acb = NULL;
	args.ea_cb = cb;
	args.ea_dict = NULL;
	selem_t zero;
	zero.sle_p = &args;
	switch (g->gr_type) {

	case DIGRAPH:
		slablist_foldr(g->gr_edges, digraph_foldr_edges_cb, zero);
		break;

	case GRAPH:
		/*
		 * This slab-list uses a special comparison function to single
		 * out unique edges in an undirected graph.
		 */
		args.ea_dict = slablist_create("ea_dict", uniq_edge_cmp,
		    uniq_edge_bnd, SL_SORTED);
		slablist_foldr(g->gr_edges, graph_foldr_edges_cb, zero);
		slablist_destroy(args.ea_dict, NULL);
		break;

	case DIGRAPH_WE:
		slablist_foldr(g->gr_edges, digraph_foldr_w_edges_cb, zero);
		break;

	case GRAPH_WE:
		args.ea_dict = slablist_create("ea_dict", uniq_w_edge_cmp,
		    uniq_w_edge_bnd, SL_SORTED);
		slablist_foldr(g->gr_edges, graph_foldr_w_edges_cb, zero);
		slablist_destroy(args.ea_dict, NULL);
		break;
	}
}

void
lg_edges_arg(lg_graph_t *g, edges_arg_cb_t *acb, gelem_t arg)
{
	/*
	 * If this is just a directed graph, then it's a matter of a simple
	 * foldr.
	 */
	edges_args_t args;
	args.ea_cb = NULL;
	args.ea_acb = acb;
	args.ea_arg = arg;
	args.ea_dict = NULL;
	selem_t zero;
	zero.sle_p = &args;
	switch (g->gr_type) {

	case DIGRAPH:
		slablist_foldr(g->gr_edges, digraph_foldr_edges_cb, zero);
		break;

	case GRAPH:
		/*
		 * This slab-list uses a special comparison function to single
		 * out unique edges in an undirected graph.
		 */
		args.ea_dict = slablist_create("ea_dict", uniq_edge_cmp,
		    uniq_edge_bnd, SL_SORTED);
		slablist_foldr(g->gr_edges, graph_foldr_edges_cb, zero);
		slablist_destroy(args.ea_dict, NULL);
		break;

	case DIGRAPH_WE:
		slablist_foldr(g->gr_edges, digraph_foldr_w_edges_cb, zero);
		break;

	case GRAPH_WE:
		args.ea_dict = slablist_create("ea_dict", uniq_w_edge_cmp,
		    uniq_w_edge_bnd, SL_SORTED);
		slablist_foldr(g->gr_edges, graph_foldr_w_edges_cb, zero);
		slablist_destroy(args.ea_dict, NULL);
		break;
	}
}

selem_t
digraph_foldr_flip_cb(selem_t zero, selem_t *e, uint64_t sz)
{
	uint64_t i = 0;
	while (i < sz) {
		edge_t *edge = e[i].sle_p;
		lg_connect(zero.sle_p, edge->ed_to, edge->ed_from);
		i++;
	}
	return (zero);
}

selem_t
digraph_foldr_w_flip_cb(selem_t zero, selem_t *e, uint64_t sz)
{
	uint64_t i = 0;
	while (i < sz) {
		w_edge_t *edge = e[i].sle_p;
		lg_wconnect(zero.sle_p, edge->wed_to, edge->wed_from,
		    edge->wed_weight);
		i++;
	}
	return (zero);
}

/*
 * We walk over the graph `g`, and return a new graph which is identical to
 * graph `g`, except the `to` and `from` values of the edges are swapped. This
 * has no effect on undirected graphs, as the returned graph would be identical
 * to the input graph. Flipping the edges is a useful way to derive a graph. If
 * you have a directed graph of authors -> documents, a flipped graph would
 * basically be a graph of documents -> authors. This makes it easy to switch
 * from an author-centric analysis to a document centric analysis. Another
 * example: it makes it easy to take a graph of dependencies and turn it into a
 * graph of reverse-dependencies.
 */
lg_graph_t *
lg_flip_edges(lg_graph_t *g)
{
	selem_t zero;
	lg_graph_t *g2 = NULL;
	switch (g->gr_type) {

	case GRAPH:
	case GRAPH_WE:
		return (NULL);
		break;

	case DIGRAPH:
		g2 = lg_create_digraph();
		zero.sle_p = g2;
		slablist_foldr(g->gr_edges, digraph_foldr_flip_cb, zero);
		break;

	case DIGRAPH_WE:
		g2 = lg_create_wdigraph();
		zero.sle_p = g2;
		slablist_foldr(g->gr_edges, digraph_foldr_w_flip_cb, zero);
		break;

	}
	return (g2);
}

/*
 * This function walks over the edges that are outgoing from the node `n`. It's
 * kind of like the first iteration of BFS, except it's faster, and doesn't
 * require queues or visited-sets.
 *
 * You'll notice that the implementation is similar to the lg_edges function,
 * except that it uses a ranged fold instead of a regular fold, and doesn't
 * need a special slablist undirected graphs.
 */
void
lg_neighbors(lg_graph_t *g, gelem_t n, edges_cb_t *cb)
{
	edges_args_t args;
	args.ea_acb = NULL;
	args.ea_cb = cb;
	args.ea_dict = NULL;
	selem_t zero;
	zero.sle_p = &args;

	edge_t emin;
	edge_t emax;
	selem_t s_emin;
	selem_t s_emax;
	s_emin.sle_p = &emin;
	s_emax.sle_p = &emax;

	w_edge_t wemin;
	w_edge_t wemax;
	selem_t s_wemin;
	selem_t s_wemax;
	s_wemin.sle_p = &wemin;
	s_wemax.sle_p = &wemax;

	switch (g->gr_type) {

	case DIGRAPH:
	case GRAPH:
		emin.ed_from.ge_u = n.ge_u;
		emin.ed_to.ge_u = 0;
		emax.ed_from.ge_u = n.ge_u;
		emax.ed_to.ge_u = UINT64_MAX;
		slablist_foldr_range(g->gr_edges, digraph_foldr_edges_cb,
		    s_emin, s_emax, zero);
		break;


	case DIGRAPH_WE:
	case GRAPH_WE:
		wemin.wed_from.ge_u = n.ge_u;
		wemin.wed_to.ge_u = 0;
		wemin.wed_weight.ge_u = 0;
		wemax.wed_from.ge_u = n.ge_u;
		wemax.wed_to.ge_u = UINT64_MAX;
		wemax.wed_weight.ge_u = UINT64_MAX;
		slablist_foldr_range(g->gr_edges, digraph_foldr_w_edges_cb,
		    s_wemin, s_wemax, zero);
		break;
	}
}

/*
 * This function is just like lg_neighbors, except that it accepts and passes
 * an arg to the callback.
 */
void
lg_neighbors_arg(lg_graph_t *g, gelem_t n, edges_arg_cb_t *cb, gelem_t arg)
{
	edges_args_t args;
	args.ea_acb = cb;
	args.ea_cb = NULL;
	args.ea_dict = NULL;
	args.ea_arg = arg;
	selem_t zero;
	zero.sle_p = &args;

	edge_t emin;
	edge_t emax;
	selem_t s_emin;
	selem_t s_emax;
	s_emin.sle_p = &emin;
	s_emax.sle_p = &emax;

	w_edge_t wemin;
	w_edge_t wemax;
	selem_t s_wemin;
	selem_t s_wemax;
	s_wemin.sle_p = &wemin;
	s_wemax.sle_p = &wemax;

	switch (g->gr_type) {

	case DIGRAPH:
	case GRAPH:
		emin.ed_from.ge_u = n.ge_u;
		emin.ed_to.ge_u = 0;
		emax.ed_from.ge_u = n.ge_u;
		emax.ed_to.ge_u = UINT64_MAX;
		slablist_foldr_range(g->gr_edges, digraph_foldr_edges_cb,
		    s_emin, s_emax, zero);
		break;


	case DIGRAPH_WE:
	case GRAPH_WE:
		wemin.wed_from.ge_u = n.ge_u;
		wemin.wed_to.ge_u = 0;
		wemin.wed_weight.ge_u = 0;
		wemax.wed_from.ge_u = n.ge_u;
		wemax.wed_to.ge_u = UINT64_MAX;
		wemax.wed_weight.ge_u = UINT64_MAX;
		slablist_foldr_range(g->gr_edges, digraph_foldr_w_edges_cb,
		    s_wemin, s_wemax, zero);
		break;
	}

}

int
snap_cmp(selem_t e1, selem_t e2)
{
	change_t *c1 = e1.sle_p;
	change_t *c2 = e2.sle_p;

	if (c1->ch_snap > c2->ch_snap) {
		return (1);
	}
	if (c1->ch_snap < c2->ch_snap) {
		return (-1);
	}
	if (c1->ch_op > c2->ch_op) {
		return (1);
	}
	if (c1->ch_op < c2->ch_op) {
		return (-1);
	}
	if (c1->ch_from.ge_u > c2->ch_from.ge_u) {
		return (1);
	}
	if (c1->ch_from.ge_u < c2->ch_from.ge_u) {
		return (-1);
	}
	if (c1->ch_to.ge_u > c2->ch_to.ge_u) {
		return (1);
	}
	if (c1->ch_to.ge_u < c2->ch_to.ge_u) {
		return (-1);
	}
	if (c1->ch_weight.ge_u > c2->ch_weight.ge_u) {
		return (1);
	}
	if (c1->ch_weight.ge_u < c2->ch_weight.ge_u) {
		return (-1);
	}
	return (0);
}

int
snap_bnd(selem_t e, selem_t min, selem_t max)
{
	int c = snap_cmp(e, min);
	if (c < 0) {
		return (-1);
	}
	c = snap_cmp(e, max);
	if (c > 0) {
		return (1);
	}
	return (0);
}

/*
 * The snap_cb is called on every edge that is affected by a change.
 * This is typically used to increase or decrease a reference count in structs
 * that are pointed-to by the gelem_t's of the edge -- and to free them if that
 * count drops to 0. Any node/struct cannot be cleaned up by a libgraph
 * consumer, unless it is not present in any snapshots. This callback
 * facilitates the necessary cooperation between the consumer and libgraph to
 * accomplish this.
 */
void
lg_snapshot_cb(lg_graph_t *g, snap_cb_t cb)
{
	g->gr_snap_cb = cb;
}

/*
 * Takes a snapshot of a graph and returns an integer representing the
 * snapshot.
 */
uint64_t
lg_snapshot(lg_graph_t *g)
{
	if (g->gr_snapstrat != SNAP_FAST && g->gr_snapstrat != SNAP_DEDUP) {
		g->gr_snapstrat = SNAP_FAST;
	}
	if (g->gr_snaps == NULL) {
		switch (g->gr_snapstrat) {

		case SNAP_FAST:
			g->gr_snaps = slablist_create("snapshots", NULL, NULL,
					SL_ORDERED);
			break;
		case SNAP_DEDUP:
			g->gr_snaps = slablist_create("snapshots", snap_cmp,
					snap_bnd, SL_SORTED);
			break;
		}
	}
	uint64_t r = g->gr_snap;
	g->gr_snap++;
	return (r);
}

void
lg_snapstrat(lg_graph_t *g, snap_strat_t s)
{
	g->gr_snapstrat = s;
}

/*
 * Creates a clone of graph `g` at snapshot `snap`. To create a clone, we
 * create an identical copy of `g`. On this copy, we carry out a rollback
 * (using the snapshot data stored in `g`, so that we save space). Unlike
 * clones in other systems like ZFS, cloned graphs do _not_ occupy any less
 * space than identical but uncloned graphs.
 */
lg_graph_t *
lg_clone(lg_graph_t *g, uint64_t snap)
{
	if (snap >= g->gr_snap || g->gr_snaps == NULL) {
		return (NULL);
	}
	/* XXX temporary to stop compiler warnings */
	lg_graph_t *c = g;
	return (c);
}

selem_t
rollback_fold_fast(selem_t z, selem_t *e, uint64_t sz)
{
	lg_graph_t *g = z.sle_p;
	gelem_t ignored;
	uint64_t i = sz - 1;
	uint64_t j = 0;
	while (j < sz) {
		change_t *c = e[i].sle_p;
		GRAPH_ROLLBACK_CHANGE(g, c);
		if (c->ch_snap >= g->gr_rollback_to) {
			switch (c->ch_op) {

			case CONNECT:
				switch (g->gr_type) {

				case DIGRAPH:
				case GRAPH:
					lg_disconnect(g, c->ch_from, c->ch_to);
					g->gr_snap_cb(0, EDGE, c->ch_from, c->ch_to,
					    ignored);
					break;

				case DIGRAPH_WE:
				case GRAPH_WE:
					lg_wdisconnect(g, c->ch_from,
					    c->ch_to, c->ch_weight);
					g->gr_snap_cb(0, EDGE, c->ch_from, c->ch_to,
					    c->ch_weight);
					break;
				}
				break;

			case DISCONNECT:
				switch (g->gr_type) {

				case DIGRAPH:
				case GRAPH:
					lg_connect(g, c->ch_from, c->ch_to);
					g->gr_snap_cb(1, EDGE, c->ch_from, c->ch_to,
					    ignored);
					break;

				case DIGRAPH_WE:
				case GRAPH_WE:
					lg_wconnect(g, c->ch_from,
					    c->ch_to, c->ch_weight);
					g->gr_snap_cb(1, EDGE, c->ch_from, c->ch_to,
					    c->ch_weight);
					break;
				}
				break;
			}
			g->gr_chs_rolled++;
		}
		i--;
		j++;
	}
	return (z);
}

selem_t
rollback_fold_dedup(selem_t z, selem_t *e, uint64_t sz)
{
	(void)z; (void)e; (void)sz;
	return (z);
}

static void
clean_change(selem_t e)
{
	change_t *c = e.sle_p;
	lg_graph_t *g = c->ch_graph;
	gelem_t ignored;
	if (g->gr_snap_cb) {
		g->gr_snap_cb(0, SNAP, c->ch_from, c->ch_to, ignored);
	}
	lg_rm_change(c);
}

/*
 * Rolls the graph back to snapshot `snap`. Given an ordered slablist of
 * changes, we do a foldl, and while the `ch_snap` value is >= `snap` we undo
 * whatever operation is described by the change.
 */
int
lg_rollback_fast(lg_graph_t *g, uint64_t snap)
{

	g->gr_chs_rolled = 0;

	g->gr_rollback_to = snap;

	selem_t zero;
	zero.sle_p = g;
	slablist_foldl(g->gr_snaps, rollback_fold_fast, zero);
	uint64_t nch = slablist_get_elems(g->gr_snaps);
	uint64_t nch_left = nch - g->gr_chs_rolled;
	uint64_t rems = g->gr_chs_rolled;
	selem_t null;
	null.sle_p = NULL;
	while (rems > 0) {
		slablist_rem(g->gr_snaps, null, nch_left, clean_change);
		rems--;
	}
	return (0);
}

int
lg_rollback_dedup(lg_graph_t *g, uint64_t snap)
{
	g->gr_rollback_to = snap;
	change_t ch_min;
	change_t ch_max;
	ch_min.ch_snap = snap;
	ch_max.ch_snap = g->gr_snap - 1;
	ch_min.ch_op = CONNECT;
	ch_max.ch_op = DISCONNECT;
	ch_min.ch_from.ge_u = 0;
	ch_max.ch_from.ge_u = UINT64_MAX;
	ch_min.ch_to.ge_u = 0;
	ch_max.ch_to.ge_u = UINT64_MAX;
	ch_min.ch_weight.ge_u = 0;
	ch_max.ch_weight.ge_u = UINT64_MAX;

	selem_t s_min;
	selem_t s_max;
	s_min.sle_p = &ch_min;
	s_max.sle_p = &ch_max;

	selem_t zero;
	zero.sle_p = g;
	slablist_foldl_range(g->gr_snaps, rollback_fold_dedup, s_min, s_max,
	    zero);
	slablist_rem_range(g->gr_snaps, s_min, s_max, clean_change);
	return (0);
}

int
lg_rollback(lg_graph_t *g, uint64_t snap)
{
	if (snap >= g->gr_snap || g->gr_snaps == NULL) {
		return (-1);
	}
	int r = 0;
	g->gr_rollingback = 1;

	switch (g->gr_snapstrat) {

	case SNAP_FAST:
		r = lg_rollback_fast(g, snap);
		break;
	case SNAP_DEDUP:
		/* TODO implement a DEDUP strategy */
		r = lg_rollback_dedup(g, snap);
		break;
	}
	g->gr_rollingback = 0;
	return (r);
}

/*
 * To destroy a snapshot, we move all changes into the previous snapshot. For
 * the fast snap-strat all this means is that you'll be unable to rollback or
 * clone the snapshot you destroyed, but the changes are all still there. For
 * the dedup-strat, you may get some memory savings if there are changes common
 * to both snapshots.
 */
int
lg_destroy_snapshot(lg_graph_t *g, uint64_t snap)
{
	(void)g;
	(void)snap;
	return (0);
}

/*
 * We destroy all changes associated with snapshots, reset the snapshot counter
 * to zero.
 */
int
lg_destroy_all_snapshots(lg_graph_t *g)
{
	(void)g;
	return (0);
}

/*
 * Count the number of edges in the graph.
 */
uint64_t
lg_nedges(lg_graph_t *g)
{
	if (g->gr_edges) {
		return (slablist_get_elems(g->gr_edges));
	}
	return (0);
}

typedef struct flatten_cookie {
	slablist_t	*fck_chs;
	flatten_cb_t	*fck_cb;
	gelem_t		fck_node;
	gelem_t		fck_arg;
	uint64_t	fck_connects;
} flatten_cookie_t;

static int
flatten_fold(gelem_t agg, gelem_t node, gelem_t *aggp)
{
	(void)agg;
	(void)aggp;
	(void)node;
	return (0);
}

#define FLATTEN_PROMOTE 1
#define FLATTEN_DROP -1
#define FLATTEN_KEEP 0


/*
 * The callback, if implemented stupidly, can split the graph into more than
 * one connected component. In general it should return DROP when it wants to
 * break a connection between from and to. This implies that the child of the
 * DROPPED node should be PROMOTED unless it too is going to be DROPPED.
 * Otherwise the graph may have nodes that are unreachable from the relative
 * root supplied to lg_flatten().
 */
void
flatten_adj(gelem_t to, gelem_t from, gelem_t weight, gelem_t agg)
{
	flatten_cookie_t *ck = agg.ge_p;
	int pdk = ck->fck_cb(ck->fck_node, to, ck->fck_arg);
	change_t *c = NULL;
	selem_t sc;
	switch (pdk) {

	case FLATTEN_PROMOTE:
		c = lg_mk_change();
		/*
		 * If not already connected to parent, queue for connection.
		 */
		if (from.ge_u != ck->fck_node.ge_u) {
			ck->fck_connects++;
			c->ch_op = CONNECT;
			c->ch_from = ck->fck_node;
			c->ch_to = to;
			c->ch_weight.ge_u = ck->fck_connects;
			sc.sle_p = c;
			slablist_add(ck->fck_chs, sc, 0);
			c = lg_mk_change();
			c->ch_op = DISCONNECT;
			c->ch_from = from;
			c->ch_to = to;
			c->ch_weight.ge_u = weight.ge_u;
			sc.sle_p = c;
			slablist_add(ck->fck_chs, sc, 0);
		}
		break;

	case FLATTEN_DROP:
		c = lg_mk_change();
		c->ch_op = DISCONNECT;
		c->ch_from = from;
		c->ch_to = to;
		c->ch_weight.ge_u = weight.ge_u;
		sc.sle_p = c;
		slablist_add(ck->fck_chs, sc, 0);
		break;

	case FLATTEN_KEEP:
		/* We don't break any connections */
		break;
	}
}

selem_t
flatten_apply_changes(selem_t agg, selem_t *e, uint64_t sz)
{
	lg_graph_t *g = agg.sle_p;
	uint64_t i = 0;
	/*
	 * We branch here instead of inside the loop, because that would make
	 * the loop slower.
	 */
	if (g->gr_type == GRAPH || g->gr_type == DIGRAPH) {
		while (i < sz) {
			change_t *c = e[i].sle_p;
			switch (c->ch_op) {

			case CONNECT:
				lg_connect(g, c->ch_from, c->ch_to);
				break;

			case DISCONNECT:
				lg_disconnect(g, c->ch_from, c->ch_to);
				break;
			}
			i++;
		}
	} else {
		while (i < sz) {
			change_t *c = e[i].sle_p;
			switch (c->ch_op) {

			case CONNECT:
				lg_wconnect(g, c->ch_from, c->ch_to,
				    c->ch_weight);
				break;

			case DISCONNECT:
				lg_wdisconnect(g, c->ch_from, c->ch_to,
				    c->ch_weight);
				break;
			}
			i++;
		}
	}
	return (agg);
}

void
rm_change_cb(selem_t c)
{
	change_t *ch = c.sle_p;
	lg_rm_change(ch);
}

/*
 * Flattening a node's descendants essentially turns some or all of its
 * descendants into children. Those that do not become children, become
 * eliminated. The consumer-supplied cb is responsible for telling the routine
 * which nodes will become children, and for destroying any nodes if necessary.
 *
 * The flatten routine walks the subtree using BFS. We can't change the graph
 * while doing a BFS, so we instead record a sequence of change_t's that we
 * will apply after the BFS.
 */
void
lg_flatten(lg_graph_t *g, gelem_t node, flatten_cb_t *cb, gelem_t arg)
{
	flatten_cookie_t cookie;

	slablist_t *chs = slablist_create("flatten_changes", NULL, NULL,
	    SL_ORDERED);

	cookie.fck_chs = chs;
	cookie.fck_cb = cb;
	cookie.fck_node.ge_u = node.ge_u;
	cookie.fck_arg.ge_u = arg.ge_u;
	cookie.fck_connects = 0;

	gelem_t gcookie;
	gcookie.ge_p = &cookie;

	lg_bfs_fold(g, node, flatten_adj, flatten_fold, gcookie);
	selem_t sgr;
	sgr.sle_p = g;
	slablist_foldr(chs, flatten_apply_changes, sgr);
	slablist_destroy(chs, rm_change_cb);
}

/*
 * We touch all of the nodes in a graph, and ask `cb` if it should be dropped
 * (along with all of its descendants). If so, we queue the node up. We then do
 * a BFS starting from all of the queued nodes, and accumulate a list of
 * change_t's. We then apply all of the change_t's to the graph.
 */
void
lg_drop(lg_graph_t *g, drop_cb_t *cb, drop_strat_t s)
{

}

/*
 * Create an identical copy of the graph `g`, and return it. Note that we only
 * copy edges, and the user is responsible for handling how structs pointed to
 * from those edges are copied.
 */
lg_graph_t *
lg_copy(lg_graph_t *g)
{

}
