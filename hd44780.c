#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "stm8s.h"
#include "stm8s_uart1.h"

#include "hd44780.h"
#include "ltkrn.h"

volatile char hd44780_flags;
volatile int hd44780_tx_head;
volatile int hd44780_tx_tail;
krn_thread *hd44780_sleep_thread_tx;
char hd44780_tx_bfr[HD44780_TX_SIZE];
static krn_mutex hd44780_mutex_tx;

void hd44780_cmd(uint8_t cmd)
{
  uint8_t p = (uint8_t)HD44780_PORT->ODR & ~(HD44780_DATA | HD44780_RS | HD44780_E | HD44780_RW);
  uint8_t t = p | HD44780_E | ( ( (cmd & 0xF0) >> 4) << HD44780_SH);
  HD44780_PORT->ODR = t;
  HD44780_PORT->ODR = t & ~HD44780_E;
  t = p | HD44780_E | ( (cmd & 0x0F) << HD44780_SH);
  HD44780_PORT->ODR = t;
  HD44780_PORT->ODR = t & ~HD44780_E;
}

void hd44780_out(uint8_t data)
{
  uint8_t p = (uint8_t)HD44780_PORT->ODR & ~(HD44780_DATA | HD44780_RS | HD44780_E | HD44780_RW);
  p |= HD44780_RS;
  uint8_t t = p | HD44780_E | ( ( (data & 0xF0) >> 4) << HD44780_SH);
  HD44780_PORT->ODR = t;
  HD44780_PORT->ODR = t & ~HD44780_E;
  t = p | HD44780_E | ( (data & 0x0F) << HD44780_SH);
  HD44780_PORT->ODR = t;
  HD44780_PORT->ODR = t & ~HD44780_E;
}
////

void hd44780_tx_proc (void)
{
  char c;
  if(hd44780_tx_tail != hd44780_tx_head)
  {
    c = hd44780_tx_bfr[hd44780_tx_tail ++];
    if(c == 0)
    {
      if(hd44780_tx_tail >= HD44780_TX_SIZE) hd44780_tx_tail = 0;
      c = hd44780_tx_bfr[hd44780_tx_tail ++];
      hd44780_cmd(c);
    }
    else hd44780_out(c);
    if(hd44780_tx_tail >= HD44780_TX_SIZE) hd44780_tx_tail = 0;
  }
  else {
    hd44780_flags &= ~HD44780_TX;
    if(hd44780_sleep_thread_tx) {
      krn_thread_unlock(hd44780_sleep_thread_tx);
      krn_thread_move(hd44780_sleep_thread_tx, krn_thread_current);
      hd44780_sleep_thread_tx = 0;
      //krn_dispatch(); //uncomment for extra hardness
    }
  }
}

int hd44780_write_bfr(char *bfr, int len)
{
  int l;
  l = hd44780_tx_len();
  if(l <= 0) return 0;
  if (len > l) len = l;
  if( (hd44780_tx_head + len) > HD44780_TX_SIZE)
  {
    l = HD44780_TX_SIZE - hd44780_tx_head;
    memcpy(hd44780_tx_bfr + hd44780_tx_head, bfr, l);
    bfr += l;
    hd44780_tx_head = len - l;
    memcpy(hd44780_tx_bfr, bfr, hd44780_tx_head);
  }
  else
  {
    memcpy(hd44780_tx_bfr + hd44780_tx_head, bfr, len);
    hd44780_tx_head += len;
    if(hd44780_tx_head >= HD44780_TX_SIZE) hd44780_tx_head = 0;
  }
  return len;
}

void hd44780_write(char *bfr, int len)
{
  CRITICAL_STORE;
  int l;
  krn_mutex_lock(&hd44780_mutex_tx);
  while(len)
  {
    CRITICAL_START();
    l = hd44780_write_bfr(bfr, len);
    hd44780_flags |= HD44780_TX;
    len -= l;
    if(len == 0) {
      CRITICAL_END();
      break;
    }
    bfr += l;
    hd44780_sleep_thread_tx = krn_thread_current;
    krn_thread_lock(hd44780_sleep_thread_tx);
    krn_dispatch();
    CRITICAL_END();
  }
  krn_mutex_unlock(&hd44780_mutex_tx);
}

int hd44780_init()
{
  int status;
  hd44780_sleep_thread_tx = 0;
  hd44780_flags = 0;
  krn_mutex_init(&hd44780_mutex_tx);
  hd44780_cmd(0x28);
  krn_sleep(1);
  hd44780_cmd(0x0c);
  krn_sleep(1);
  hd44780_cmd(0x01);
  krn_sleep(1);
  return (status);
}
