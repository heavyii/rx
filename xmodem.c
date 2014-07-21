#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "xmodem.h"

#define MAX_ERR  100
#define MAX_RETRY 25

#define XMODEM_SOH 0x01
#define XMODEM_STX 0x02
#define XMODEM_EOT 0x04
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define XMODEM_CAN 0x18
#define XMODEM_CRC_CHR 'C'

#define XMODEM_DATA_SIZE_SOH 128  /* for Xmodem protocol */
#define XMODEM_DATA_SIZE_STX 1024 /* for 1K xmodem protocol */
#define USE_1K_XMODEM 0  /* 1 for use 1k_xmodem 0 for xmodem */

#if (USE_1K_XMODEM)
#define XMODEM_DATA_SIZE  XMODEM_DATA_SIZE_STX
#define XMODEM_HEAD  XMODEM_STX
#else
#define XMODEM_DATA_SIZE  XMODEM_DATA_SIZE_SOH
#define XMODEM_HEAD   XMODEM_SOH
#endif

#define XMODEM_PKT_SIZE (XMODEM_DATA_SIZE + 5)

/*
 * Xmodem Frame form: <SOH><blk #><255-blk #><--128 data bytes--><CRC hi><CRC lo>
 */
typedef struct _pkt {
	uint8_t head;
	uint8_t id;
	uint8_t uid;
	uint8_t data[XMODEM_DATA_SIZE];
	uint8_t crc_hi;
	uint8_t crc_lo;
} xmodem_pkt;

/**
 * return crc16 value
 */
static uint16_t crc16(const uint8_t *buf, int sz) {
	uint16_t crc = 0;
	while (--sz >= 0) {
		int i;
		crc = crc ^ ((uint16_t) *buf++) << 8;
		for (i = 0; i < 8; i++) {
			if (crc & 0x8000) {
				crc = crc << 1 ^ 0x1021;
			} else {
				crc <<= 1;
			}
		}
	}
	return crc;
}

/**
 * check crc16
 * @return: return 0 on match, -1 on not match
 */
static int check_pkt_crc(xmodem_pkt *pkt, uint8_t id) {
	uint16_t crc_pkt;
	if (id != pkt->id || pkt->id + pkt->uid != 0xFF)
		return -1;

	crc_pkt = ((uint16_t) pkt->crc_hi << 8) + pkt->crc_lo;
	if (crc16(pkt->data, sizeof(pkt->data)) == crc_pkt)
		return 0;

	return -1;
}

/**
 * start to receive a file
 * @return: 0 on success, -1 on error
 */
static int recv_start(struct xmodem *rx) {
	uint8_t errcnt = 0;
	uint8_t pktnum = 1;
	xmodem_pkt pkt;
	uint8_t *p = NULL;
	uint8_t * const endp = (uint8_t *) &pkt + XMODEM_PKT_SIZE;
	while (1) {
		/* receive one packet */
		for (p = (uint8_t *) &pkt; p < endp; p++) {
			*p = rx->get_char(rx->args);
			if (p == (uint8_t *) &pkt) {
				switch (*p) {
				case XMODEM_HEAD: {
					/* start new frame */
					break;
				}
				case XMODEM_EOT: {
					rx->put_char(rx->args, XMODEM_ACK);
					/*finished ok*/
					return 0;
				}
				default: {
					errcnt++;
					if (errcnt > MAX_ERR) {
						/* to many error, cancel it */
						rx->put_char(rx->args, XMODEM_CAN);
						return -1;
					}

					/* try again */
					p--;
					rx->put_char(rx->args, XMODEM_NAK);
					break;
				}
				} /* end switch */
			} /* end if p == buf */
		}/* while receive one packet */

		/* check crc */
		if (check_pkt_crc(&pkt, pktnum) == 0) {
			/* handle one packet */
			pktnum++;
			if (rx->write(rx->args, pkt.data, sizeof(pkt.data))
					!= sizeof(pkt.data)) {
				/* write error, stop */
				rx->put_char(rx->args, XMODEM_CAN);
				return -1;
			}
			rx->put_char(rx->args, XMODEM_ACK);
		} else {
			rx->put_char(rx->args, XMODEM_NAK); /* error packet */
		}
	}/* while 1 */
	return -1;
}

/**
 * start to receive a file
 * @return: 0 on success, -1 on error
 */
int xmodem_recv(struct xmodem *rx) {
	uint8_t retry_count = 0;
	while (retry_count < MAX_RETRY) {
		rx->put_char(rx->args, XMODEM_CRC_CHR);
		if (rx->char_avail(rx->args)) {
			return recv_start(rx);
		}
		rx->delay_1s();
	}

	return -1;
}
