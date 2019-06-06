#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define MAX_LINE_LEN	1024

#define unlikely(x) __builtin_expect(x, 0)
#define likely(x) __builtin_expect(x, 1)

static int evaluate_if(char *procbuf, int llen, int *lp, int lno)
{
	char *chr;

	if (strstr(procbuf, "LINUX_VERSION_CODE") != NULL ||
			strstr(procbuf, "RHEL_MAJOR") != NULL ||
			strstr(procbuf, "CONFIG_SUSE_KERNEL") != NULL) {
		fprintf(stderr, "#if processed at line: %d\n", lno+1);
		if (*lp > 1)
			fprintf(stderr, "	%d lines here.\n", *lp);
	}
	printf("%s", procbuf);
	chr = procbuf;
	while (*chr != 0) {
		if (*chr == '\n') {
			if (*(chr-1) == '\\')
				*(chr-1) = ' ';
			*chr = ' ';
		}
		if (unlikely(*chr == EOF)) {
			if (*(chr-1) == '\\')
				*(chr-1) = ' ';
			*chr = ' ';
		}
		chr++;
	}
	return *lp;
}

static int check_version(const char *lbuf, FILE *fin, FILE *fout, int lno)
{
	const char *cchr;
	char *procbuf, *chr;
	int llen, lp;

	cchr = lbuf;
	while (*cchr == ' ' || *cchr == '\t')
		cchr++;

	if (likely(strstr(cchr, "#if") != cchr))
		return 0;

	procbuf = malloc(4*MAX_LINE_LEN);
	if (!procbuf) {
		fprintf(stderr, "Out of Memory. Err Code: 100\n");
		exit(100);
	}

	strcpy(procbuf, lbuf);
	lp = 1;
	llen = strlen(procbuf);
	chr = procbuf + llen;
	while (*(chr-2) == '\\' && *(chr-1) == '\n') {
		*chr = fgetc(fin);
		while (*chr != '\n' && *chr != EOF) {
			chr += 1;
			llen += 1;
			*chr = fgetc(fin);
		}
		chr += 1;
		llen += 1;
		lp++;
	}
	*chr = 0;
	evaluate_if(procbuf, llen, &lp, lno);
	free(procbuf);

	return lp;
}

int main(int argc, char *argv[])
{
	int retv = 0, lcnt, lp;
	char *lbuf;
	ssize_t len;
	size_t maxlen;

	lbuf = malloc(MAX_LINE_LEN);
	if (unlikely(!lbuf)) {
		fprintf(stderr, "Out of Memory! Err Code: 10000\n");
		return 100;
	}
	maxlen = MAX_LINE_LEN;

	lcnt = 0;
	len = getline(&lbuf, &maxlen, stdin);
	while (len > 0 && len < 1024) {
		lp = check_version(lbuf, stdin, stdout, lcnt);
		if (lp == 0) {
			printf("%s", lbuf);
			lcnt += 1;
		} else
			lcnt += lp;
		len = getline(&lbuf, &maxlen, stdin);
	}
	if (unlikely(!feof(stdin))) {
		fprintf(stderr, "Unexpected read error \"%s\": 8\n",
				strerror(errno));
		if (len >= 1024)
			fprintf(stderr, "Properly a binary file!\n");
		retv = 8;
	}
	fprintf(stderr, "Total %d lines.\n", lcnt);

	free(lbuf);
	return retv;
}
