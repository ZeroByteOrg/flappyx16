#ifndef __BANNER_H__
#define __BANNER_H__

#include <stdint.h>

#define _bannerbytes	(64 * 64 / 2)

#define MSG_TITLE		0
#define MSG_GETREADY	1
#define MSG_GAMEOVER	2

typedef struct banner_t {
  uint16_t	addr;
  int16_t	x, y, target;
  int8_t	speed;
} banner_t;

extern void init_banner(banner_t *banner, int16_t x, int16_t y, uint8_t msg);
extern void banner_setmsg(banner_t *banner, uint8_t message);
extern uint8_t *update_banner(banner_t *banner, uint8_t *spregs);

#endif
