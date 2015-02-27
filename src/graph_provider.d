provider graph {
	probe got_here(int);
	probe bfs_begin(uintptr_t);
	probe bfs_end(uintptr_t);
	/* gelem */
	probe bfs_enq(uint64_t);
	probe bfs_deq(uint64_t);
	probe bfs_visit(uint64_t);
	probe dfs_begin(uintptr_t);
	probe dfs_end(uintptr_t);
	probe dfs_push(uintptr_t, uint64_t);
	probe dfs_pop(uintptr_t, uint64_t);
	probe dfs_visit(uintptr_t, uint64_t);
};

#pragma D attributes Evolving/Evolving/ISA      provider graph provider
#pragma D attributes Private/Private/Unknown    provider graph module
#pragma D attributes Private/Private/Unknown    provider graph function
#pragma D attributes Private/Private/ISA        provider graph name
#pragma D attributes Evolving/Evolving/ISA      provider graph args
