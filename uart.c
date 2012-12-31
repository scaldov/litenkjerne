#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "stm8s.h"
#include "stm8s_uart1.h"

#include "uart.h"
#include "ltkrn.h"

volatile char uart_flags;
volatile int uart_rx_head;
volatile int uart_rx_tail;
volatile int uart_tx_head;
volatile int uart_tx_tail;

char uart_rx_bfr[UART_RX_SIZE];
char uart_tx_bfr[UART_TX_SIZE];
krn_thread *uart_sleep_thread_tx;
krn_thread *uart_sleep_thread_rx;

static krn_mutex uart_mutex_tx;
static krn_mutex uart_mutex_rx;

#if defined(__IAR_SYSTEMS_ICC__)
#pragma vector = 20
#endif
INTERRUPT void uart_rx_isr (void)
#if defined(__RCSTM8__)
interrupt 18
#endif
{
  uart_rx_bfr[uart_rx_head++] = UART1->DR;
  if(uart_rx_head == UART_RX_SIZE) uart_rx_head = 0;
  if(uart_rx_head == uart_rx_tail) uart_flags |= UART_RX_RING_OF;
  uart_flags |= UART_RX_EV;
  if( uart_flags & UART_RX_RING_OF) uart_rx_tail = uart_rx_head;
  if(uart_sleep_thread_rx) {
    krn_thread_unlock(uart_sleep_thread_rx);
    krn_thread_move(uart_sleep_thread_rx, krn_thread_current);
    uart_sleep_thread_rx = 0;
    //krn_dispatch(); //uncomment for extra hardness
  }
}

#if defined(__IAR_SYSTEMS_ICC__)
#pragma vector = 19
#endif
INTERRUPT void uart_tx_isr (void)
#if defined(__RCSTM8__)
interrupt 17
#endif
{
  if(uart_tx_tail != uart_tx_head)
  {
    UART1->DR = uart_tx_bfr[uart_tx_tail];
    uart_tx_tail ++;
    if(uart_tx_tail >= UART_TX_SIZE) uart_tx_tail = 0;
  }
  else {
    UART1->CR2 &= ~UART1_CR2_TIEN; //disable tx reg empty interrupt
    if(uart_sleep_thread_tx) {
      krn_thread_unlock(uart_sleep_thread_tx);
      krn_thread_move(uart_sleep_thread_tx, krn_thread_current);
      uart_sleep_thread_tx = 0;
      //krn_dispatch(); //uncomment for extra hardness
    }
  }
}

void uart_rx_flush()
{
  CRITICAL_STORE;
  CRITICAL_START();
  uart_rx_head = 0;
  uart_rx_tail = 0;
  CRITICAL_END();
}

int uart_rx_get_len()
{
  CRITICAL_STORE;
  int l;
  if(uart_flags & UART_RX_RING_OF) return UART_RX_SIZE;
  CRITICAL_START();
  l = uart_rx_head - uart_rx_tail;
  if (l < 0) l += UART_RX_SIZE;
  CRITICAL_END();
  return l;
}

//reads n characters from buffer head
//n must be less than buffer length
int uart_last(char *p, int len)
{
  CRITICAL_STORE;
  int l;
  CRITICAL_START();
  l = uart_rx_head - len;
  if ( l < 0 )
    {
      l = -l;
      memcpy(p, uart_rx_bfr + UART_RX_SIZE - l, l);
      memcpy(p + l, uart_rx_bfr, uart_rx_head);
    }
  else memcpy(p, uart_rx_bfr + l, len);
  CRITICAL_END();
return len;
}

