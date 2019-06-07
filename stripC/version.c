#include <stdio.h>
#include <stdlib.h>
/*#define LINUX_VERSION_CODE 199168
 *#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
 *#define RHEL_MAJOR 7
 *#define RHEL_MINOR 5
 *#define RHEL_RELEASE_VERSION(a,b) (((a) << 8) + (b))
 *#define RHEL_RELEASE_CODE 1797
 *#define RHEL_RELEASE "862.14.4"
 */
static inline unsigned int kver(unsigned int a, unsigned int b, unsigned c)
{
	return (a << 16) + (b << 8) + c;
}

int main(int argc, char *argv[])
{
	unsigned a = 0, b = 0, c = 0;

	if (argc > 1)
		a = atoi(argv[1]);
	if (argc > 2)
		b = atoi(argv[2]);
	if (argc > 3)
		c = atoi(argv[3]);
	printf("%u\n", kver(a, b, c));
	return 0;
}
