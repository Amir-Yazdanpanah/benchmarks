#ifndef QRENCODE_H
#define QRENCODE_H

#define PROGMEM
#define memcpy_P memcpy
#define __LPM(x) (*(x))
#define pgm_read_word(x) (*(x))

extern unsigned char *strinbuf;
extern unsigned char *qrframe;

void initframe (void);
void freeframe (void);
unsigned initeccsize (unsigned char ecc, unsigned size);
unsigned initecc (unsigned char level, unsigned char version);

extern unsigned char WD, WDB;
#include "qrbits.h"

void qrencode (void);
void freeecc (void);

#endif