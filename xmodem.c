#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "stm8s.h"
#include "xmodem.h"
#include "uart.h"

#define XMODEM_SOH	0x01
#define XMODEM_EOT	0x04
#define XMODEM_ACK	0x06
#define XMODEM_NACK	0x15
#define XMODEM_C	0x43
//#define XMODEM_

NEAR static uint8_t xmodem_bfr[1 + 2 + 128 + 2];
char (*xmodem_recv_cb)(uint8_t *bfr, int len);

NEAR char xmodem_const_nack = XMODEM_NACK;
NEAR char xmodem_const_ack = XMODEM_ACK;
NEAR char xmodem_const_eot = XMODEM_EOT;

//	Polynomial: x^16 + x^12 + x^5 + 1 (0x1021)
//	Initial value: 0x0
//http://www.piconomic.co.za/fwlib/group___x_m_o_d_e_m.html

/*
uint16_t crc_xmodem_update (uint16_t crc, uint8_t data)
{
	int i;
	crc = crc ^ ((uint16_t)data << 8);
	for (i=0; i<8; i++)
	{
		if (crc & 0x8000)
			crc = (crc << 1) ^ 0x1021;
		else
			crc <<= 1;
	}
	return crc;
}
*/

//table of crc16(0..15)
const unsigned int crc16_table[16] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF, 
}; 

uint16_t crc16_upd (uint16_t crc, uint8_t data)
{
    crc = crc16_table[((crc >> 12) ^ (data >> 4)) & 0x0F] ^ (crc << 4);
    crc = crc16_table[((crc >> 12) ^ (data & 0x0F)) & 0x0F] ^ (crc << 4);
    return(crc);
}

uint16_t crc16 (uint8_t *data, int len)
{
	int i;
	uint16_t crc = 0;
	for( i = 0; i < len; i++)
	{
		crc = crc16_upd (crc, data[i]);
	}
	return crc;
}

char xmodem_recv()
{
  int i;
  uint16_t crc;
  uint8_t n,k;
  uart_rx_flush();
  i = 4;
  while(1) {
    uart_write("C", 1);
    uart_rx_wait();
    if(uart_rx_is_timeout()) {
      if(i == 0) return -1;
      i --;
    } else {
      uart_last(xmodem_bfr, 1);
      if(xmodem_bfr[0] == XMODEM_SOH) break;
      else return -1;
    }
    //receive header
  }
  n = 1;
  while(1) {
    uart_read(xmodem_bfr, 1 + 2 + 128 + 2);
    if(xmodem_bfr[0] != XMODEM_SOH) goto nack;
    k = xmodem_bfr[1];
    if(k != (xmodem_bfr[2] ^ 0xFF)) goto nack;
    crc = crc16(&xmodem_bfr[3], 128);
    if( xmodem_bfr[131] != (crc >> 8)) goto nack;
    if( xmodem_bfr[132] != (crc & 0xFF)) goto nack;
    if( k == n ) {
      xmodem_recv_cb(&xmodem_bfr[3], 128);
      n++;
    }
    uart_write(&xmodem_const_ack, 1);
  nack:
    uart_write(&xmodem_const_nack, 1);
    continue;
  }
}
