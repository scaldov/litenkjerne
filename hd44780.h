#ifndef __hd44780_h__
#define __hd44780_h__

#define HD44780_RS    0x02
#define HD44780_E     0x08
#define HD44780_RW    0x04
#define HD44780_DATA  0xF0
#define HD44780_SH    4
#define HD44780_PORT  GPIOC

#define HD44780_TX_SIZE	0x10
#define HD44780_TX	0x01

extern volatile char hd44780_flags;

#define hd44780_tx_len() ((hd44780_tx_head >= hd44780_tx_tail) ? HD44780_TX_SIZE + hd44780_tx_tail - hd44780_tx_head - 1 :  hd44780_tx_tail - hd44780_tx_head - 1)

extern void hd44780_tx_proc (void);
extern void hd44780_write(char *bfr, int len);
extern int hd44780_init();

#endif
