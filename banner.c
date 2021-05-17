#include <stdint.h>
#include <stdlib.h> // for abs()
#include <cbm.h>	// for VERA definitions
#include "banner.h"
#include "sysdefs.h"
#include "flappy.h"

void init_banner(banner_t *banner, uint16_t addr, uint8_t tilespec, uint8_t layout)
{
	/* this init routine is more focused on getting the sprite display parameters
	 * properly set up for the desired banner, and not in initializing the banner
	 * location and motion values. The routine calling this should directly set those
	 * as required for the situation.
	 *  
	 * parameters:
	 * 		*banner = pointer to the banner data being intitialized
	 *  	addr = the sprite data VRAM address, shifted for use in the sprite register
	 * 				this should be the address of the first sprite tile, with the rest
	 * 				being computed on the fly
	 * 		tilespec = sprite size/color palette selection (see sprite register documentation)
	 * 		layout = rows x cols - rows stored in hi nybble, cols stored in lo nybble
	*/ 
	banner->x 		= 0;
	banner->y 		= 240; // default to an off-screen location
	banner->speed	= 0;
	banner->target	= 0;
	banner->addr	= addr; // this is in the spr reg format, not raw vram addr
	banner->spec	= tilespec;
	banner->cols	= layout & 0x0f;
	banner->ntiles	= banner->cols * (layout >> 4);
	// compute the tile H and W pixel counts from the register settings
	// register uses bits 4-5 for sprite widths and bits 6-7 for height
	// the values 0-3 map to pixel counts of 8,16,32,64
	// so the following calcs are essentially: pixels = 8*2^n
	banner->tile_h	= 8 << (tilespec >> 6);
	banner->tile_w	= 8 << ((tilespec & 0x30) >> 4);
	banner->step	= SPRaddrstep(banner->tile_h * banner->tile_w / 2);
}

uint8_t *update_banner(banner_t *banner, uint8_t *spregs)
{
	uint8_t i, c;
	uint16_t x, y, addr;
	
	if (banner->speed) {
		if (abs(banner->y - banner->target) <= banner->speed)
		{
			banner->speed = 0;
			banner->y = banner->target;
		}
		else
		{
			if (banner->y < banner-> target)
				banner->y += banner->speed;
			else
				banner->y -= banner->speed;
		}
	}
	addr = banner->addr;
	x = banner->x;
	y = banner->y;
	c = 0;
	for ( i = 0 ; i < banner->ntiles ; i++ ) {
		*spregs++ = addr & 0xff;	// sprite data lo_address
		*spregs++ = addr >> 8;		// sprite data hi_address
		addr += banner->step;
		*spregs++ = x & 0xff;
		*spregs++ = x >> 8 & 0x03; // think I can do w/o the mask...
		*spregs++ = y & 0xff;
		*spregs++ = y >> 8 & 0x03;
		++c;
		if (c == banner->cols) {
			c=0;
			x=banner->x;
			y += banner->tile_h;
		}
		else {
			x += banner->tile_w;
		}
		*spregs++ = 0x0c;	// collision mask 0, zdepth=3, vflip=0, hflip=0
		*spregs++ = banner->spec;
	}
	return spregs;
}

extern int16_t set_medalcolor(const uint16_t score, const uint16_t hiscore)
{
	#define medalpalette	0x1fa70
	
	int16_t h;
	
	static const uint8_t newhicolor[2][4] = {
		{ 0xc9, 0x0d, 0xc9, 0x0d },
		{ 0xff, 0x0f, 0x14, 0x0e }
	};

	static const uint8_t color[5][10] = {
		// none
		{0xa6,0x0b, 0xa6,0x0b, 0xa6,0x0b, 0xa6,0x0b, 0xa6,0x0b},
		// bronze
		{0x40,0x0c, 0x71,0x0e, 0xa6,0x0e, 0x60,0x0c, 0x40,0x08},
		// silver
		{0xbb,0x0b, 0xdd,0x0d, 0xdd,0x0e, 0x9a,0x09, 0x66,0x06},
		// gold
		{0x90,0x0c, 0xb0,0x0f, 0xfa,0x0f, 0x60,0x09, 0x30,0x05},
		// platinum
		{0xdd,0x0d, 0xee,0x0e, 0xff,0x0f, 0xbc,0x0b, 0x77,0x07}
	};
	if (score > hiscore) {
		h = score;
		load_vera (medalpalette + 12, &newhicolor[1][0], 4);
	}
	else {
		h = hiscore;
		load_vera (medalpalette + 12, &newhicolor[0][0], 4);
	}
	if (score >= 40)
	{
		load_vera(medalpalette, &color[4][0], 10);
		return(h);
	}	 
	if (score >= 30) 
	{
		load_vera(medalpalette, &color[3][0], 10);
		return(h);
	}	 
	if (score >= 20) 
	{
		load_vera(medalpalette, &color[2][0], 10);
		return(h);
	}	 
	if (score >= 10) 
	{
		load_vera(medalpalette, &color[1][0], 10);
		return(h);
	}
	load_vera(medalpalette, &color[0][0], 10);
	return(h);
}

