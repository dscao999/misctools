#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "node.h"

void numa_probe(struct numainfo *numainfo)
{
	int i, j, numcpu;
	struct nodeinfo *ninfo;

	if (numa_available() == -1) {
		fprintf(stderr, "NUMA Control not available!\n");
		exit(1);
	}

	numainfo->maxnode = numa_max_node() + 1;
	numainfo->nodemask = numa_all_nodes_ptr;
	numainfo->numnode = numa_bitmask_weight(numainfo->nodemask);
	printf("Total number of nodes: %d\n", numainfo->numnode);
	numainfo->nodeinfo = malloc(sizeof(struct nodeinfo)*numainfo->numnode);
	ninfo = numainfo->nodeinfo;
	for (i = 0; i < numainfo->maxnode; i++) {
		if (!numa_bitmask_isbitset(numainfo->nodemask, i))
			continue;
		ninfo->node = i;
		printf("Node %d CPU: ", i);
		ninfo->numainfo = numainfo;
		ninfo->cpumask = numa_allocate_cpumask();
		numa_node_to_cpus(i, ninfo->cpumask);
		ninfo->numcpu = numa_bitmask_weight(ninfo->cpumask);
		numcpu = 0;
		for (j = 0; numcpu < ninfo->numcpu && j < MAX_CPUS; j++) {
			if (!numa_bitmask_isbitset(ninfo->cpumask, j))
				continue;
			printf("%3d ", j);
			numcpu++;
		}
		printf("\n");
		ninfo++;
	}
}

void numa_exit(struct numainfo *numainfo)
{
	int i;
	struct nodeinfo *ninfo;

	ninfo = numainfo->nodeinfo;
	for (i = 0; i < numainfo->numnode; i++) {
		numa_free_cpumask(ninfo->cpumask);
		ninfo++;
	}
	free(numainfo->nodeinfo);
}
