#include <stdio.h>

#include "stm8s.h"
#include "stm8s_tim4.h"
#include "ltkrn.h"
#include "ad8950.h"
#include "hd44780.h"
#include "uart.h"
#include "spi.h"
#include "kout.h"
#include "xmodem.h"

#define OS_THREAD_STACK 0x30
#define MAIN_THREAD_STACK 0x30

#define main_thread_stack (void*)(KRN_STACKFRAME - 1 * MAIN_THREAD_STACK)
#define btn_thread_stack  (void*)(KRN_STACKFRAME - 2 * MAIN_THREAD_STACK)
#define io_thread_stack   (void*)(KRN_STACKFRAME - 3 * MAIN_THREAD_STACK)

#include "stm8s_i2c.h"
#include "stm8s_tim1.h"
#include "stm8s_spi.h"

#define FRMT_IHEX 1
#define FRMT_HEX 2
#define FRMT_BIN 3


krn_mutex mutex_printf;
extern char uart_putchar (char c);

static krn_thread thr_main, thr_btn, thr_io;

NEAR static uint8_t spi_data[256];
NEAR static char g_str[0x20];
NEAR static char g_cmd[0x50];

NEAR static char g_frmt;
NEAR static uint32_t g_addr;
NEAR static uint32_t g_len;

static uint8_t flag_led;
static uint8_t g_cnt;

static NO_REG_SAVE void main_thread_func (void)
{
  uint32_t sleep_ticks;
  int j;
  while (1)
    {
        sleep_ticks = (flag_led) ? (KRN_FREQ / 1) : (KRN_FREQ / 4);
        GPIO_WriteReverse(GPIOD, GPIO_PIN_0);
        /*
        krn_mutex_lock(&mutex_printf);
        for(j = 0; j < 80; j++) uart_putchar('-');
        krn_mutex_unlock(&mutex_printf);
        //*/
        krn_sleep(sleep_ticks);
    }
}

static NO_REG_SAVE void btn_thread_func (void)
{
  int btn, btn_old;
  int j;
  while(1)
  {
    g_cnt += 1;
    btn = GPIO_ReadInputPin(GPIOB, GPIO_PIN_7) ? 1 : 0;
    if( btn != btn_old)
    {
      if (btn == 1) flag_led ^= 1;
    }
    btn_old = btn;
    krn_sleep(KRN_FREQ/100);
  }
}

void kout_uart(char *s)
{
  krn_mutex_lock(&mutex_printf);
  uart_write(s, strlen(s));
  krn_mutex_unlock(&mutex_printf);
}

char recv_cb(uint8_t *bfr, int len)
{
}

static NO_REG_SAVE void io_thread_func (void)
{
  char *str, *val;
  int i, j, k;
  uint32_t v;
  spi_master_init(1);
  while(1)
  {
    /*
	spi_data[0] = 0x9f;
        spi_data[1] = spi_data[2] = spi_data[3] = spi_data[4] = 0;
    	spi_tx_bfr = spi_data;
	spi_rx_bfr = spi_data;
	spi_tx_len = 1;
	spi_rx_len = 3;
        GPIOC->ODR &= ~0x10;
	spi_master_start();
	spi_master_wait();
        GPIOC->ODR |= 0x10;
    //*/
    str = g_cmd;
    i = 0x50 - 1;
    uart_rx_flush();
    kout_uart("\n>");
    do {
      do uart_read(str, 1); while uart_rx_is_timeout();
      //uart_write(str, 1);
      if( *str == '\012' ) break;
      if( *str == '\015' ) break;
      i --;
      str ++;
    } while(i);
    uart_write(g_cmd, 80);
    if(i == 0) {
      kout_uart("\nOOO!\n");
      continue;
    }
    *str = 0;
    if( !strncmp( g_cmd, "addr", 4) ) {
      kin_u32( kin_next(g_cmd + 4), &v);
      kout_uart("set addr to ");
      kout_u32h(g_str, v);
      kout_uart(g_str);
      kout_uart("\n");
      continue;
    }
    if( !strncmp( g_cmd, "frmt", 4) ) {
      val = kin_next(g_cmd + 4);
      if( !strncmp( val, "ihex", 4) ) g_frmt = FRMT_IHEX;
      else if( !strncmp( val, "bin", 4) ) g_frmt = FRMT_BIN;
      else if( !strncmp( val, "hex", 4) ) g_frmt = FRMT_HEX;
      continue;
    }
    if( !strncmp( g_cmd, "recv", 4) ) {
      xmodem_recv_cb = recv_cb;
      xmodem_recv();
      continue;
    }
    if( !strncmp( g_cmd, "send", 4) ) {
      continue;
    }
    if( !strncmp( g_cmd, "jedec", 4) ) {
      continue;
    }
    kout_uart("???\n");
  }
}

//krn_dispatch();

int main( void )
{
  int8_t stat;
  flag_led = 0;
  CLK_DeInit();
  CLK_HSICmd(ENABLE);
  CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV1);  //8-8 still normal
  CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1);
  GPIO_DeInit(GPIOD);
  GPIO_Init(GPIOD, GPIO_PIN_0, GPIO_MODE_OUT_PP_HIGH_FAST);
  GPIO_Init(GPIOB, GPIO_PIN_7, GPIO_MODE_IN_PU_NO_IT);
  GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_IN_PU_NO_IT);
  GPIO_Init(GPIOB, GPIO_PIN_1, GPIO_MODE_IN_PU_NO_IT);
  GPIO_Init(GPIOA, GPIO_PIN_3, GPIO_MODE_OUT_PP_HIGH_FAST);
  GPIO_Init(GPIOC, GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1, GPIO_MODE_OUT_PP_HIGH_FAST);
  GPIO_Init(GPIOC, GPIO_PIN_7, GPIO_MODE_IN_PU_NO_IT);
  GPIO_WriteHigh(GPIOA, GPIO_PIN_3);
  GPIOC->ODR |= 0x10;
  uart_init(115200);
  krn_tmp_stack();
  krn_thread_init();
  krn_thread_create(&thr_main, (void*)main_thread_func, (void*)1, 1, main_thread_stack, MAIN_THREAD_STACK);
  krn_thread_create(&thr_btn, (void*)btn_thread_func, (void*)2, 6, btn_thread_stack, MAIN_THREAD_STACK);
  krn_thread_create(&thr_io, (void*)io_thread_func, (void*)3, 1, io_thread_stack, MAIN_THREAD_STACK);
  krn_timer_init();
  krn_mutex_init(&mutex_printf);
  krn_run();
  while (1);
  return 0;
}