int uart_read_bfr(char *p, int len, char peek)
{
  CRITICAL_STORE;
  int l;
  CRITICAL_START();
  if(uart_flags & UART_RX_RING_OF) l = UART_RX_SIZE;
  else l = uart_rx_head - uart_rx_tail;
  if (l == 0)
    {
      CRITICAL_END();
      return 0;
    }
  if( l < 0 ) l += UART_RX_SIZE;
  if( len < l ) l = len;
  else len = l;
  if( len > (UART_RX_SIZE - uart_rx_tail) ) len = UART_RX_SIZE - uart_rx_tail;
  memcpy(p, uart_rx_bfr + uart_rx_tail, len);
  if( len < l )
    {
      p += len;
      len = l - len;
      memcpy(p, uart_rx_bfr, len);
      if( peek == 0 ) uart_rx_tail = len;
      } else if( peek == 0 ) uart_rx_tail += len;
  CRITICAL_END();
  return l;
}

int uart_read(char *bfr, int len)
{
  CRITICAL_STORE;
  int l;
  krn_mutex_lock(&uart_mutex_rx);
  while(len)
  {
    CRITICAL_START();
    l = uart_read_bfr(bfr, len, 0);
    len -= l;
    if(len == 0) {
      CRITICAL_END();
      break;
    }
    bfr += l;
    uart_sleep_thread_rx = krn_thread_current;
    krn_thread_lock(uart_sleep_thread_rx);
    krn_dispatch();
    CRITICAL_END();
  }
  krn_mutex_unlock(&uart_mutex_rx);
  return 0;
}

int uart_write_bfr(char *bfr, int len)
{
  int l;
  l = uart_tx_len();
  if(l <= 0) return 0;
  if (len > l) len = l;
  if( (uart_tx_head + len) > UART_TX_SIZE)
  {
    l = UART_TX_SIZE - uart_tx_head;
    memcpy(uart_tx_bfr + uart_tx_head, bfr, l);
    bfr += l;
    uart_tx_head = len - l;
    memcpy(uart_tx_bfr, bfr, uart_tx_head);
  }
  else
  {
    memcpy(uart_tx_bfr + uart_tx_head, bfr, len);
    uart_tx_head += len;
    if(uart_tx_head >= UART_TX_SIZE) uart_tx_head = 0;
  }
  return len;
}

void uart_write(char *bfr, int len)
{
  CRITICAL_STORE;
  int l;
  krn_mutex_lock(&uart_mutex_tx);
  while(len)
  {
    CRITICAL_START();
    l = uart_write_bfr(bfr, len);
    uart_start_tx();
    len -= l;
    if(len == 0) {
      CRITICAL_END();
      break;
    }
    bfr += l;
    uart_sleep_thread_tx = krn_thread_current;
    krn_thread_lock(uart_sleep_thread_tx);
    krn_dispatch();
    CRITICAL_END();
  }
  krn_mutex_unlock(&uart_mutex_tx);
}

int uart_init(uint32_t baudrate)
{
  int status;
  uart_flags = 0;
  uart_sleep_thread_tx = 0;
  uart_sleep_thread_rx = 0;
  UART1_DeInit();
  UART1_Init (baudrate, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO,
              UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE);
  krn_mutex_init(&uart_mutex_tx);
  krn_mutex_init(&uart_mutex_rx);
  UART1->CR2 |= UART1_CR2_RIEN;
  return (status);
}

char uart_putchar (char c)
{
  uart_write(&c, 1);
  return (c);
}


#if defined(__CSMC__)
char putchar (char c)
{
    return (uart_putchar(c));
}
#endif


#if defined(__RCSTM8__)
int putchar (char c)
{
    uart_putchar(c);
    return (1);
}
#endif


#if defined(__IAR_SYSTEMS_ICC__)
size_t __write(int handle, const unsigned char *buf, size_t bufSize)
{
    size_t chars_written = 0;
    
    if (handle == -1)
    {
      chars_written = (size_t)0; 
    }
    else if ((handle != 1) && (handle != 2))
    {
      chars_written = (size_t)-1; 
    }
    else
    {
        while (chars_written < bufSize)
        {
            uart_putchar (buf[chars_written]);
            chars_written++;
        }
    }
    
    return (chars_written);
}
#endif
