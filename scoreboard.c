#include <stdint.h>

#include "sysdefs.h"
#include "scoreboard.h"

void init_scoreboard(struct scoreboard_t *s, const int16_t x, const int16_t y, const uint8_t center, const uint8_t hex)
{
	s->x		= x;
	s->y		= y;
	s->hex		= hex;
	s->center	= center;
	s->score	= 0;
}

uint8_t *update_scoreboard(const struct scoreboard_t *s, uint8_t *reg)
{
	int8_t		i, xo;
	uint8_t		digit;
	uint16_t	show;

	int8_t		digitwidth = 12;

	static const uint16_t ten[4] = { 1, 10, 100, 1000 };
	static const uint16_t hex[4] = { 1, 0x10, 0x100, 0x1000 };

	xo = 0; // x-offset - used to center the scoreboard
	show = 0;
	for (i=3 ; i >= 0 ; i--)
	{
		if (s->hex) {
			digit = (s->score / hex[i]) % 16;
		}
		else
		{
			digit = (s->score / ten[i]) % 10;
		}
		// if this is the first digit being shown...
		// skip leading 0s for decimal mode, show all 4 in hex mode
		if ((!show) && ( digit > 0 || i == 0 || s->hex)) {
			show=1;
			switch (s->center) {
			case SB_center:
				{
					xo = 0-((i+1)*digitwidth)/2;
					break;
				}
			case SB_right:
				{
					xo = -((i+1)*digitwidth);
					break;
				}
			default:
				{
				}
			}
			//xo = s->center ? 0: 0-((i+1)*digitwidth)/2;
		}
		if (show) {
			*reg++ = SPRlo(_digitbase + (digit * _tilebytes));
			*reg++ = SPRhi(_digitbase + (digit * _tilebytes));
			*reg++ = (s->x+xo) & 0xff;
			*reg++ = (s->x+xo) >> 8 & 0x03;
			*reg++ = s->y & 0xff;
			*reg++ = s->y >> 8 & 0x03;
			*reg++ = 0x0c;
			*reg++ = 0x50; // size=16x16
			xo += digitwidth;
		}
	}
	return(reg);
}
