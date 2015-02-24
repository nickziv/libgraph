/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright (c) 2015, Nick Zivkovic
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
	if (edge1->wed_to.ge_u < edge2->wed_to.ge_u) {
		return (-1);
	}
	if (edge1->wed_to.ge_u > edge2->wed_to.ge_u) {
		return (1);
	}
	if (edge1->wed_weight < edge2->wed_weight) {
		return (-1);
	}
	if (edge1->wed_weight > edge2->wed_weight) {
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

int
lg_connect(lg_graph_t *g, gelem_t from, gelem_t to)
{
	selem_t se1;
	selem_t se2;
	edge_t *e1;
	edge_t *e2;
	int r;
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
	return (0);
}

int
lg_wconnect(lg_graph_t *g, gelem_t from, gelem_t to, double weight)
{
	selem_t swe1;
	selem_t swe2;
	w_edge_t *we1;
	w_edge_t *we2;
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
	return (0);
}

typedef struct args {
	fold_cb_t		*a_cb;
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
		if (type == DIGRAPH || type == GRAPH) {
			edge = c.sle_p;
			enq.sle_u = edge->ed_to.ge_u;
		} else {
			w_edge = c.sle_p;
			enq.sle_u = w_edge->wed_to.ge_u;
		}
		if (!visited(V, enq)) {
			slablist_add(Q, enq, 0);
			slablist_add(V, enq, 0);
		}
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
	edge_t *min = NULL;
	edge_t *max = NULL;
	w_edge_t *w_min = NULL;
	w_edge_t *w_max = NULL;

	if (g->gr_type == GRAPH || g->gr_type == DIGRAPH) {
		min = lg_mk_edge();
		max = lg_mk_edge();

		min->ed_from = origin;
		min->ed_to = min_to;

		max->ed_from = origin;
		max->ed_to = max_to;

		min_edge.sle_p = min;
		max_edge.sle_p = max;
	} else {
		w_min = lg_mk_w_edge();
		w_max = lg_mk_w_edge();

		w_min->wed_from = origin;
		w_min->wed_to = min_to;

		w_max->wed_from = origin;
		w_max->wed_to = max_to;

		min_edge.sle_p = w_min;
		max_edge.sle_p = w_max;
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
 * Sometimes we just want a enqueue a single node (usually the start-node).
 */
void
enq_origin(slablist_t *Q, slablist_t *V, gelem_t origin)
{
	GRAPH_GOT_HERE(0);
	selem_t enq;
	enq.sle_u = origin.ge_u;
	slablist_add(Q, enq, 0);
	slablist_add(V, enq, 0);
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
 * Similar in priciple to enq_origin, but we push a bookmark which is pointing
 * to the first 'edge' that connects the origin to another gelem. 
 */
void
push_bm(slablist_t *S, slablist_bm_t *bm)
{
	selem_t push;
	push.sle_p = bm;
	slablist_add(S, push, 0);
}

/*
 * This gives us the pointer to the last bmark on the stack, but _doesn't_
 * remove it.
 */
slablist_bm_t *
last_bm(slablist_t *S)
{
	selem_t slast = slablist_end(S);
	uint64_t elems = slablist_get_elems(S);
	if (!elems) {
		return (NULL);
	}
	slablist_bm_t *last;
	last = slast.sle_p;
	return (last);
}

/*
 * Returns the last bookmark, and pops it.
 */
slablist_bm_t *
pop(slablist_t *S)
{
	selem_t slast = slablist_end(S);
	uint64_t elems = slablist_get_elems(S);
	slablist_rem(S, ignored, (elems - 1), NULL);
	slablist_bm_t *last;
	last = slast.sle_p;
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
 * So this function executes the BFS algorithm above, however it takes a
 * callback: the callback aggregates over the algorithm and indicates if the
 * function should terminate its search.
 *
 * This aggregation can be used to compute arbitrary things like shortest path,
 * centrality values, and so on.
 */
gelem_t
lg_bfs_fold(lg_graph_t *g, gelem_t start, fold_cb_t *cb, gelem_t gzero)
{
	/*
	 * First we create an appropriate queue and visited-set.
	 */
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
	args.a_q = Q;
	args.a_v = V;
	args.a_agg = gzero;
	/*
	 * Here we execute BFS. We enqueue all the nodes connected to start,
	 * and we loop through BFS, until we reach the terminating condition --
	 * or visit all of the nodes.
	 */
	enq_origin(Q, V, start);
	while (slablist_get_elems(Q) > 0) {
		gelem_t last = deq(Q);
		int stat = cb(args.a_agg, last, &(args.a_agg));
		if (stat) {
			/*
			 * The user should save what he's looking for in a_agg.
			 */
			slablist_destroy(Q);
			slablist_destroy(V);
			return (args.a_agg);
		}
		enq_connected(g, last, zero);
	}
	slablist_destroy(Q);
	slablist_destroy(V);
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
		end_we.wed_from = start;
		end_we.wed_to.ge_u = UINT64_MAX;
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
 */
slablist_bm_t *
get_pushable(lg_graph_t *g, slablist_t *V, slablist_bm_t *last,
    gelem_t *par_pushed)
{
	slablist_t *edges = g->gr_edges;
	selem_t curelem;
	slablist_cur(edges, last, &curelem);
	edge_t *e;
	w_edge_t *we;
	gelem_t unvis_child;
	int par = 0;
	gelem_t parent;
	/*
	 * We try to find a child that wasn't visited.
	 */
	if (g->gr_type == GRAPH || g->gr_type == DIGRAPH) {
		do {
			e = curelem.sle_p;
			/*
			 * If we've run out of children, we bail.
			 */
			if (par && parent.ge_u != e->ed_from.ge_u) {
				return (NULL);
			}
			parent.ge_u = e->ed_from.ge_u;
			par = 1;
			unvis_child.ge_u = e->ed_to.ge_u;
			int end = slablist_next(edges, last, &curelem);
			/*
			 * We've run out of edges, so we bail.
			 */
			if (end) {
				return (NULL);
			}
		} while (gvisited(V, unvis_child));
	} else {
		do {
			we = curelem.sle_p;
			/*
			 * If we've run out of children we bail.
			 */
			if (par && parent.ge_u != we->wed_from.ge_u) {
				slablist_prev(edges, last, &curelem);
				return (NULL);
			}
			parent.ge_u = we->wed_from.ge_u;
			par = 1;
			unvis_child.ge_u = we->wed_to.ge_u;
			int end = slablist_next(edges, last, &curelem);
			if (end) {
				return (NULL);
			}
		} while (gvisited(V, unvis_child));
	}
	par_pushed->ge_u = unvis_child.ge_u;
	slablist_bm_t *pushable = edge_bm(g, unvis_child);
	return (pushable);
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
lg_dfs_fold(lg_graph_t *g, gelem_t start, fold_cb_t *cb, gelem_t gzero)
{
	args_t args;
	/*
	 * So we don't really need the zero selem?
	 */
	slablist_t *S;
	slablist_t *V;
	if (g->gr_type == GRAPH || g->gr_type == GRAPH_WE) {
	}

	S = slablist_create("graph_dfs_stack", NULL, NULL, SL_ORDERED);
	V = slablist_create("graph_dfs_vset", gelem_cmp, gelem_bnd, SL_SORTED);

	args.a_g = g;
	args.a_cb = cb;
	args.a_q = S;
	args.a_v = V;
	args.a_agg = gzero;

	slablist_bm_t *bm = edge_bm(g, start);
	slablist_bm_t *last_pushed = bm;
	/*
	 * We can't do DFS because this node has no edges.
	 */
	if (bm == NULL) {
		return (gzero);
	}

	int stat = 0;
	selem_t sedge;
	edge_t *e;
	w_edge_t *we;
	push_bm(S, bm);
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
		slablist_destroy(S);
		slablist_destroy(V);
		return (args.a_agg);
	}
	selem_t visited;
	visited.sle_u = par_pushed.ge_u;
	slablist_add(V, visited, 0);
	slablist_bm_t *pushable;

try_continue:;
	while ((pushable =
	    get_pushable(g, V, last_pushed, &par_pushed)) != NULL) {
		push_bm(S, pushable);
		last_pushed = last_bm(S);
		stat = cb(args.a_agg, par_pushed,
		    &(args.a_agg));
		/* we've met our terminating condition */
		if (stat) {
			slablist_destroy(S);
			slablist_destroy(V);
			return (args.a_agg);
		}
		visited.sle_u = par_pushed.ge_u;
		slablist_add(V, visited, 0);
	}
	/*
	 * We've reached the bottom. We want to pop one element off of the
	 * stack, and then we want to try to continue to DFS search. We keep
	 * doing this until the stack is empty.
	 */
	uint64_t depth = slablist_get_elems(S);
	if (depth > 1) {
		(void)pop(S);
		last_pushed = last_bm(S);
		goto try_continue;
	} else if (depth == 1) {
		(void)pop(S);
	}
	slablist_destroy(S);
	slablist_destroy(V);
	return (args.a_agg);
}
