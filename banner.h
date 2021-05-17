#ifndef __BANNER_H__
#define __BANNER_H__

#include <stdint.h>

#define _bannerbytes	(64 * 64 / 2)

typedef struct banner_t {
  uint16_t	addr;
  uint8_t	spec;			// same as sprite reg7: hhwwpppp
							// h=height w=width p=palette offset
  int16_t	x, y, target;
  int8_t	speed;
  uint8_t	ntiles;
  uint8_t	cols;			// number of sprite tiles per row
  uint8_t	tile_h, tile_w; // derived from spec @ init
  uint8_t	step;			// amt to add to SPR ADDR for each tile
} banner_t;

extern void init_banner(banner_t *banner, uint16_t addr, uint8_t tilespec, uint8_t layout);
extern uint8_t *update_banner(banner_t *banner, uint8_t *spregs);
extern int16_t set_medalcolor(const uint16_t score, const uint16_t hiscore);

#endif
