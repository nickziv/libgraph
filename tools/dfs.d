/*
 * This script traces the parts of the DFS, telling us when the stack is pushed
 * to, popped from, which edge the top bookmark points to, and when slablist
 * bookmark modifications are made.
 */
pid$target::slablist_cur:entry,
pid$target::slablist_next:entry,
pid$target::slablist_prev:entry
{
	printf("%p\n", arg1);
}

pid$target::get_pushable:entry,
pid$target::get_pushable:return
{
}

graph$target:::dfs_push,
graph$target:::dfs_pop
{
	printf("%u\n", arg1);
}

graph$target:::dfs_bm
{
	printf("BM: %p [ %u -> %u ]\n", arg1, arg2, arg3);
}
