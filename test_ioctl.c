#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "include/ioctl_cmds.h"

int main () {
	int fd = open("/dev/mydev", O_RDWR);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	if (ioctl(fd, RD_SWITCHES, 0) < 0) {
		perror("ioctl");
		return 1;
	}

	unsigned int data;
	if (read(fd, &data, sizeof(data)) < 0) {
		perror("read");
		return 1;
	}

	printf("Read data: 0x%X\n", data);

	close(fd);
	return 0;
}
