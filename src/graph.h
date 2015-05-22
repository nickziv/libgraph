/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * Copyright (c) 2015, Nick Zivkovic
 */

#define SZ_SMALL 0
#define SZ_BIG 1

#define G_ERR_EDGE_EXISTS -1
#define G_ERR_SELF_CONNECT -2
#define G_ERR_SELF_DISCONNECT -3
#define G_ERR_NFOUND_DISCONNECT -4

#include <unistd.h>
#include <stdint.h>

typedef union gelem {
	uint64_t	ge_u;
	int64_t		ge_i;
	char		ge_c[8];
	void		*ge_p;
	double		ge_d;
} gelem_t;

typedef struct lg_graph lg_graph_t;

/* agg-val, node, ptr to agg-val */
typedef int fold_cb_t(gelem_t, gelem_t, gelem_t *);
/* from-node, to-node, agg-val */
typedef void adj_cb_t(gelem_t, gelem_t, gelem_t);
/* the popped node, agg-val*/
typedef int pop_cb_t(gelem_t, gelem_t);
typedef void edges_cb_t(gelem_t, gelem_t, gelem_t);


extern lg_graph_t *lg_create_graph();
extern lg_graph_t *lg_create_wgraph();
extern lg_graph_t *lg_create_digraph();
extern lg_graph_t *lg_create_wdigraph();
extern int lg_connect(lg_graph_t *g, gelem_t e1, gelem_t e2);
extern int lg_disconnect(lg_graph_t *g, gelem_t e1, gelem_t e2);
extern int lg_wconnect(lg_graph_t *g, gelem_t e1, gelem_t e2, gelem_t w);
extern int lg_wdisconnect(lg_graph_t *g, gelem_t e1, gelem_t e2, gelem_t w);
extern gelem_t lg_bfs_fold(lg_graph_t *g, gelem_t start, adj_cb_t, fold_cb_t, gelem_t z);
extern gelem_t lg_bfs_rdnt_fold(lg_graph_t *g, gelem_t start, adj_cb_t, fold_cb_t, gelem_t z);
extern gelem_t lg_dfs_fold(lg_graph_t *g, gelem_t start, pop_cb_t, fold_cb_t, gelem_t z);
extern gelem_t lg_dfs_rdnt_fold(lg_graph_t *g, gelem_t start, pop_cb_t, fold_cb_t, gelem_t z);
extern void lg_edges(lg_graph_t *g, edges_cb_t);
