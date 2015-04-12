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
	if (edge1->wed_weight.ge_u < edge2->wed_weight.ge_u) {
		return (-1);
	}
	if (edge1->wed_weight.ge_u > edge2->wed_weight.ge_u) {
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
		if (type == DIGRAPH || type == GRAPH) {
			edge = c.sle_p;
			from.ge_u = edge->ed_from.ge_u;
			enq.sle_u = edge->ed_to.ge_u;
		} else {
			w_edge = c.sle_p;
			from.ge_u = w_edge->wed_from.ge_u;
			enq.sle_u = w_edge->wed_to.ge_u;
		}
		if (!visited(V, enq)) {
			gelem_t to;
			to.ge_u = enq.sle_u;
			if (args->a_acb != NULL) {
				args->a_acb(to, from, args->a_agg);
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
		if (type == DIGRAPH || type == GRAPH) {
			edge = c.sle_p;
			from.ge_u = edge->ed_from.ge_u;
			enq.sle_u = edge->ed_to.ge_u;
		} else {
			w_edge = c.sle_p;
			from.ge_u = w_edge->wed_from.ge_u;
			enq.sle_u = w_edge->wed_to.ge_u;
		}
		gelem_t to;
		to.ge_u = enq.sle_u;
		if (args->a_acb != NULL) {
			args->a_acb(to, from, args->a_agg);
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

		w_max.wed_from = origin;
		w_max.wed_to = max_to;

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
stack_elem_t *
pop(lg_graph_t *g, slablist_t *S)
{
	selem_t slast = slablist_end(S);
	uint64_t elems = slablist_get_elems(S);
	slablist_rem(S, ignored, (elems - 1), NULL);
	stack_elem_t *last;
	last = slast.sle_p;
	stack_elem_t *popped_elem = last;
	GRAPH_DFS_POP(g, popped_elem->se_node);
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
				slablist_destroy(Q);
				slablist_destroy(V);
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
	slablist_destroy(Q);
	slablist_destroy(V);
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
				slablist_destroy(Q);
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
	slablist_destroy(Q);
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
		slablist_destroy(S);
		slablist_destroy(V);
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
			slablist_destroy(S);
			slablist_destroy(V);
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
	stack_elem_t *popped = NULL;
	uint64_t depth = slablist_get_elems(S);
	if (depth > 1) {
		popped = pop(g, S);
		pcb(popped->se_node, args.a_agg);
		slablist_bm_destroy(popped->se_bm);
		lg_rm_stack_elem(popped);
		last_pushed = last_se(S);
		goto try_continue;
	} else if (depth == 1) {
		popped = pop(g, S);
		pcb(popped->se_node, args.a_agg);
		slablist_bm_destroy(popped->se_bm);
		lg_rm_stack_elem(popped);
	}
	slablist_destroy(S);
	slablist_destroy(V);
	GRAPH_DFS_END(g);
	return (args.a_agg);
}

typedef struct edges_args {
	edges_cb_t	*ea_cb;
	slablist_t	*ea_dict;
} edges_args_t;

selem_t
graph_foldr_w_edges_cb(selem_t zero, selem_t *e, uint64_t sz)
{
	edges_args_t *a = zero.sle_p;
	edges_cb_t *cb = a->ea_cb;
	slablist_t *sl = a->ea_dict;
	uint64_t i = 0;
	while (i < sz) {
		int touched = slablist_add(sl, e[i], 0);
		if (!touched) {
			w_edge_t *edge = e[i].sle_p;
			gelem_t weight;
			weight = edge->wed_weight;
			cb(edge->wed_from, edge->wed_to, weight);
		}
		i++;
	}
	return (zero);
}

selem_t
graph_foldr_edges_cb(selem_t zero, selem_t *e, uint64_t sz)
{
	edges_args_t *a = zero.sle_p;
	edges_cb_t *cb = a->ea_cb;
	slablist_t *sl = a->ea_dict;
	uint64_t i = 0;
	gelem_t weight;
	weight.ge_d = 0;
	while (i < sz) {
		int touched = slablist_add(sl, e[i], 0);
		if (!touched) {
			edge_t *edge = e[i].sle_p;
			cb(edge->ed_from, edge->ed_to, weight);
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
	uint64_t i = 0;
	while (i < sz) {
		w_edge_t *edge = e[i].sle_p;
		gelem_t weight;
		weight = edge->wed_weight;
		cb(edge->wed_from, edge->wed_to, weight);
		i++;
	}
	return (zero);
}

selem_t
digraph_foldr_edges_cb(selem_t zero, selem_t *e, uint64_t sz)
{
	edges_args_t *a = zero.sle_p;
	edges_cb_t *cb = a->ea_cb;
	uint64_t i = 0;
	gelem_t weight;
	weight.ge_d = 0;
	while (i < sz) {
		edge_t *edge = e[i].sle_p;
		cb(edge->ed_from, edge->ed_to, weight);
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
		slablist_destroy(args.ea_dict);
		break;

	case DIGRAPH_WE:
		slablist_foldr(g->gr_edges, digraph_foldr_w_edges_cb, zero);
		break;

	case GRAPH_WE:
		args.ea_dict = slablist_create("ea_dict", uniq_w_edge_cmp,
		    uniq_w_edge_bnd, SL_SORTED);
		slablist_foldr(g->gr_edges, graph_foldr_w_edges_cb, zero);
		slablist_destroy(args.ea_dict);
		break;
	}
}
