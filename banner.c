#include <stdint.h>
#include <stdlib.h> // for abs()
#include "banner.h"
#include "sysdefs.h"

void init_banner(banner_t *banner, uint16_t addr, uint8_t tilespec, uint8_t layout)
{
	banner->x 		= 0;
	banner->y 		= 240; // default to an off-screen location
	banner->speed	= 0;
	banner->target	= 0;
	banner->addr	= addr; // this is in the spr reg format, not raw vram addr
	banner->spec	= tilespec;
	banner->cols	= layout & 0x0f;
	banner->ntiles	= banner->cols * (layout >> 4);
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
/*
	*spregs++ = banner->addr & 0xff;	// sprite data lo_address
	*spregs++ = banner->addr >> 8;	// sprite data hi_address
	*spregs++ = banner->x & 0xff;
	*spregs++ = banner->x >> 8 & 0x03; // think I can do w/o the mask...
	*spregs++ = banner->y & 0xff;
	*spregs++ = banner->y >> 8 & 0x03;
	*spregs++ = 0x0c;			// collision mask 0, zdepth=3, vflip=0, hflip=0
	*spregs++ = 0xf2;			// height=64, width=64, palette=2
	*spregs++ = banner->addr + 0x40 & 0xff;	// sprite data lo_address
	*spregs++ = banner->addr + 0x40 >> 8;	// sprite data hi_address
	*spregs++ = banner->x + 64 & 0xff;
	*spregs++ = banner->x + 64 >> 8 & 0x03; // think I can do w/o the mask...
	*spregs++ = banner->y & 0xff;
	*spregs++ = banner->y >> 8 & 0x03;
	*spregs++ = 0x0c;			// collision mask 0, zdepth=3, vflip=0, hflip=0
	*spregs++ = 0xf2;			// height=64, width=64, palette=2
	*spregs++ = banner->addr + 0x80 & 0xff;	// sprite data lo_address
	*spregs++ = banner->addr + 0x80 >> 8;	// sprite data hi_address
	*spregs++ = banner->x + 128 & 0xff;
	*spregs++ = banner->x + 128 >> 8 & 0x03; // think I can do w/o the mask...
	*spregs++ = banner->y & 0xff;
	*spregs++ = banner->y >> 8 & 0x03;
	*spregs++ = 0x0c;			// collision mask 0, zdepth=3, vflip=0, hflip=0
	*spregs++ = 0xf2;			// height=64, width=64, palette=2
*/
	return spregs;
}
