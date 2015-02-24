#define SZ_SMALL 0
#define SZ_BIG 1

#define G_ERR_EDGE_EXISTS -1

#include <unistd.h>
#include <stdint.h>

typedef union gelem {
	uint64_t	ge_u;
	int64_t		ge_i;
	char		ge_c[8];
	uintptr_t	ge_p;
} gelem_t;

typedef struct lg_graph lg_graph_t;

typedef int fold_cb_t(gelem_t, gelem_t, gelem_t *);
typedef int term_cb_t(gelem_t);

extern lg_graph_t *lg_create_graph();
extern lg_graph_t *lg_create_digraph();
extern int lg_connect(lg_graph_t *g, gelem_t e1, gelem_t e2);
extern int lg_wconnect(lg_graph_t *g, gelem_t e1, gelem_t e2, double w);
extern gelem_t lg_bfs_fold(lg_graph_t *g, gelem_t start, fold_cb_t, gelem_t z);
extern gelem_t lg_dfs_fold(lg_graph_t *g, gelem_t start, fold_cb_t, gelem_t z);
