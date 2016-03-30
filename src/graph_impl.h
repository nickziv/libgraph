/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright (c) 2015, Nick Zivkovic
 */

#include "graph.h"
#include <slablist.h>
#include <stdio.h>

/*
 * The user can create 4 kinds of graphs:
 *
 *	- A directed graph
 *	- An undirected graph
 *	- A directed graph with edge-weights
 *	- An undirected graph with edge-weights
 */
typedef enum gtype {
	DIGRAPH, /* directed graph */
	GRAPH, /* undirected graph */
	DIGRAPH_WE, /* directed graph w/ weighted edges */
	GRAPH_WE /* undirected graph w/ weighted edges */
} gtype_t;

/*
 * The graph is implemented as a list of edges. There are 2 types of edges,
 * weighted and unweighted. An unweighted edge is 128 bits in size, while a
 * weighted edge is 192 bits in size. Each node takes up 64 bits, and the
 * weight itself is a 64-bits. The 64 bits that represent each node and the
 * wieght are encapsulated in a union called the `gelem_t`. Which is short for
 * graph-element. This means that nodes themselves can be pointers, doubles,
 * signed/unsinged integers, 8-byte char-arrays and so on.  The library doesn't
 * really need to know anything about the node. Only which node points to
 * which. Nodes are identified by their bits. So if one is storing pointers to
 * structs in the gelem's, then 2 identical structs with different pointers
 * will be interpreted as being different.
 *
 * The edges are stored in a sorted slablist. They are sorted by first
 * comparing the `from` edges as unsinged integers, and then comparing the `to`
 * edges as unsigned integers. This is similar to how people compare 2-digit
 * numbers: if the left digits are equal, then compare the right digits. If one
 * left digit is greater than the other, there is no need to compare the right
 * digits.
 *
 * Weighted edges are compared just like unweighted edges, except we compare
 * the double, too. So it's like a 3-digit number, the double is the right-most
 * digit.
 *
 * Both directed and undirected graphs are stored in the same way. The only
 * difference is that when we connect FOO to BAR in an undirected graph, we
 * connect FOO -> BAR and BAR -> FOO, implicitly.
 *
 * In other words: under the hood, undirected graphs are basically directed
 * graphs, in which the edges must point in both directions, and must have the
 * same weight in both directions.
 *
 * This representation allows us to use libslablist's ranged_fold feature to
 * visit all the neighbors of any single node. Which, if done repeatedly, is
 * basically a BFS. Naturally, libgraph takes care of the messy details behind
 * such a BFS implementation, and provides a convenient function for doing BFS.
 *
 * DFS is facilitated by libslablist's bookmark feature, which allows one to
 * iteratively visit elements (edges) in sequence.
 */
typedef struct edge {
	gelem_t		ed_from;
	gelem_t		ed_to;
} edge_t;

typedef struct w_edge {
	gelem_t		wed_from;
	gelem_t		wed_to;
	gelem_t		wed_weight;
} w_edge_t;

typedef enum graph_op {
	CONNECT,
	DISCONNECT
} graph_op_t;

/*
 * This represents a change to a graph, after a snapshot was taken. All changes
 * act on edges and either involve the creation of a new edge or destruction of
 * an old one. A change with `ch_snap=S`, means that `ch_op` was done on edge
 * `ch_edge` after snapshot S was taken.
 */
typedef struct change {
	uint64_t	ch_snap;
	graph_op_t	ch_op;
	lg_graph_t	*ch_graph;
	gelem_t		ch_from;
	gelem_t		ch_to;
	gelem_t		ch_weight;
} change_t;

/*
 * The graph is essentially a slablist of edges. It also contains an integer
 * representing the current generation or snapshot. Snapshotting of graphs can
 * be implemented in multiple ways. The user can specify the implementation by
 * setting the gr_snapstrat member. There currently two strategies:
 *
 * 	-FAST
 * 	-DEDUP
 *
 * The FAST strategy implements the snapshot as a simple list of change_t's,
 * where the change_t is appended on every change. This strategy induced the
 * lowest overhead on the lg_[w]connect/lg_[w]disconnect operations. However,
 * it will store duplicate changes (i.e. if you add/remove the same edge
 * multiple times). It will also walk the entire change-list from left to
 * right, when cloning or rolling back.
 *
 * The DEDUP strategy is like the above, except that the changes are stored as
 * a sorted set, which results in higher overhead (i.e. O(logN) insertions instead
 * of O(1)). On the flip side, it uses the minimal amount of memory, and can do
 * a ranged left fold, which means it will only walk the part of the list that
 * it has to. TODO: This is not implemented yet.
 *
 * FAST is good for use on graphs with small amounts of changes. DEDUP is good
 * for use on graps with large amounts of changes.
 */
struct lg_graph {
	gtype_t		gr_type;
	snap_strat_t	gr_snapstrat;
	uint64_t	gr_snap;
	uint8_t		gr_rollingback; /* bool */
	uint64_t	gr_rollback_to;
	uint64_t	gr_chs_rolled;
	snap_cb_t	*gr_snap_cb;
	slablist_t	*gr_edges;
	slablist_t	*gr_snaps;
};

/*
 * This structure is used to implement the stack for DFS.
 *
 * TODO: Describe DFS algorithm, in terms of our slablist-backed graph, and why
 * the bookmark is and so on.
 */
typedef struct stack_elem {
	int		se_end;
	gelem_t		se_node;
	slablist_bm_t	*se_bm;
} stack_elem_t;

typedef void *bfs_fold_cb_t(void *);
typedef void bfs_map_cb_t(void *);

lg_graph_t *lg_mk_graph();
void lg_rm_graph(lg_graph_t *);
edge_t *lg_mk_edge();
stack_elem_t *lg_mk_stack_elem();
void lg_rm_edge(edge_t *);
w_edge_t *lg_mk_w_edge();
void lg_rm_w_edge(w_edge_t *);
void lg_rm_stack_elem(stack_elem_t *);
change_t *lg_mk_change();
void lg_rm_change(change_t *);
