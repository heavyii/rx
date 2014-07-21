#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include "xmodem.h"

struct xmodem_args {
	int fd;
	int filefd;
};

/**
 *	serial_set - Config serial port parameter
 *	fd			serial device file descriptor
 *	baud_rate		serial communication baud rate
 *	data_bit			data bit count, default is 8
 *	stop_bit     stop bit count, default is 1
 *	parity				odd or even parity, default is no parity
 *	flow_ctrl		hardware or software flow control, default is no fc
 * @return:	0 on success, -1 on error.
 */
int serial_set(int fd, int baud_rate, char data_bit, char stop_bit, char parity,
		char flow_ctrl) {
	struct termios termios_new;
	int baud;
	int ret = -1;

	memset(&termios_new, 0, sizeof(termios_new));
	cfmakeraw(&termios_new); /* set termios raw data */

	switch (baud_rate) {
	case 2400:
		baud = B2400;
		break;
	case 4800:
		baud = B4800;
		break;
	case 38400:
		baud = B38400;
		break;
	case 57600:
		baud = B57600;
		break;
	case 115200:
		baud = B115200;
		break;
	default:
		baud = B9600;
		break;
	}/* end switch */
	cfsetispeed(&termios_new, baud);
	cfsetospeed(&termios_new, baud);

	termios_new.c_cflag |= (CLOCAL | CREAD);

	/* flow control */
	switch (flow_ctrl) {
	case 'H':
	case 'h':
		termios_new.c_cflag |= CRTSCTS; /* hardware flow control */
		break;
	case 'S':
	case 's':
		termios_new.c_cflag |= (IXON | IXOFF | IXANY); /* software flow control */
		break;
	default:
		termios_new.c_cflag &= ~CRTSCTS; /* default no flow control */
		break;
	}/* end switch */

	/* data bit */
	termios_new.c_cflag &= ~CSIZE;
	switch (data_bit) {
	case 5:
		termios_new.c_cflag |= CS5;
		break;
	case 6:
		termios_new.c_cflag |= CS6;
		break;
	case 7:
		termios_new.c_cflag |= CS7;
		break;
	default:
		termios_new.c_cflag |= CS8;
		break;
	}/* end switch */

	/* parity check */
	switch (parity) {
	case 'O':
	case 'o':
		termios_new.c_cflag |= PARENB; /* odd check */
		termios_new.c_cflag &= ~PARODD;
		break;
	case 'E':
	case 'e':
		termios_new.c_cflag |= PARENB; /* even check */
		termios_new.c_cflag |= PARODD;
		break;
	default:
		termios_new.c_cflag &= ~PARENB; /* default no check */
		break;
	}/* end switch */

	/* stop bit */
	if (stop_bit == 2)
		termios_new.c_cflag |= CSTOPB; /* 2 stop bit */
	else
		termios_new.c_cflag &= ~CSTOPB; /* 1 stop bit */

	/* other attribute */
	termios_new.c_oflag &= ~OPOST;
	termios_new.c_iflag &= ~ICANON;
	termios_new.c_cc[VMIN] = 1; /* read char min quantity */
	termios_new.c_cc[VTIME] = 1; /* wait time unit (1/10) second */

	/* clear data in receive buffer */
	tcflush(fd, TCIFLUSH);

	/* set config data */
	ret = tcsetattr(fd, TCSANOW, &termios_new);

	return ret;
}

int put_char(void *args, char c) {
	struct xmodem_args *p = (struct xmodem_args *)args;
	if (write(p->fd, &c, 1) != 1)
		return -1;
	return 0;
}

int char_avail(void *args) {
	int ret;
	fd_set input;
	struct timeval to;
	struct xmodem_args *p = (struct xmodem_args *)args;

	FD_ZERO(&input);
	FD_SET(p->fd, &input);
	to.tv_sec = 0;
	to.tv_usec = 5000;
	ret = select(p->fd + 1, &input, NULL, NULL, &to);
	if (ret < 0) {
		perror("select");
	} else if (ret > 0) {
		if (FD_ISSET(p->fd, &input)) {
			return 1;
		}
	}

	return 0;
}

int get_char(void *args) {
	char ch;
	struct xmodem_args *p = (struct xmodem_args *)args;
	while (!char_avail(args)) {
		/* nothing */
	}
	if (read(p->fd, &ch, 1) == 1)
		return (int) ch;

	perror(__FUNCTION__);
	return 0;
}

void delay_1s() {
	sleep(1);
}

int writer(void *args, void *buf, int size) {
	struct xmodem_args *p = (struct xmodem_args *)args;
	return write(p->filefd, buf, size);
}

int main(int argc, char *argv[]) {
	struct xmodem_args args;
	struct xmodem recver;
	char *filename = argv[2];
	if (argc != 3) {
		printf("receive file in xmodem protocol\n");
		printf("usage: %s <tty dev> <filename>\n", argv[0]);
		return -1;
	}

	/* open tty */
	args.fd = open(argv[1], (O_RDWR | O_NOCTTY | O_NONBLOCK));
	if (args.fd < 0) {
		perror("open");
		return -1;
	}
	serial_set(args.fd, 115200, 8, 1, 0, 0);

	/* open file to write recv file */
	args.filefd = open(filename, O_CREAT | O_WRONLY | O_TRUNC,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (args.filefd < 0) {
		perror("open");
		goto main_quit;
	}

	printf("filefd = %d\n", args.filefd);
	recver.args = &args;
	recver.get_char = get_char;
	recver.put_char = put_char;
	recver.char_avail = char_avail;
	recver.delay_1s = delay_1s;
	recver.write = writer;
	xmodem_recv(&recver);

	main_quit:
	if (args.filefd >= 0) {
		close(args.filefd);
		args.filefd = -1;
	}
	if (args.fd >= 0) {
		close(args.fd);
		args.fd = -1;
	}
	return 0;
}
