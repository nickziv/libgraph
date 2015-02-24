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

typedef enum gtype {
	DIGRAPH, /* directed graph */
	GRAPH, /* undirected graph */
	DIGRAPH_WE, /* directed graph w/ weighted edges */
	GRAPH_WE /* undirected graph w/ weighted edges */
} gtype_t;

struct lg_graph {
	gtype_t		gr_type;
	slablist_t	*gr_edges;
};

typedef struct edge {
	gelem_t		ed_from;
	gelem_t		ed_to;
} edge_t;

typedef struct w_edge {
	gelem_t		wed_from;
	gelem_t		wed_to;
	double		wed_weight;
} w_edge_t;

typedef void *bfs_fold_cb_t(void *);
typedef void bfs_map_cb_t(void *);

lg_graph_t *lg_mk_graph();
void lg_rm_graph(lg_graph_t *);
edge_t *lg_mk_edge();
void lg_rm_edge(edge_t *);
w_edge_t *lg_mk_w_edge();
void lg_rm_w_edge(w_edge_t *);
