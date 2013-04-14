#ifndef _SPI_H_
#define _SPI_H_

#define SPI_RX_EV		0x02
#define SPI_TX_EV		0x04
#define SPI_RX_PEND		0x08
#define SPI_TX_PEND		0x10
#define SPI_IDLE		0x20
#define SPI_ERROR		0x80

#include "stm8s_spi.h"

extern volatile char *spi_tx_bfr;
extern volatile char *spi_rx_bfr;
extern volatile int spi_tx_len;
extern volatile int spi_rx_len;
extern volatile int spi_tx_cnt;
extern volatile int spi_rx_cnt;
extern volatile int spi_flag;
extern krn_thread *spi_master_sleep_thread;
extern krn_mutex spi_master_mutex;

#define spi_master_lock() krn_mutex_lock(&spi_master_mutex)
#define spi_master_unlock() krn_mutex_unlock(&spi_master_mutex)

extern void spi_master_init(char prescaler);
extern void spi_master_start();	// rx/tx buffers must be set before
extern void spi_master_wait();

#endif

