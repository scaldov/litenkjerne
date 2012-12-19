#include "stm8s.h"
#include "ad8950.h"
#include "ltkrn.h"

void ad8950_init()
{
  CRITICAL_STORE;
  CRITICAL_START();
  I2C_DeInit();
  TIM1_DeInit();
  SPI_DeInit();
  //AD9850_DATA->DDR = 0xFF;
  //AD9850_DATA->CR1 = 0xFF;
  //AD9850_DATA->CR2 = 0x0;
  //AD9850_SYNC->DDR = AD9850_WCLK | AD9850_FQUP | AD9850_RST;
  //AD9850_SYNC->CR1 = AD9850_WCLK | AD9850_FQUP | AD9850_RST;
  //AD9850_SYNC->CR2 = AD9850_WCLK | AD9850_FQUP | AD9850_RST;
  GPIO_Init(AD9850_SYNC, AD9850_WCLK | AD9850_FQUP | AD9850_RST, GPIO_MODE_OUT_PP_HIGH_FAST);
#ifdef AD9850_PART
  GPIO_Init(AD9850_LDATA, AD9850_LMASK, GPIO_MODE_OUT_PP_HIGH_FAST);
  GPIO_Init(AD9850_HDATA, AD9850_HMASK, GPIO_MODE_OUT_PP_HIGH_FAST);
#else
  GPIO_Init(AD9850_DATA, 0xFF, GPIO_MODE_OUT_PP_HIGH_FAST);
#endif
  AD9850_WRITE(0);
  AD9850_DATA->ODR = 0;
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  asm("nop\n");
  AD9850_SYNC->ODR |= AD9850_RST;
  asm("nop\n");
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  asm("nop\n");
  AD9850_SYNC->ODR |= AD9850_WCLK;
  asm("nop\n");
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  asm("nop\n");
  AD9850_SYNC->ODR |= AD9850_FQUP;
  asm("nop\n");
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  asm("nop\n");
  CRITICAL_END();
}

void ad8950_freq(uint32_t freq)
{
  uint8_t ll, lh, hl ,hh;
  CRITICAL_STORE;
  CRITICAL_START();
  ll = freq & 0xFF;
  lh = (freq >> 8) & 0xFF;
  hl = (freq >> 16) & 0xFF;
  hh = freq >> 24;
  //*
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  AD9850_WRITE(0);
  AD9850_SYNC->ODR |= AD9850_WCLK;
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  AD9850_WRITE(hh);
  AD9850_SYNC->ODR |= AD9850_WCLK;
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  AD9850_WRITE(hl);
  AD9850_SYNC->ODR |= AD9850_WCLK;
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  AD9850_WRITE(lh);
  AD9850_SYNC->ODR |= AD9850_WCLK;
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  AD9850_WRITE(ll);
  AD9850_SYNC->ODR |= AD9850_WCLK;
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  AD9850_SYNC->ODR |= AD9850_FQUP;
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  //*/
  CRITICAL_END();
}

void ad8950_phase(uint8_t phase)
{
  CRITICAL_STORE;
  CRITICAL_START();
  AD9850_WRITE(phase & 0xF8);
  AD9850_SYNC->ODR |= AD9850_WCLK;
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  AD9850_SYNC->ODR |= AD9850_FQUP;
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  CRITICAL_END();
}


void ad8950_freq_serial(uint32_t freq)
{
  uint8_t i, d;
  CRITICAL_STORE;
  CRITICAL_START();
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  asm("nop\n");
  d = (freq ) & 0xFF;
  for(i = 0; i < 8; i++)
  {
    AD9850_DATA->ODR = (d & 1) ? AD9850_SDATA : 0;
    d = (d >> 1);
  AD9850_SYNC->ODR |= AD9850_WCLK;
    asm("nop\n");
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
    asm("nop\n");
  }
  d = (freq >> 8) & 0xFF;
  for(i = 0; i < 8; i++)
  {
    AD9850_DATA->ODR = (d & 1) ? AD9850_SDATA : 0;
    d = (d >> 1);
  AD9850_SYNC->ODR |= AD9850_WCLK;
    asm("nop\n");
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
    asm("nop\n");
  }
  d = (freq >> 16) & 0xFF;
  for(i = 0; i < 8; i++)
  {
    AD9850_DATA->ODR = (d & 1) ? AD9850_SDATA : 0;
    d = (d >> 1);
  AD9850_SYNC->ODR |= AD9850_WCLK;
    asm("nop\n");
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
    asm("nop\n");
  }
  d = freq >> 24;
  for(i = 0; i < 8; i++)
  {
    AD9850_DATA->ODR = (d & 1) ? AD9850_SDATA : 0;
    d = (d >> 1);
  AD9850_SYNC->ODR |= AD9850_WCLK;
    asm("nop\n");
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
    asm("nop\n");
  }
  d = 0;
  for(i = 0; i < 8; i++)
  {
    AD9850_DATA->ODR = (d & 1) ? AD9850_SDATA : 0;
    d = (d >> 1);
  AD9850_SYNC->ODR |= AD9850_WCLK;
    asm("nop\n");
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
    asm("nop\n");
  }
  AD9850_SYNC->ODR |= AD9850_FQUP;
  asm("nop\n");
  AD9850_SYNC->ODR &= ~(AD9850_FQUP | AD9850_WCLK | AD9850_RST);
  asm("nop\n");
  CRITICAL_END();
}
