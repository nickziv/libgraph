/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at src/LIBSLABLIST.LICENSE
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at src/LIBSLABLIST.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2014 Nicholas Zivkovic. All rights reserved.
 * Use is subject to license terms.
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
