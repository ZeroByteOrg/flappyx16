#ifndef __PIPES_H__
#define __PIPES_H__

#include <stdint.h>

#define _pipespeed		9
#define _pipespacing	8
#define _pipegap		3

#define TILE_gfx			0x10
#define TILE_space			0x1f

#define TILE_pipetopl		TILE_gfx+1
#define TILE_pipetopr		TILE_gfx
#define TILE_pipebotl		TILE_gfx+3
#define TILE_pipebotr		TILE_gfx+2
#define TILE_pipel			TILE_gfx+4
#define TILE_piper			TILE_gfx+5
#define TILE_ground			TILE_gfx+6
#define TILE_underground	TILE_gfx+7

typedef struct pipe_t {
  uint8_t
	column,
	height,
	active,
	speed,
	h1,
	h2;
  int16_t
	ceiling,
	floor,
	scroll;
} pipe_t;

//extern uint8_t hit_pipes(pipe_t *pipe, const int16_t x, const int16_t y);
extern void init_pipes(pipe_t *pipe);
extern void clear_screen();
extern void update_pipes(pipe_t *pipe);

#endif


