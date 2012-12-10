#include <stdio.h>

#include "ltkrn.h"
#include "uart.h"
#include "stm8s.h"
#include "stm8s_tim4.h"

#define OS_THREAD_STACK 0x30
#define MAIN_THREAD_STACK 0x30

#define main_thread_stack (void*)(KRN_STACKFRAME - 1 * MAIN_THREAD_STACK)
#define btn_thread_stack  (void*)(KRN_STACKFRAME - 2 * MAIN_THREAD_STACK)
#define io_thread_stack   (void*)(KRN_STACKFRAME - 3 * MAIN_THREAD_STACK)

////
#define HD44780_RS    0x02
#define HD44780_E     0x08
#define HD44780_RW    0x04
#define HD44780_DATA  0xF0
#define HD44780_SH    4
#define HD44780_PORT  GPIOC

#define AD9850_DATA  GPIOB
#define AD9850_SYNC  GPIOC
#define AD9850_SDATA 0x40
#define AD9850_WCLK  0x80
#define AD9850_FQUP  0x40
#define AD9850_RST   0x20

void ad8950_init()
{
  uint8_t p;
//  CRITICAL_STORE;
//  CRITICAL_START();
  GPIO_Init(AD9850_DATA, 0xFF, GPIO_MODE_OUT_PP_HIGH_FAST);
  GPIO_Init(AD9850_SYNC, AD9850_WCLK | AD9850_FQUP | AD9850_RST, GPIO_MODE_OUT_PP_HIGH_FAST);
  p = (uint8_t)AD9850_SYNC->ODR & ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  AD9850_DATA->ODR = 0;
  AD9850_SYNC->ODR = p;
  asm("nop\n");
  AD9850_SYNC->ODR = p | AD9850_RST;
  asm("nop\n");
  AD9850_SYNC->ODR = p;
  asm("nop\n");
  AD9850_SYNC->ODR = p | AD9850_WCLK;
  asm("nop\n");
  AD9850_SYNC->ODR = p;
  asm("nop\n");
  AD9850_SYNC->ODR = p | AD9850_FQUP;
  asm("nop\n");
  AD9850_SYNC->ODR = p;
  asm("nop\n");
//  CRITICAL_END();
}

void ad8950_freq(uint32_t freq)
{
  uint8_t p;
  CRITICAL_STORE;
  CRITICAL_START();
  p = (uint8_t)AD9850_SYNC->ODR & ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  AD9850_SYNC->ODR = p;
  asm("nop\n");
  AD9850_DATA->ODR = 0;
  AD9850_SYNC->ODR = p | AD9850_WCLK;
  asm("nop\n");
  AD9850_SYNC->ODR = p;
  asm("nop\n");
  AD9850_DATA->ODR = (freq >> 24) & 0xFF;
  AD9850_SYNC->ODR = p | AD9850_WCLK;
  asm("nop\n");
  AD9850_SYNC->ODR = p;
  asm("nop\n");
  AD9850_DATA->ODR = (freq >> 16) & 0xFF;
  AD9850_SYNC->ODR = p | AD9850_WCLK;
  asm("nop\n");
  AD9850_SYNC->ODR = p;
  asm("nop\n");
  AD9850_DATA->ODR = (freq >> 8) & 0xFF;
  AD9850_SYNC->ODR = p | AD9850_WCLK;
  asm("nop\n");
  AD9850_SYNC->ODR = p;
  asm("nop\n");
  AD9850_DATA->ODR = (freq >> 0) & 0xFF;
  AD9850_SYNC->ODR = p | AD9850_WCLK;
  asm("nop\n");
  AD9850_SYNC->ODR = p;
  asm("nop\n");
  //*/
  AD9850_SYNC->ODR = p | AD9850_FQUP;
  asm("nop\n");
  AD9850_SYNC->ODR = p;
  asm("nop\n");
  CRITICAL_END();
}

