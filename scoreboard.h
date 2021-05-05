#ifndef __SCOREBOARD_H__
#define __SCOREBOARD_H__

#include <stdint.h>

#define SB_center	0
#define SB_left		1
#define	SB_right	2
#define	SB_decimal	0
#define	SB_hex		1

typedef struct scoreboard_t {
  uint16_t score;
  int16_t x, y;
  uint8_t center, hex;
} scoreboard_t;

extern void init_scoreboard(struct scoreboard_t *s, const int16_t x, const int16_t y, const uint8_t center, const uint8_t hex);
extern uint8_t *update_scoreboard(const struct scoreboard_t *s, uint8_t *reg);

#endif
