#include <stdio.h>

#include "stm8s.h"
#include "stm8s_tim4.h"
#include "ltkrn.h"
#include "ad8950.h"
#include "hd44780.h"
#include "uart.h"
#include "kout.h"

#define OS_THREAD_STACK 0x30
#define MAIN_THREAD_STACK 0x30

#define main_thread_stack (void*)(KRN_STACKFRAME - 1 * MAIN_THREAD_STACK)
#define btn_thread_stack  (void*)(KRN_STACKFRAME - 2 * MAIN_THREAD_STACK)
#define io_thread_stack   (void*)(KRN_STACKFRAME - 3 * MAIN_THREAD_STACK)

#include "stm8s_i2c.h"
#include "stm8s_tim1.h"
#include "stm8s_spi.h"

krn_mutex mutex_printf;
extern char uart_putchar (char c);

static krn_thread thr_main, thr_btn, thr_io;

NEAR static uint8_t adc_data[540];
NEAR static char g_str[0x20];

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
        //*
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
    btn = adc_data[btn_old];
    /*
    krn_mutex_lock(&mutex_printf);
    for(j = 0; j < 1; j++) uart_putchar(((char)g_cnt & 0x1F) + 0x41);
    krn_mutex_unlock(&mutex_printf);
    //*/
    krn_sleep(KRN_FREQ/100);
    //krn_dispatch();
  }
}

void kout_uart(char *s)
{
  krn_mutex_lock(&mutex_printf);
  uart_write(s, strlen(s));
  krn_mutex_unlock(&mutex_printf);
}

static NO_REG_SAVE void io_thread_func (void)
{
  int i, j, k;
  uint32_t f = 0;
  //*
  hd44780_init();
  krn_sleep(KRN_FREQ);
  hd44780_init();
  hd44780_write("\0\xC0Skjoldur.", 11);
  //*/
  //ad8950_init();
  //ad8950_freq(463856468); //13.5MHz
  //ad8950_freq(927712936); //27MHz
  //ad8950_freq(1030792151); //30MHz
  //ad8950_freq(8589935); //250KHz
  while(1)
  {
    //adc_data[i] = i&0xFF;
    i = (i+1) & 0xFF;
    //krn_mutex_lock(&mutex_printf);
    //j = GPIO_ReadInputPin(GPIOB, GPIO_PIN_1) ? 1 : 0;
    //k = GPIO_ReadInputPin(GPIOB, GPIO_PIN_0) ? 1 : 0;
    //printf("%d\t%d\tsec=%d\tT=%d\tP=%d\n", i, g_cnt, krn_timer_cnt / KRN_FREQ, j, k);
    //krn_mutex_unlock(&mutex_printf);
    //*
    kout_u32h(g_str, f);
    kout_uart("f=");
    hd44780_write("\0\200f=", 4);
    kout_uart(g_str);
    hd44780_write(g_str, 8);
    kout_uart(" sec=");
    kout_uart(kout_u32d(g_str + 12, krn_timer_cnt / KRN_FREQ));
    kout_uart("\n");
    //mode 1 demonstrates use of sliding ring buffer
    //*
    if(uart_rx_ev_get()) {
      j = uart_rx_get_len();
      uart_peek( g_str, j);
      kout_uart(" bfr=");
      uart_write(g_str, j);
      kout_uart("\n");
      uart_rx_ev_clr();
    }
    //*/
    //mode 2 demonstrates receive buffer larger than ring buffer
    /*
    uart_read( g_str, 0x20);
    kout_uart(" bfr=");
    uart_write(g_str, 0x20);
    kout_uart("\n");
    //*/
    //ad8950_phase(f & 0xFF);
    f+=1;//000000;
    krn_sleep(KRN_FREQ/2);
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
  GPIO_Init(GPIOC, GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1, GPIO_MODE_OUT_PP_HIGH_FAST);
  GPIO_WriteHigh(GPIOA, GPIO_PIN_3);
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
