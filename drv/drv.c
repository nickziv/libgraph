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
	printf("ID: %u\n", id);
	char *name = city[id];
	printf("Visited: %s\n", name);
	return (0);
}


int
main()
{
	lg_graph_t *germany = germany_map();
	gelem_t zero;
	gelem_t start;
	start.ge_u = FRANKFURT;
	printf("BFS Walk, starting from Frankfurt:\n");
	lg_bfs_fold(germany, start, print_parent, print_walk, zero);
	printf("DFS Walk, starting from Frankfurt:\n");
	lg_dfs_fold(germany, start, print_walk, zero);
}
