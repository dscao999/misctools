#ifndef NODE_LEADER_DSCAO__
#define NODE_LEADER_DSCAO__
#include <pthread.h>
#include <numa.h>

#define MAX_CPUS	1024

struct numainfo;
struct nodeinfo {
	int node, numcpu;
	pthread_t thid;
	pthread_attr_t thattr;
	void *stack;
	struct bitmask *cpumask;
	struct numainfo *numainfo;
	void *pdat, *odat;
};
struct numainfo {
	int maxnode, numnode;
	struct bitmask *nodemask;
	struct nodeinfo *nodeinfo;
};

void numa_probe(struct numainfo *numainfo);
void numa_exit(struct numainfo *numainfo);

#endif /* NODE_LEADER_DSCAO__ */
