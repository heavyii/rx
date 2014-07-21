/**
 * tty2tty.c
 *
 * make a pair of virtual look back tty device
 */
#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

int ptym_open(char *pts_name, char *pts_name_s, int pts_namesz) {
	char *ptr;
	int fdm;

	strncpy(pts_name, "/dev/ptmx", pts_namesz);
	pts_name[pts_namesz - 1] = '\0';

	fdm = posix_openpt(O_RDWR | O_NONBLOCK);
	if (fdm < 0)
		return (-1);
	if (grantpt(fdm) < 0) {
		close(fdm);
		return (-2);
	}
	if (unlockpt(fdm) < 0) {
		close(fdm);
		return (-3);
	}
	if ((ptr = ptsname(fdm)) == NULL) {
		close(fdm);
		return (-4);
	}

	strncpy(pts_name_s, ptr, pts_namesz);
	pts_name[pts_namesz - 1] = '\0';

	return (fdm);
}

int main(void) {
	char master1[1024];
	char slave1[1024];
	char master2[1024];
	char slave2[1024];

	int fd1;
	int fd2;

	char c1, c2;

	fd1 = ptym_open(master1, slave1, 1024);

	fd2 = ptym_open(master2, slave2, 1024);

	printf("(%s) <=> (%s)\n", slave1, slave2);

	while (1) {
		int ret;
		fd_set input;

		FD_ZERO(&input);
		FD_SET(fd1, &input);
		FD_SET(fd2, &input);
		ret = select(fd2 + 1, &input, NULL, NULL, NULL);
		if (ret < 0) {
			perror("select");
			break;
		} else if (ret == 0) {
			continue;
		} else {
			if (FD_ISSET(fd1, &input)) {
				if (read(fd1, &c1, 1) == 1 && write(fd2, &c1, 1) == 1)
					continue;
			} else if (FD_ISSET(fd2, &input)) {
				if (read(fd2, &c2, 1) == 1 && write(fd1, &c2, 1) == 1)
					continue;
			}
		}
	};

	close(fd1);
	close(fd2);

	return EXIT_SUCCESS;
}
