#ifndef __BIRD_H__
#define __BIRD_H__

#include <stdint.h>

#define _birdbytes 0x200

typedef struct bird_t
{
  int16_t
    x, y,		// >>4 to get actual screen coordinates. (uses 4-bit fixed point)
    vx, vy,
    flapping;	// height where the bird flapped (0 = bird not flapping)
  uint8_t
	alive,
    pose,		// 0="up" 1="level" 2="dn" 3="straight down"
    frame,		// current animation frame
    animspeed,	// frames to pause between animation frames
    animsteps,	// number of frames to advance for each animation frame
    animdelay,	// frame delay for animation speed
    gravity;
} bird_t;

extern void flap_bird(bird_t *bird);
extern uint8_t check_bird(bird_t *bird, const int16_t ceil, const int16_t floor);
extern void init_bird(bird_t *bird, const uint16_t x, const uint16_t y);
extern uint8_t*	update_bird(bird_t *bird, uint8_t *regs);

#endif