void ad8950_freqs(uint32_t freq)
{
  uint8_t p;
  uint8_t i, d;
  CRITICAL_STORE;
  CRITICAL_START();
  p = (uint8_t)AD9850_SYNC->ODR & ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  AD9850_SYNC->ODR = p;
  asm("nop\n");
  d = (freq ) & 0xFF;
  for(i = 0; i < 8; i++)
  {
    AD9850_DATA->ODR = ((d>>1) & 1) ? 0xC0 : 0;
    AD9850_SYNC->ODR = p | AD9850_WCLK;
    asm("nop\n");
    AD9850_SYNC->ODR = p;
    asm("nop\n");
  }
  d = (freq >> 8) & 0xFF;
  for(i = 0; i < 8; i++)
  {
    AD9850_DATA->ODR = ((d>>1) & 1) ? 0xC0 : 0;
    AD9850_SYNC->ODR = p | AD9850_WCLK;
    asm("nop\n");
    AD9850_SYNC->ODR = p;
    asm("nop\n");
  }
  d = (freq >> 16) & 0xFF;
  for(i = 0; i < 8; i++)
  {
    AD9850_DATA->ODR = ((d>>1) & 1) ? 0xC0 : 0;
    AD9850_SYNC->ODR = p | AD9850_WCLK;
    asm("nop\n");
    AD9850_SYNC->ODR = p;
    asm("nop\n");
  }
  d = (freq >> 24) & 0xFF;
  for(i = 0; i < 8; i++)
  {
    AD9850_DATA->ODR = ((d>>1) & 1) ? 0xC0 : 0;
    AD9850_SYNC->ODR = p | AD9850_WCLK;
    asm("nop\n");
    AD9850_SYNC->ODR = p;
    asm("nop\n");
  }
  d = 0;
  for(i = 0; i < 8; i++)
  {
    AD9850_DATA->ODR = ((d>>1) & 1) ? 0xC0 : 0;
    AD9850_SYNC->ODR = p | AD9850_WCLK;
    asm("nop\n");
    AD9850_SYNC->ODR = p;
    asm("nop\n");
  }
  AD9850_SYNC->ODR = p | AD9850_FQUP;
  asm("nop\n");
  AD9850_SYNC->ODR = p;
  asm("nop\n");
  CRITICAL_END();
}


void ad8950_phase(uint8_t phase)
{
  uint8_t p;
  CRITICAL_STORE;
  CRITICAL_START();
  p = (uint8_t)AD9850_SYNC->ODR & ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  AD9850_DATA->ODR = phase & 0xF8;
  AD9850_SYNC->ODR = p | AD9850_WCLK;
  AD9850_SYNC->ODR = p;
  AD9850_SYNC->ODR = p | AD9850_FQUP;
  AD9850_SYNC->ODR = p;
  CRITICAL_END();
}

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

krn_mutex mutex_printf;
extern char uart_putchar (char c);

static krn_thread thr_main, thr_btn, thr_io;

NEAR static uint8_t adc_data[540];
NEAR static uint8_t g_str[20];

static uint8_t flag_led;
static uint8_t g_cnt;

static NO_REG_SAVE void main_thread_func (void)
{
  uint32_t sleep_ticks;
  int j;
  while (1)
    {
        sleep_ticks = (flag_led) ? (KRN_FREQ / 1) : (KRN_FREQ / 4);
        //for(i = 0; i < sleep_ticks; i--) GPIO_WriteHigh(GPIOD, GPIO_PIN_0);
        //for(i = 0; i < sleep_ticks; i--) GPIO_WriteLow(GPIOD, GPIO_PIN_0);
        GPIO_WriteReverse(GPIOD, GPIO_PIN_0);
        krn_mutex_lock(&mutex_printf);
        for(j = 0; j < 500; j++) uart_putchar('-');
        krn_mutex_unlock(&mutex_printf);
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
    krn_mutex_lock(&mutex_printf);
    for(j = 0; j < 1; j++) uart_putchar(((char)g_cnt & 0x1F) + 0x41);
    krn_mutex_unlock(&mutex_printf);
    krn_sleep(KRN_FREQ/100);
    //krn_dispatch();
  }
}

static NO_REG_SAVE void io_thread_func (void)
{
  int i, j, k;
  /*
  krn_sleep(KRN_FREQ/2);
  hd44780_cmd(0x28);
  krn_sleep(1);
  hd44780_cmd(0x0c);
  krn_sleep(1);
  hd44780_cmd(0x01);
  krn_sleep(1);
  hd44780_out('S');
  krn_sleep(1);
  hd44780_out('c');
  krn_sleep(1);
  hd44780_out('y');
  krn_sleep(1);
  hd44780_out('l');
  krn_sleep(1);
  hd44780_out('d');
  krn_sleep(1);
  hd44780_out('.');
  krn_sleep(1);
  */
  ad8950_init();
  //ad8950_freq(463856468); //13.5MHz
  //ad8950_freqs(927712936); //27MHz
  ad8950_freqs(8590); //250Hz
  while(1)
  {
    adc_data[i] = i&0xFF;
    i = (i+1) & 0xFF;
    krn_mutex_lock(&mutex_printf);
    //j = GPIO_ReadInputPin(GPIOB, GPIO_PIN_1) ? 1 : 0;
    //k = GPIO_ReadInputPin(GPIOB, GPIO_PIN_0) ? 1 : 0;
    printf("%d\t%d\tsec=%d\tT=%d\tP=%d\n", i, g_cnt, krn_timer_cnt / KRN_FREQ, j, k);
    krn_mutex_unlock(&mutex_printf);
    sprintf(g_str, "%d", krn_timer_cnt / KRN_FREQ);
    k = strlen(g_str);
    for(j = 0; j < k; j++)
    {
        //hd44780_out(g_str[j]);
        //krn_sleep(1);
    }
    krn_sleep(KRN_FREQ/10);
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
