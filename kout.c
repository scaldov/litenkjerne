#include "kout.h"

int8_t kout_u8h(char *s, uint8_t x)
{
  uint8_t t;
  t = x >> 4;
  t += 6;
  t += (t & 0x10) ? 0x31 : 0x2A;
  *(s++) = t;
  t = x & 0xF;
  t += 6;
  t += (t & 0x10) ? 0x31 : 0x2A;
  *(s++) = t;
  *(s++) = 0;
  return 2;
}

int8_t kout_u16h(char *s, uint16_t x)
{
  kout_u8h(s, x >> 8);
  s += 2;
  kout_u8h(s, x & 0xFF);
  return 4;
}

int8_t kout_u32h(char *s, uint32_t x)
{
  kout_u8h(s, x >> 24);
  s += 2;
  kout_u8h(s, (x >> 16) & 0xFF);
  s += 2;
  kout_u8h(s, (x >> 8) & 0xFF);
  s += 2;
  kout_u8h(s, x & 0xFF);
  return 8;
}

typedef struct
{
  uint32_t quot;
  uint8_t rem;
} ltkrn_divmodu3210struc;

inline static ltkrn_divmodu3210struc ltkrn_divmodu3210(uint32_t n)
{
  uint32_t qq;
  ltkrn_divmodu3210struc r;
  r.quot = n >> 1;
  r.quot += r.quot >> 1;
  r.quot += r.quot >> 4;
  r.quot += r.quot >> 8;
  r.quot += r.quot >> 16;
  qq = r.quot;
  r.quot >>= 3;
  r.rem = (uint8_t)(n - ((r.quot << 1) + (qq & ~7ul)));
  if(r.rem > 9)
    {
        r.rem -= 10;
        r.quot++;
    }
  return r;
}

char * kout_u32d(char *s, uint32_t x)
{
  ltkrn_divmodu3210struc r;
  s += 11; 
  *--s = 0;
  do
  {
     r = ltkrn_divmodu3210(x);
      *--s = r.rem + 0x30;
      x = r.quot;
   }
   while (x != 0);
   return s;
}
