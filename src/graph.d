typedef struct lg_graph lg_graph_t;

typedef enum gtype {
        DIGRAPH, /* directed graph */
        GRAPH, /* undirected graph */
        DIGRAPH_WE, /* directed graph w/ weighted edges */
        GRAPH_WE /* undirected graph w/ weighted edges */
} gtype_t;

typedef union gelem {
	uint64_t	ge_u;
	int64_t		ge_i;
	char		ge_c[8];
	void		*ge_p;
	double		ge_d;
} gelem_t;

struct lg_graph {
	gtype_t		gr_type;
	void		*gr_edges;
};

typedef struct graphinfo {
	gtype_t		gri_type;
	void		*gri_edges;
} graphinfo_t;

#pragma D binding "1.6.1" translator
translator graphinfo_t < lg_graph_t *g >
{
	gri_type = *(gtype_t *)copyin((uintptr_t)&g->gr_type,
		sizeof (g->gr_type));
	gri_edges = *(void *)copyin((uintptr_t)&g->gr_edges,
		sizeof (g->gr_edges));
}
