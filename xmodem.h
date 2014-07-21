#ifndef XMODEM_H
#define XMODEM_H

struct xmodem {
	void *args; /* arguments pointer pass to operation function */
	int (*get_char)(void *args); /* return one char */
	int (*put_char)(void *args, char c);
	int (*char_avail)(void *args); /* return none zero when available */
	void (*delay_1s)(void); /* delay 1s */
	int (*write)(void *args, void *buf, int len); /* return numbers of bytes written */
};

/**
 * start to receive a file
 * @return: 0 on success, -1 on error
 */
int xmodem_recv(struct xmodem *rx);

#endif
