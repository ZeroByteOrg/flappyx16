#include <stdint.h>
#include <stdlib.h> // for abs()
#include "banner.h"
#include "sysdefs.h"

void init_banner(banner_t *banner, int16_t x, int16_t y, uint8_t msg)
{
	banner->x 		= x;
	banner->y 		= y;
	banner->speed	= 0;
	banner->target	= y;
	banner_setmsg(banner,msg);
}

void banner_setmsg(banner_t *banner, uint8_t message)
{
	unsigned long addr;

	addr = _bannerbase + _bannerbytes * 3 * message;
	banner->addr  = SPRlo(addr);
	banner->addr |= SPRhi(addr) << 8;
}

uint8_t *update_banner(banner_t *banner, uint8_t *spregs)
{
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
	return spregs;
}
