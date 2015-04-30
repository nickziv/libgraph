typedef struct { int dummy; } lg_graph_t;
typedef struct { int dummy; } graphinfo_t;
typedef union gelem { int dummy; } gelem_t;

provider graph {
	probe got_here(int);
	probe bfs_begin(lg_graph_t *g) : (graphinfo_t *g);
	probe bfs_end(lg_graph_t *g) : (graphinfo_t *g);
	probe bfs_enq(gelem_t e) : (gelem_t e);
	probe bfs_deq(gelem_t e) : (gelem_t e);
	probe bfs_visit(gelem_t e) : (gelem_t e);
	probe bfs_rdnt_begin(lg_graph_t *g) : (graphinfo_t *g);
	probe bfs_rdnt_end(lg_graph_t *g) : (graphinfo_t *g);
	probe bfs_rdnt_enq(gelem_t e) : (gelem_t e);
	probe bfs_rdnt_deq(gelem_t e) : (gelem_t e);
	probe dfs_begin(lg_graph_t *g) : (graphinfo_t *g);
	probe dfs_end(lg_graph_t *g) : (graphinfo_t *g);
	probe dfs_push(lg_graph_t *g, gelem_t e) : (graphinfo_t *g, gelem_t e);
	probe dfs_pop(lg_graph_t *g, gelem_t e) : (graphinfo_t *g, gelem_t e);
	probe dfs_bm(lg_graph_t *g, void *bm, gelem_t from, gelem_t to) :
		(graphinfo_t *g, void *bm, gelem_t from, gelem_t to);
	probe dfs_visit(lg_graph_t *g, gelem_t e) : (graphinfo_t *g, gelem_t e);
	probe dfs_rdnt_begin(lg_graph_t *g) : (graphinfo_t *g);
	probe dfs_rdnt_end(lg_graph_t *g) : (graphinfo_t *g);
	probe dfs_rdnt_push(lg_graph_t *g, gelem_t e) : (graphinfo_t *g, gelem_t e);
	probe dfs_rdnt_pop(lg_graph_t *g, gelem_t e) : (graphinfo_t *g, gelem_t e);
	probe dfs_rdnt_bm(lg_graph_t *g, void *bm, gelem_t from, gelem_t to) :
		(graphinfo_t *g, void *bm, gelem_t from, gelem_t to);
};

#pragma D attributes Evolving/Evolving/ISA      provider graph provider
#pragma D attributes Private/Private/Unknown    provider graph module
#pragma D attributes Private/Private/Unknown    provider graph function
#pragma D attributes Private/Private/ISA        provider graph name
#pragma D attributes Evolving/Evolving/ISA      provider graph args
