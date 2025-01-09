#ifndef __PGMSPACE_H_
#define __PGMSPACE_H_

#include <inttypes.h>

#define PROGMEM
#define PGM_P  const char *
#define PSTR(str) (str)

#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#define pgm_read_word(addr) (*(const unsigned short *)(addr))
#define pgm_read_dword(addr) (*(const unsigned long *)(addr))
#define pgm_read_float(addr) (*(const float *)(addr))

#define strlen_P(s) strlen((const char *)(s))
#define strcpy_P(dest, src) strcpy((char *)(dest), (const char *)(src))
#define strncpy_P(dest, src, n) strncpy((char *)(dest), (const char *)(src), (n))

#endif
