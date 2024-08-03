#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(void)
{
	int fd;
	int bytes;
	char buf[2048];
	char rbuf[2048];
	int ret;

	fd = open("/dev/pcd-dev-create-1", O_RDWR);
	if (fd == -1) {
		printf("open failed\n");
		perror("open");

		return errno;
	}

	sprintf(buf, "DEAD");

	bytes = write(fd, buf, 4);

	if (bytes == -1) {
		printf("write failed\n");
		perror("write");
		return errno;
	} else {
		printf("write data size: %d\n", bytes);
	}

	ret = lseek(fd, 0, SEEK_SET);

	if (ret == -1 ){
		printf("lseek failed\n");
		perror("lseek");
		return errno;
	}

	bytes = read(fd, rbuf, 4);
	if (bytes == -1) {
		printf("read failed\n");
		perror("read");
		return errno;
	} else {
		printf("bytes read: %d\n", bytes);
		printf("read buffer: %s\n", rbuf);
	}
	return 0;
}
