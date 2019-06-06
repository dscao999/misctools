#ifndef FPRIMES_DSCAO__
#define FPRIMES_DSCAO__
#include <pthread.h>

struct priminfo {
	pthread_t thid;
	int plen;
	volatile int *sflag;
	void *odat;
};
void *scan_prime(void *arg);
#endif /* FPRIMES_DSCAO__ */
