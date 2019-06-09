#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define MAX_LINE_LEN	1024

#define unlikely(x) __builtin_expect(x, 0)
#define likely(x) __builtin_expect(x, 1)

struct token_subst {
	const char *token;
	const char *rep;
	int olen, nlen;
};

static struct token_subst tokens[] = {
	{ .token = "LINUX_VERSION_CODE", .rep = "199168", .olen=18, .nlen = 6 },
	{ .token = "RHEL_MAJOR", .rep = "7", .olen = 10, .nlen = 1 },
	{ .token = "RHEL_MINOR", .rep = "5", .olen = 10, .nlen = 1 },
	{ .token = "CONFIG_SUSE_KERNEL", .rep = "0", .olen = 18, .nlen = 1 },
	{ .token = NULL, .rep = NULL }
};

static inline unsigned int kversion(unsigned int a, unsigned int b, unsigned c)
{
	return (a << 16) + (b << 8) + c;
}

static void replace_linux_version(char *procbuf, int llen)
{
	char *hit, *term, *bdef;
	int subed = 0, defstay, par, len;
	struct token_subst *token = tokens;
	unsigned int ka, kb, kc;
	const char *pchr;

	while (token->token) {
		hit = strstr(procbuf, token->token);
		while (hit) {
			subed = 1;
			memcpy(hit, token->rep, token->nlen);
			memset(hit+token->nlen, ' ', token->olen-token->nlen);
			hit = strstr(procbuf, token->token);
		}
		token++;
	}

	hit=strstr(procbuf, "KERNEL_VERSION(");
	while (hit) {
		par = 1;
		pchr = hit + 15;
		while (par) {
			if (*pchr == ')')
				par -= 1;
			else if (*pchr == '(')
				par += 1;
			pchr++;
		}
		sscanf(hit+15, " %d, %d, %d )", &ka, &kb, &kc);
		len = sprintf(hit, "%d", kversion(ka, kb, kc));
		memset(hit+len, ' ', pchr - hit - len);
		subed = 1;
		hit=strstr(pchr, "KERNEL_VERSION(");
	}

	if (subed) {
		hit = strstr(procbuf, "defined(");
		while (hit) {
			term = strchr(hit, ')');
			defstay = 0;
			bdef = hit;
			hit += 8;
			while (hit < term) {
				if (*hit != ' ' && (*hit < '0' || *hit > '9')) {
					defstay = 1;
					break;
				}
				hit++;
			}
			if (!defstay) {
				memset(bdef, ' ', 8);
				*term = ' ';
			}
			hit = strstr(term, "defined(");
		}
	}
}

static int evaluate_if(char *procbuf, int len)
{
	return -1;
}

static inline const char *strip_blanks(const char *buf)
{
	const char *cchr = buf;

	while (*cchr == ' ' || *cchr == '\t')
		cchr++;
	return cchr;
}

static inline int m_getline(char **buf, size_t *len, FILE *fin)
{
	int nochr;

	nochr = getline(buf, len, fin);
	if (nochr == 0 || nochr > 1024) {
		fprintf(stderr, "Invalid file, properly a binary!\n");
		exit(200);
	}
	return nochr;
}

static int process_if(const char *buf, int len, FILE *fin, FILE *fot, int lno)
{
	char *procbuf, *chr, *lbuf;
	int lp;
	int nochr;
	size_t buflen;

	procbuf = malloc(8*MAX_LINE_LEN);
	lbuf = malloc(MAX_LINE_LEN);
	if (unlikely(!procbuf || !lbuf)) {
		fprintf(stderr, "Out of Memory. Err Code: 100\n");
		exit(100);
	}
	buflen = MAX_LINE_LEN;

	strcpy(procbuf, buf);
	fwrite(procbuf, 1, len, fot);
	lp = 1;
	chr = procbuf + len;
	while (*(chr-2) == '\\' && *(chr-1) == '\n') {
		chr -= 2;
		nochr = m_getline(&lbuf, &buflen, fin);
		memcpy(chr, lbuf, nochr);
		len += nochr - 2;
		chr += nochr;
		fwrite(lbuf, 1, nochr, fot);
		lp++;
	}
	*chr = 0;
	if (*(chr-1) == '\n') {
		*(chr-1) = 0;
		len--;
	}

	replace_linux_version(procbuf, len);
	fprintf(stderr, "#if processed at line: %d\n", lno+1);
	fprintf(stderr, "%s\n", procbuf);

	evaluate_if(procbuf, len);

	free(lbuf);
	free(procbuf);
	return lp;
}

int main(int argc, char *argv[])
{
	int retv = 0, lcnt, lp;
	char *lbuf;
	ssize_t len;
	size_t maxlen;
	FILE *fin, *fot;
	const char *cchr;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s file [out file]\n", argv[0]);
		return 4;
	}
	fin = fopen(argv[1], "rb");
	if (argc > 2)
		fot = fopen(argv[2], "wb");
	else
		fot = fopen("/dev/null", "wb");
	if (unlikely(!fin || !fot)) {
		if (fin)
			fclose(fin);
		if (fot)
			fclose(fot);
		return 8;
	}

	lbuf = malloc(MAX_LINE_LEN);
	if (unlikely(!lbuf)) {
		fprintf(stderr, "Out of Memory! Err Code: 10000\n");
		return 100;
	}
	maxlen = MAX_LINE_LEN;

	lcnt = 0;
	len = getline(&lbuf, &maxlen, fin);
	while (len > 0 && len < 1024) {
		cchr = strip_blanks(lbuf);
		if (unlikely(strstr(cchr, "#if") == cchr)) {
			lp = process_if(lbuf, len, fin, fot, lcnt);
			lcnt += lp;
		} else {
			fwrite(lbuf, 1, len, fot);
			lcnt += 1;
		}
		len = getline(&lbuf, &maxlen, fin);
	}
	if (unlikely(!feof(fin))) {
		fprintf(stderr, "Unexpected read error \"%s\": 8\n",
				strerror(errno));
		if (len >= 1024)
			fprintf(stderr, "Properly a binary file!\n");
		retv = 8;
	}
	fprintf(stderr, "Total %d lines.\n", lcnt);

	free(lbuf);
	fclose(fot);
	fclose(fin);
	return retv;
}
