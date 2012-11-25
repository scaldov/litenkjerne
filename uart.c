#include <stdio.h>
#include <stddef.h>

#include "stm8s.h"
#include "stm8s_uart1.h"

#include "uart.h"
#include "ltkrn.h"

static krn_mutex uart_mutex;

int uart_init(uint32_t baudrate)
{
  int status;
  UART1_DeInit();
  UART1_Init (baudrate, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO,
              UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE);
  krn_mutex_init(&uart_mutex);
  return (status);
}

char uart_putchar (char c)
{
  krn_mutex_lock(&uart_mutex);
  UART1_SendData8(c);
  while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET);
  krn_mutex_unlock(&uart_mutex);
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
