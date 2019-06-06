#ifndef THRCTL_DSCAO__
#define THRCTL_DSCAO__
#include <time.h>

enum NSTAGE {UNINIT = 0, INIT, SCAN, FINISH};
static inline unsigned int cmpswap(unsigned int oldv, unsigned int newv, void *sink)
{
	__asm__ __volatile__("cmpxchg %2, (%3)"
				:"=a"(oldv):"0"(oldv), "r"(newv), "r"(sink));
	return oldv;
};
static inline int atomic_inc(unsigned int *count)
{
	__asm__ __volatile__("lock incl (%0)":"=r"(count):"0"(count));
	return *count;
};
static inline unsigned long timediff(const struct timespec *stm, const struct timespec *etm)
{
	unsigned long secs;
	long nsecs;

	secs = etm->tv_sec - stm->tv_sec;
	nsecs = etm->tv_nsec - stm->tv_nsec;
	if (nsecs < 0) {
		nsecs += 1000000000;
		secs--;
	}
	return secs*1000000 + nsecs/1000;
};
#endif /* THRCTL_DSCAO__ */
