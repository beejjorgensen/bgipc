/*
** mmapdemo.c -- demonstrates memory mapped files lamely.
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	int fd;
	off_t offset;
	char *data;
	struct stat sbuf;

	if (argc != 2) {
		fprintf(stderr, "usage: mmapdemo offset\n");
		exit(1);
	}

	if ((fd = open("mmapdemo.c", O_RDONLY)) == -1) {
		perror("open");
		exit(1);
	}

	if (stat("mmapdemo.c", &sbuf) == -1) {
		perror("stat");
		exit(1);
	}

	offset = atoi(argv[1]);
	if (offset < 0 || offset > sbuf.st_size-1) {
        // On MacOS the size is a long long, so we cast to make it more
        // portable
		fprintf(stderr, "mmapdemo: offset must be in the range 0-%lld\n", (long long)sbuf.st_size-1);
		exit(1);
	}
	
	if ((data = mmap((void*)0, sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	printf("byte at offset %lld is '%c'\n", (long long)offset, data[offset]);

	return 0;
}
