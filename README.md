What Is `libgraph`?
-------------------

libgraph is a C library for handling graphs (in the graph-theory sense of the
word). It is built on top of the libslablist library, because a) in certain
situations this can result in memory savings, and b) I want to see how good
slab lists are for implementing a graph. This is very experimental (read:
untested) stuff. Use at your own risk. The interfaces are _evolving_ and are
subject to changes that break backward compatibility. You have been warned.

How Does `libgraph` Implement a Graph, and Why?
-----------------------------------------------

There are 2 ways to implement a graph using a sorted-set structure (like an AVL
Tree, Slab List, etc).

	1: Store the graph as a sorted set of edges.
	2: Embed the edges into the nodes themselves.

The first variant is convenient in the sense that it lets you separate the data
(the nodes) from the actual graph. It also seems to be more space-efficient
than the second variant for larger graphs. The second variant allows one to
implement algorithms like BFS and DFS more easily from scratch. As far as
speed, I haven't done the benchmarks yet. I don't know which method is faster
at what, yet.

And then there are matrix representations, which are great if you have an
upper-bound on the size of your graph, and anticipate that it won't grow. It's
also great if you plan to do matrix transformations on the graph.

I ultimately chose variant 1, because I like memory efficiency, and only have
to code generic BFS and DFS functions once. I will soon find out if this was a
wise choice.

What Are Some Things That Variant 1 Can Do Better?
--------------------------------------------------

One unique feature of separating the nodes from the graph, is the ability to
handle multiple graphs that are overlapping. For example, if you have two
graphs A and B, the library can handle nodes that belong to _both_ A and B.
This situation is not contrived. In the turn-based strategy game _Diplomacy_,
one can consider the map as two graphs: a graph of fleet nodes, and a graph of
army nodes. Coastal nodes are nodes that belong to both graphs: they can be
acted-upon by both fleets and armies. Similarly, a population can be modeled as
N graphs, where each graph represents the connections between people that are
vulnerable to specific viruses or contagions. People that are vulnerable to
more than one virus, are present in more than one graph.

Do You Have Any Examples On How To Use `libgraph`?
--------------------------------------------------

The libgraph/drv directory contains a 'driving' program that shows how to
create a graph and do BFS and DFS on it.
