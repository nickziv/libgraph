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
 * weight itself is stored as the 64-bit `double` type. The 64 bits that
 * represent each node are encapsulated in a union called the `gelem_t`. Which
 * is short for graph-element. This means that nodes themselves can be
 * pointers, doubles, signed/unsinged integers, 8-byte char-arrays and so on.
 * The library doesn't really need to know anything about the node. Only which
 * node points to which. Nodes are identified by their bits. So if one is
 * storing pointers to structs in the gelem's, then 2 identical structs with
 * different pointers will be interpreted as being different.
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
	double		wed_weight;
} w_edge_t;

struct lg_graph {
	gtype_t		gr_type;
	slablist_t	*gr_edges;
};

typedef void *bfs_fold_cb_t(void *);
typedef void bfs_map_cb_t(void *);

lg_graph_t *lg_mk_graph();
void lg_rm_graph(lg_graph_t *);
edge_t *lg_mk_edge();
void lg_rm_edge(edge_t *);
w_edge_t *lg_mk_w_edge();
void lg_rm_w_edge(w_edge_t *);
