#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "stm8s.h"
#include "ltkrn.h"
#include "spi.h"

#define SPI_DR SPI->DR

volatile char *spi_tx_bfr;
volatile char *spi_rx_bfr;
volatile int spi_tx_len;
volatile int spi_rx_len;
volatile int spi_tx_cnt;
volatile int spi_rx_cnt;
volatile int spi_flag;
volatile int spi_stat;

krn_thread *spi_master_sleep_thread;
static krn_mutex spi_master_mutex;

void spi_master_init(char prescaler)
{
  krn_mutex_init(&spi_master_mutex);
  SPI->CR1 = SPI_CR1_CPOL | SPI_CR1_CPHA | SPI_CR1_MSTR | SPI_BAUDRATEPRESCALER_16;
  SPI->CR2 = SPI_CR2_SSI | SPI_CR2_SSM;
  SPI->ICR = SPI_ICR_TXEI;
}

void spi_master_start()
{
  CRITICAL_STORE;
  CRITICAL_START();
	spi_flag = 0;
	spi_tx_cnt = 0;
	spi_rx_cnt = 0;
	if(spi_tx_len != 0) spi_flag |= SPI_TX_PEND;
	if(spi_rx_len != 0) spi_flag |= SPI_RX_PEND;
	if( (spi_flag & (SPI_RX_PEND | SPI_TX_PEND)) == 0 ) {
          CRITICAL_END();
          return;
        }
        SPI->CR1 |= SPI_CR1_SPE;
        if(spi_flag & SPI_TX_PEND) SPI_DR = *(spi_tx_bfr++);
	else SPI_DR = 0xff;
        CRITICAL_END();
}

void spi_master_wait()
{
  CRITICAL_STORE;
  CRITICAL_START();
  if(spi_flag & (SPI_RX_PEND | SPI_TX_PEND) )
  {
    spi_master_sleep_thread = krn_thread_current;
    krn_thread_lock(spi_master_sleep_thread);
    krn_dispatch();
  }
  CRITICAL_END();
}

#if defined(__IAR_SYSTEMS_ICC__)
#pragma vector = 12
#endif
INTERRUPT void spi_isr (void)
#if defined(__RCSTM8__)
interrupt 10
#endif
{
	if(spi_flag & SPI_TX_PEND)
	{
       		spi_tx_len--;
      		spi_tx_cnt++;
		if( spi_tx_len == 0 )
		{
			spi_flag &= ~SPI_TX_PEND;
			if(spi_flag & SPI_RX_PEND) SPI_DR = 0xff;
			else { 
                              SPI->CR1 &= ~SPI_CR1_SPE;
			      if(spi_master_sleep_thread) {
				krn_thread_unlock(spi_master_sleep_thread);
				krn_thread_move(spi_master_sleep_thread, krn_thread_current);
				spi_master_sleep_thread = 0;
				krn_dispatch(); //uncomment for extra hardness
			      }
			  spi_flag |= SPI_IDLE;
			}
		}
		else
		{
			SPI_DR = *(spi_tx_bfr++);
			spi_flag |= SPI_TX_EV;;
		}
	}
	else
	{
		*(spi_rx_bfr++) = SPI_DR;
		spi_flag |= SPI_RX_EV;
		spi_rx_len--;
		spi_rx_cnt++;
		if( spi_rx_len == 0 ) 
		{
			spi_flag &= ~SPI_RX_PEND;
			spi_flag |= SPI_IDLE;
                        SPI->CR1 &= ~SPI_CR1_SPE;
			if(spi_master_sleep_thread) {
			  krn_thread_unlock(spi_master_sleep_thread);
			  krn_thread_move(spi_master_sleep_thread, krn_thread_current);
			  spi_master_sleep_thread = 0;
			  krn_dispatch(); //uncomment for extra hardness
			}
		}
                else SPI_DR = 0xff;
	}
}
