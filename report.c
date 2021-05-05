#include <stdint.h>
#include <stdlib.h> // for abs()
#include "report.h"
#include "sysdefs.h"

void init_report(report_t *report, int16_t x, int16_t y)
{
	report->x 		= x;
	report->y 		= y;
	report->speed	= 0;
	report->target	= y;
	report->addr	= SPRlo(_reportbase);
	report->addr	|= SPRhi(_reportbase) << 8;
}

uint8_t *update_report(report_t *report, uint8_t *spregs)
{
	if (report->speed) {
		if (abs(report->y - report->target) <= report->speed)
		{
			report->speed = 0;
			report->y = report->target;
		}
		else
		{
			if (report->y < report-> target)
				report->y += report->speed;
			else
				report->y -= report->speed;
		}
	}

	*spregs++ = report->addr & 0xff;	// sprite data lo_address
	*spregs++ = report->addr >> 8;	// sprite data hi_address
	*spregs++ = report->x & 0xff;
	*spregs++ = report->x >> 8 & 0x03; // think I can do w/o the mask...
	*spregs++ = report->y & 0xff;
	*spregs++ = report->y >> 8 & 0x03;
	*spregs++ = 0x0c;			// collision mask 0, zdepth=3, vflip=0, hflip=0
	*spregs++ = 0xf2;			// height=64, width=64, palette=2
	*spregs++ = report->addr + 0x40 & 0xff;	// sprite data lo_address
	*spregs++ = report->addr + 0x40 >> 8;	// sprite data hi_address
	*spregs++ = report->x + 64 & 0xff;
	*spregs++ = report->x + 64 >> 8 & 0x03; // think I can do w/o the mask...
	*spregs++ = report->y & 0xff;
	*spregs++ = report->y >> 8 & 0x03;
	*spregs++ = 0x0c;			// collision mask 0, zdepth=3, vflip=0, hflip=0
	*spregs++ = 0xf2;			// height=64, width=64, palette=2
	*spregs++ = report->addr + 0x80 & 0xff;	// sprite data lo_address
	*spregs++ = report->addr + 0x80 >> 8;	// sprite data hi_address
	*spregs++ = report->x + 128 & 0xff;
	*spregs++ = report->x + 128 >> 8 & 0x03; // think I can do w/o the mask...
	*spregs++ = report->y & 0xff;
	*spregs++ = report->y >> 8 & 0x03;
	*spregs++ = 0x0c;			// collision mask 0, zdepth=3, vflip=0, hflip=0
	*spregs++ = 0xf2;			// height=64, width=64, palette=2
	return spregs;
}
