#ifndef __kout_h__
#define __kout_h__

#include "stm8s.h"

extern int8_t kout_u8h(char *s, uint8_t x);
extern int8_t kout_u16h(char *s, uint16_t x);
extern int8_t kout_u32h(char *s, uint32_t x);
extern char * kout_u32d(char *s, uint32_t x);

#endif
