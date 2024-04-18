#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "nv.h"

void block_erase(int offset)
{
	char cmd[64] = {0};
	sprintf(cmd,"flash_erase -q %s %d 1",NV_DEV_FILE,offset);
	system(cmd);
}


int block_write(char *blkname, off_t offset, void *data, size_t size)
{
	int fd;
	size_t ret;

	fd = open(blkname, O_WRONLY);
	if (fd < 0) {
		printf("open %s error: %s.\n", blkname, strerror(errno));
		goto err_out;
	}

	if (offset != lseek(fd, offset, SEEK_SET)) {
		printf("lseek to %lu bytes error: %s.\n", offset, strerror(errno));
		goto err_out;
	}

	ret = write(fd, data, size);
	if (size != ret) {
		printf("write %d(real %d) bytes error: %s.\n",
			   (int)size, (int)ret, strerror(errno));
		goto err_out;
	}

	close(fd);

	return ret;

err_out:
	if (fd >= 0) {
		close(fd);
	}

	return -1;
}

int block_read(char *blkname, off_t offset, void *data, size_t size)
{
	int fd;
	size_t ret;

	fd = open(blkname, O_RDONLY);
	if (fd < 0) {
		printf("open %s error: %s.\n", blkname, strerror(errno));
		goto err_out;
	}

	if (offset != lseek(fd, offset, SEEK_SET)) {
		printf("lseek to %lu bytes error: %s.\n", offset, strerror(errno));
		goto err_out;
	}

	ret = read(fd, data, size);
	if (size != ret) {
		printf("read %d(real %d) bytes error: %s.\n",
			   (int)size, (int)ret, strerror(errno));
		goto err_out;
	}

	close(fd);

	return 0;

err_out:
	if (fd >= 0) {
		close(fd);
	}

	return -1;
}

