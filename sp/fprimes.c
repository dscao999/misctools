#include <stdio.h>
#include <gmp.h>
#include <stdlib.h>
#include "fprimes.h"
#include "miscutil.h"

static char hexdigit(unsigned char c)
{
	unsigned char rv, tv;

	rv = c & 0x0f;
	if (rv > 9)
		tv = 'A' + (rv - 10);
	else
		tv = '0' + rv;
	return tv;
}

void *scan_prime(void *arg)
{
	struct priminfo *cm;
	char *rndstr;
	unsigned char *rndnum;
	long int *rd;
	union {
		struct timespec tm;
		struct {
			time_t tv_sec;
			char pad[2];
			unsigned short xsubi[3];
		};
	} rseed;
	mpz_t nstart, nprime;
	int i, j;
	unsigned int *count;

	cm = (struct priminfo *)arg;

	count = (unsigned int *)cm->odat;
	rndnum = malloc(cm->plen);
	rndstr = malloc(2*cm->plen+1);
	if (!rndstr || !rndnum) {
		fprintf(stderr, "Out of Memory!\n");
		return NULL;
	}

	clock_gettime(CLOCK_MONOTONIC_RAW, &rseed.tm);
	for (rd = (long int *)rndnum, i = 0; i < cm->plen/8; i++, rd++)
		*rd = nrand48(rseed.xsubi);

	rndnum[0] |= 0x80;
	for (i = 0, j = 0; i < cm->plen; i++, j+=2) {
		rndstr[j] = hexdigit(rndnum[i] >> 4);
		rndstr[j+1] = hexdigit(rndnum[i] & 0x0f);
	}
	rndstr[j] = '\0';

	mpz_init(nprime);
	mpz_init(nstart);
	mpz_set_str(nstart, rndstr, 16);
	do {
		mpz_nextprime(nprime, nstart);
		atomic_inc(count);
		mpz_add_ui(nstart, nprime, 2);
	} while (*cm->sflag == 0);

	free(rndnum);
	free(rndstr);
	return NULL;
}
