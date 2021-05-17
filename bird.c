#include "sysdefs.h"
#include "bird.h"

void flap_bird(bird_t *bird)
{
	bird->flapping = bird->y - 16;
	bird->vy = bird->thrust;
	bird->animsteps++;
	bird->frame = 0;
	bird->animspeed = 2;
}

uint8_t check_bird(bird_t *bird, const int16_t ceil, const int16_t floor)
{
	if (bird->y < ceil || bird->y >= floor)
	{
		bird->animsteps = 0; // disable bird animation
		bird->alive		= 0;
		bird->vy		= 0;
		bird->flapping	= 0;
		return 0;
	}
	else
		return 1;
}

void init_bird(bird_t *bird, const uint16_t x, const uint16_t y)
{
	// we don't set basegravity or thrust - the game should set these
	// whenever difficulty settings are changed/applied
	bird->x			= x;
	bird->y			= y;
	bird->vx		= 0;
	bird->vy		= 0;
	bird->alive		= 1;
	bird->pose 		= 2;
	bird->frame		= 0;
	bird->flapping	= 0;
	bird->gravity	= 0;
	bird->animdelay	= 0; 	// this is a counter, not a parameter
	bird->animspeed	= _birdanimspeed;
	bird->animsteps	= _birdanimsteps;
}

uint8_t* update_bird(bird_t *bird, uint8_t *regs) 
{

	uint8_t frame;
	unsigned long addr;

	bird->vy += bird->gravity;
	bird->y += bird->vy/16;

	if (bird->alive) {
		// don't remove 'flapping' state until bird falls below where
		// the flap started - i.e. stay looking up at least that long
		if (bird->y >= bird->flapping && bird->vy > 0 )
		{
			bird->flapping=0;
				bird->animsteps = _birdanimsteps;
				bird->animspeed = _birdanimspeed;
		}
		if (bird->animsteps) // if the bird is animating....
		{
			if (bird->vy > 0)
			{
				// restore normal flapping animation speed at the apex
				bird->animsteps = _birdanimsteps;
				bird->animspeed = _birdanimspeed;
			}
			if (++bird->animdelay >= bird->animspeed)
			{
				// new animation frame (0-3)
				bird->frame = (bird->frame+bird->animsteps & _birdanimframes);
				bird->animdelay = 0;
			}
		}
	}
	// hold the up-facing pose if flapping
	if (bird->vy < -24 || bird->flapping)
	{
		bird->pose=0;
	}
	else if (bird->vy > 128) // 208
	{
		bird->pose=3;
	}
	else if (bird->vy > 96) // 128
	{
		bird->pose=2;
	}
	else
	{
		// this condition is so the bird will stay facing down
		// when it splats on the ground
		if (bird->y < _floor - 4)
			bird->pose=1;
	}
	// update sprite regs[]
	frame = (bird->frame == 3 ? 1 : bird->frame); // pingpong animation
	addr = _birdbase + _birdbytes * (3 * bird->pose + frame);
	regs[0] = SPRlo(addr);		// sprite data lo_address
	regs[1] = SPRhi(addr);		// sprite data hi_address
	regs[2] = bird->x & 0xff;
	regs[3] = bird->x >> 8;// & 0x03; // think I can do w/o the mask...
	regs[4] = bird->y & 0xff;
	regs[5] = bird->y >> 8;// & 0x03;
	regs[6] = 0x0c;			// collision mask 0, zdepth=3, vflip=0, hflip=0
	regs[7] = 0xa0;			// height=32, width=32, palette=0
	return(&regs[8]);
}
