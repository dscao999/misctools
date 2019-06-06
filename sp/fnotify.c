#include <sys/inotify.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

static volatile int stop_flag = 0;

static void sighandler(int sig)
{
	if (sig == SIGINT || sig == SIGTERM)
		stop_flag = 1;
}

int main(int argc, char *argv[])
{
	int inot, retv, wnot, numbytes;
	size_t nlen;
	struct inotify_event ievents[32];
	char *linebuf;
	FILE *wf;
	struct sigaction sact;
	int nlines, clines;

	sigaction(SIGINT, NULL, &sact);
	sact.sa_handler = sighandler;
	sigaction(SIGINT, &sact, NULL);
	sigaction(SIGTERM, &sact, NULL);

	linebuf = malloc(256);
	retv = 0;
	wf = fopen("/tmp/test.txt", "rb");
	getline(&linebuf, &nlen, wf);
	nlines = 0;
	while (!feof(wf)) {
		nlines++;
		printf("%s", linebuf);
		getline(&linebuf, &nlen, wf);
	}
	fclose(wf);
	printf("Watching begins...\n");

	inot = inotify_init();
	if (inot == -1) {
		fprintf(stderr, "inotify_init failed: %s\n", strerror(errno));
		retv = 4;
		goto exit_100;
	}
	wnot = inotify_add_watch(inot, "/tmp/test.txt", IN_MODIFY);
	if (wnot == -1) {
		fprintf(stderr, "inotify_add_watch failed: %s\n", strerror(errno));
		retv = 8;
		goto exit_090;
	}

	do {
		numbytes = read(inot, ievents, 32*sizeof(struct inotify_event));
		printf("Something: %d\n", numbytes);
		if (numbytes == 0 || (numbytes == -1 && errno==EINTR))
			continue;
		else if (numbytes == -1) {
			retv = 12;
			fprintf(stderr, "inotify failed: %s\n", strerror(errno));
			goto exit_090;
		}
		if (ievents[0].wd == wnot) {
			wf = fopen("/tmp/test.txt", "rb");
			getline(&linebuf, &nlen, wf);
			clines = 0;
			while (clines < nlines ) {
				clines++;
				getline(&linebuf, &nlen, wf);
			}
			while (!feof(wf)) {
				clines++;
				printf("%s", linebuf);
				getline(&linebuf, &nlen, wf);
			}
			nlines = clines;
		}
	} while (stop_flag == 0);

exit_090:
	close(inot);
	printf("Watching ends...\n");
exit_100:
	free(linebuf);
	return retv;
}
