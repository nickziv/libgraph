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

typedef enum snap_strat {
	SNAP_FAST,
	SNAP_DEDUP
} snap_strat_t;

/*
 * If lg_drop gets told to drop a node `n`, these flags will tell it if it
 * should drop the parent in addition to the kids.
 */
typedef enum drop_strat {
	DROP_PARENT,
	DROP_KIDS
} drop_strat_t;

typedef union gelem {
	uint64_t	ge_u;
	int64_t		ge_i;
	char		ge_c[8];
	void		*ge_p;
	double		ge_d;
} gelem_t;

typedef struct lg_graph lg_graph_t;

/* node */
typedef int br_cb_t(gelem_t);
/* agg-val, node, ptr to agg-val */
typedef int fold_cb_t(gelem_t, gelem_t, gelem_t *);
typedef int flatten_cb_t(gelem_t, gelem_t, gelem_t);
typedef int drop_cb_t(gelem_t);
/* to-node, from-node, weight, agg-val */
typedef void adj_cb_t(gelem_t, gelem_t, gelem_t, gelem_t);
/* the popped node, agg-val*/
typedef int pop_cb_t(gelem_t, gelem_t);
typedef void edges_cb_t(gelem_t, gelem_t, gelem_t);
typedef void edges_arg_cb_t(gelem_t, gelem_t, gelem_t, gelem_t);
/* source node, destination node, weight */
typedef enum snap_cb_ctx {
	EDGE,
	SNAP
} snap_cb_ctx_t;
typedef void snap_cb_t(uint8_t, snap_cb_ctx_t, gelem_t, gelem_t, gelem_t);

extern int lg_is_graph(lg_graph_t *);
extern int lg_is_digraph(lg_graph_t *);
extern int lg_is_wgraph(lg_graph_t *);
extern int lg_is_wdigraph(lg_graph_t *);
extern lg_graph_t *lg_create_graph();
extern lg_graph_t *lg_create_wgraph();
extern lg_graph_t *lg_create_digraph();
extern lg_graph_t *lg_create_wdigraph();
extern void lg_destroy_graph(lg_graph_t *);
extern int lg_connect(lg_graph_t *g, gelem_t e1, gelem_t e2);
extern int lg_disconnect(lg_graph_t *g, gelem_t e1, gelem_t e2);
extern int lg_wconnect(lg_graph_t *g, gelem_t e1, gelem_t e2, gelem_t w);
extern int lg_wdisconnect(lg_graph_t *g, gelem_t e1, gelem_t e2, gelem_t w);
extern gelem_t lg_bfs_fold(lg_graph_t *g, gelem_t start, adj_cb_t, fold_cb_t, gelem_t z);
extern gelem_t lg_bfs_rdnt_fold(lg_graph_t *g, gelem_t start, adj_cb_t, fold_cb_t, gelem_t z);
extern gelem_t lg_dfs_fold(lg_graph_t *g, gelem_t start, pop_cb_t, fold_cb_t, gelem_t z);
extern gelem_t lg_dfs_rdnt_fold(lg_graph_t *g, gelem_t start, pop_cb_t, fold_cb_t, gelem_t z);
extern gelem_t lg_dfs_br_rdnt_fold(lg_graph_t *g, gelem_t start, br_cb_t, pop_cb_t,
		fold_cb_t, gelem_t z);
extern void lg_edges(lg_graph_t *g, edges_cb_t);
extern void lg_edges_arg(lg_graph_t *g, edges_arg_cb_t, gelem_t);
extern uint64_t lg_nedges(lg_graph_t *g);
extern lg_graph_t *lg_flip_edges(lg_graph_t *g);
extern void lg_neighbors(lg_graph_t *g, gelem_t n, edges_cb_t);
extern void lg_neighbors_arg(lg_graph_t *g, gelem_t n, edges_arg_cb_t, gelem_t);
extern void lg_snapshot_cb(lg_graph_t *g, snap_cb_t cb);
extern uint64_t lg_snapshot(lg_graph_t *g);
extern lg_graph_t *lg_clone(lg_graph_t *g, uint64_t snap);
extern int lg_rollback(lg_graph_t *g, uint64_t snap);
extern int lg_destroy_snapshot(lg_graph_t *g, uint64_t snap);
extern int lg_destroy_all_snapshots(lg_graph_t *g);
extern void lg_snapstrat(lg_graph_t *g, snap_strat_t s);
extern void lg_flatten(lg_graph_t *g, gelem_t node, flatten_cb_t *cb, gelem_t arg);
extern void lg_drop(lg_graph_t *g, drop_cb_t *cb, drop_strat_t s);
