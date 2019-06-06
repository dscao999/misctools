#include <stdio.h>
#include <gmp.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "node.h"
#include "fprimes.h"

static volatile int sflag = 0;

static void sig_handle(int sig)
{
	if (sig == SIGTERM || sig == SIGINT)
		sflag = 1;
}

struct workinfo {
	volatile int *sflag;
	int plen;
};

static void *node_leader(void *);

int main(int argc, char *argv[])
{
	struct numainfo numainfo;
	struct nodeinfo *ninfo;
	int i, sysret;
	volatile unsigned int p_count;
	struct sigaction act;
	struct priminfo winfo;

	winfo.sflag = &sflag;
	winfo.plen = 512;

	if (argc > 1)
		winfo.plen = atoi(argv[1]);
	printf("The length of prime is set to: %d\n", winfo.plen);

	act.sa_handler = sig_handle;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sysret = sigaction(SIGINT, &act, NULL);
	if (sysret == -1) {
		fprintf(stderr, "sigaction SIGINT failed->%s\n", strerror(errno));
		return 4;
	}
	sysret = sigaction(SIGTERM, &act, NULL);
	if (sysret == -1) {
		fprintf(stderr, "sigaction SIGTERM failed->%s\n", strerror(errno));
		return 4;
	}

	numa_probe(&numainfo);

	p_count = 0;
	ninfo = numainfo.nodeinfo;
	for (i = 0; i < numainfo.numnode; i++) {
		ninfo->pdat = &winfo;
		ninfo->odat = (void *)&p_count;
		ninfo->stack = numa_alloc_onnode(8192, ninfo->node);
		pthread_attr_init(&ninfo->thattr);
		pthread_attr_setstack(&ninfo->thattr, ninfo->stack, 8192);
		sysret = pthread_create(&ninfo->thid, &ninfo->thattr,
				node_leader, ninfo);
		if (sysret) {
			fprintf(stderr, "Cannot create node leader: %s\n",
					strerror(sysret));
			exit(4);
		}
		printf("Leader for node: %d is created.\n", ninfo->node);
		ninfo++;
	}

	printf("Number of primes: %4d", p_count);
	fflush(stdout);
	do {
		sleep(1);
		printf("\b\b\b\b%4d", p_count);
		fflush(stdout);
	} while (sflag == 0);

	ninfo = numainfo.nodeinfo;
	for (i = 0; i < numainfo.numnode; i++) {
		pthread_join(ninfo->thid, NULL);
		ninfo++;
	}
	printf("\nTotal %d number of primes have been found.\n", p_count);

	numa_exit(&numainfo);

	return 0;
}

static void *node_leader(void *dat)
{
	struct nodeinfo *ninfo = dat;
	struct bitmask *me;
	struct priminfo *thargs, *arg;
	int i, sysret;

	me = numa_allocate_nodemask();
	numa_bitmask_setbit(me, ninfo->node);
	numa_bind(me);
	numa_free_nodemask(me);

	thargs = malloc(sizeof(struct priminfo)*ninfo->numcpu);
	arg = thargs;
	for (i = 0; i < ninfo->numcpu; i++, arg++) {
		arg->plen = ((struct priminfo *)ninfo->pdat)->plen;
		arg->sflag = ((struct priminfo *)ninfo->pdat)->sflag;
		arg->odat = ninfo->odat;
		sysret = pthread_create(&arg->thid, NULL, scan_prime, arg);
		if (sysret) {
			fprintf(stderr, "Cannot create worker for CPU: %s\n",
					strerror(sysret));
			exit(4);
		}
	}

	arg = thargs;
	for (i = 0; i < ninfo->numcpu; i++, arg++)
		pthread_join(arg->thid, NULL);
	free(thargs);

	return NULL;
}
