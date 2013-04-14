#ifndef _XMODEM_H_
#define _XMODEM_H_

extern char (*xmodem_recv_cb)(uint8_t *bfr, int len);
extern void xmodem_init();
extern char xmodem_recv();

#endif
