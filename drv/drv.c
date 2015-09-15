#include <graph.h>
#include <stdlib.h>
#include <stdio.h>


#define FRANKFURT	0
#define	MANNHEIM	1
#define	KARLSRUHE	2
#define	WURZBERG	3
#define	NURNBERG	4
#define AUGSBERG	5
#define	ERFURT		6
#define	STUTGART	7
#define	KASSEL		8
#define	MUNCHEN		9

/*
 * We make the ID's printable.
 */
char *city[] = {"Frankfurt", "Mannheim", "Karlsruhe", "Wurzberg",
"Nurnberg", "Augsberg", "Erfurt", "Stutgart", "Kassel", "Munchen"};

/*
 * An abstraction over the lg_connect command.
 */
void
connect_city(lg_graph_t *g, uint64_t c1, uint64_t c2)
{
	gelem_t e1;
	gelem_t e2;
	e1.ge_u = c1;
	e2.ge_u = c2;
	int ret = lg_connect(g, e1, e2);
	if (ret != 0) {
		/*
		 * TODO Handle the error here...
		 */
	}
}

void
wconnect_city(lg_graph_t *g, uint64_t c1, uint64_t c2)
{
	gelem_t e1;
	gelem_t e2;
	e1.ge_u = c1;
	e2.ge_u = c2;
	gelem_t def_weight;
	def_weight.ge_u = 0;
	int ret = lg_wconnect(g, e1, e2, def_weight);
	if (ret != 0) {
		/*
		 * TODO Handle the error here...
		 */
	}
}


/*
 * This function creates an undirected, unweighted graph of German cities. 
 */
lg_graph_t *
germany_map()
{
	lg_graph_t *g = lg_create_graph();
	connect_city(g, FRANKFURT, MANNHEIM);
	connect_city(g, FRANKFURT, WURZBERG);
	connect_city(g, FRANKFURT, KASSEL);
	connect_city(g, MANNHEIM, KARLSRUHE);
	connect_city(g, WURZBERG, ERFURT);
	connect_city(g, WURZBERG, NURNBERG);
	connect_city(g, STUTGART, NURNBERG);
	connect_city(g, NURNBERG, MUNCHEN);
	connect_city(g, AUGSBERG, MUNCHEN);
	connect_city(g, AUGSBERG, KARLSRUHE);
	return (g);
}

/*
 * Creates a directed acyclic graph that has the structure of a tree, where
 * some parents share the same child. We use this to test `lg_bfs_rdnt_fold()`.
 */
lg_graph_t *
tree_graph()
{
	lg_graph_t *g = lg_create_digraph();
	connect_city(g, FRANKFURT, MANNHEIM);
	connect_city(g, FRANKFURT, WURZBERG);
	connect_city(g, FRANKFURT, KASSEL);
	connect_city(g, MANNHEIM, KARLSRUHE);
	connect_city(g, KARLSRUHE, AUGSBERG);
	connect_city(g, WURZBERG, NURNBERG);
	connect_city(g, WURZBERG, ERFURT);
	connect_city(g, NURNBERG, STUTGART);
	connect_city(g, KASSEL, MUNCHEN);
	/* this makes STUTGART a shared (therefor redundant) child */
	connect_city(g, AUGSBERG, STUTGART);
	return (g);
}

/*
 * Creates a directed weighted acycling graph that has the structure of a tree.
 */
lg_graph_t *
tree_wgraph()
{
	lg_graph_t *g = lg_create_wdigraph();
	wconnect_city(g, FRANKFURT, MANNHEIM);
	wconnect_city(g, FRANKFURT, WURZBERG);
	wconnect_city(g, FRANKFURT, KASSEL);
	wconnect_city(g, MANNHEIM, KARLSRUHE);
	wconnect_city(g, KARLSRUHE, AUGSBERG);
	wconnect_city(g, WURZBERG, NURNBERG);
	wconnect_city(g, WURZBERG, ERFURT);
	wconnect_city(g, NURNBERG, STUTGART);
	wconnect_city(g, KASSEL, MUNCHEN);
	/* this makes STUTGART a shared (therefor redundant) child */
	wconnect_city(g, AUGSBERG, STUTGART);
	return (g);
}

void
print_parent(gelem_t par, gelem_t child, gelem_t opt)
{
	uint64_t id = par.ge_u;
	uint64_t id2 = child.ge_u;
	char *name1 = city[id];
	char *name2 = city[id2];
	printf("Visiting %s from %s\n", name1, name2);
}

int
print_walk(gelem_t agg, gelem_t last, gelem_t *aggp)
{
	/*
	 * We don't aggregate anything.
	 */
	uint64_t id = last.ge_u;
	char *name = city[id];
	printf("Visited: %s\n", name);
	return (0);
}

int
pop_node(gelem_t node, gelem_t agg)
{
	uint64_t id = node.ge_u;
	char *name = city[id];
	printf("Popped: %s\n", name);
	return (0);
}

int
end()
{
	return (1);
}

int
main()
{
	lg_graph_t *germany = germany_map();
	lg_graph_t *tree = tree_graph();
	lg_graph_t *wtree = tree_wgraph();
	gelem_t zero;
	gelem_t start;
	start.ge_u = FRANKFURT;
	gelem_t start_flip;
	start_flip.ge_u = STUTGART;
	printf("BFS Walk, starting from Frankfurt:\n");
	lg_bfs_fold(germany, start, print_parent, print_walk, zero);
	printf("BFS Walk, no-cb starting from Frankfurt:\n");
	lg_bfs_fold(germany, start, print_parent, NULL, zero);
	printf("DFS Walk, starting from Frankfurt:\n");
	lg_dfs_fold(germany, start, pop_node, print_walk, zero);
	printf("DFS Tree-Walk, starting from Frankfurt:\n");
	lg_dfs_fold(tree, start, pop_node, print_walk, zero);
	printf("BFS RDNT Walk, starting from Frankfurt:\n");
	lg_bfs_rdnt_fold(tree, start, print_parent, print_walk, zero);
	printf("BFS RDNT Walk, no-cb starting from Frankfurt:\n");
	lg_bfs_rdnt_fold(tree, start, print_parent, NULL, zero);
	printf("DFS RDNT Walk, starting from Frankfurt:\n");
	lg_dfs_rdnt_fold(tree, start, pop_node, print_walk, zero);
	printf("DFS RDNT Weighted Walk, starting from Frankfurt:\n");
	lg_dfs_rdnt_fold(wtree, start, pop_node, print_walk, zero);
	printf("Weighted Edge Flip,\n");
	lg_graph_t *wflip = lg_flip_edges(wtree);
	printf("DFS Tree-Walk, starting from Stutgart:\n");
	lg_dfs_fold(wflip, start_flip, pop_node, print_walk, zero);
	printf("Unweighted Edge Flip,\n");
	lg_graph_t *flip = lg_flip_edges(tree);
	printf("DFS Tree-Walk, starting from Stutgart:\n");
	lg_dfs_fold(flip, start_flip, pop_node, print_walk, zero);
	end();
}
