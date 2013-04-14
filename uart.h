#ifndef __uart_h__
#define __uart_h__

#define UART_RX_EV	0x01
#define UART_TX_IDLE	0x04
#define UART_RX_RING_OF	0x80
#define UART_RX_WAIT	0x40
#define UART_RX_SIZE	0x80
#define UART_TX_SIZE	0x10

extern volatile char uart_flags;
extern volatile int uart_rx_head;
extern volatile int uart_rx_tail;
extern volatile int uart_tx_head;
extern volatile int uart_tx_tail;

#define uart_rx_ev_get() (uart_flags & UART_RX_EV)
#define uart_rx_ev_clr() uart_flags &= ~UART_RX_EV
#define uart_rx_of_get() (uart_flags & UART_RX_RING_OF)
#define uart_rx_of_clr() uart_flags &= ~UART_RX_RING_OF
#define uart_rx_is_empty() (uart_flags & UART_TX_IDLE)
#define uart_rx_is_timeout() (uart_flags & UART_RX_WAIT)
#define uart_start_tx() UART1->CR2|=UART1_CR2_TIEN
#define uart_tx_len() ((uart_tx_head >= uart_tx_tail) ? UART_TX_SIZE + uart_tx_tail - uart_tx_head - 1 :  uart_tx_tail - uart_tx_head - 1)
extern int uart_init(uint32_t baudrate);
extern void uart_write(char *bfr, int len);
extern void uart_rx_flush();
extern int uart_rx_get_len();
extern int uart_last(char *p, int len);
extern int uart_read_bfr(char *p, int len, char peek);
extern void uart_rx_wait();
#define uart_peek(p,len) uart_read_bfr(p, len, 1)
extern int uart_read(char *p, int len);


#endif
