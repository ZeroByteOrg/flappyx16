#include <stdlib.h> // rand()
#include <cbm.h>	// VERA.x definitions

#include "sysdefs.h"
#include "pipes.h"
#include "input.h"

#define scrollmask ((32*16-1) << 2 | 0x03)

void init_pipes(pipe_t *pipe)
{
	uint16_t i;
	
	pipe->speed		= 8; // default value
	pipe->column	= 6;
	pipe->active	= 0;
	pipe->height	= 255;
	pipe->h1		= 255;
	pipe->h2		= 255;
	pipe->ceiling	= _ceiling;
	pipe->floor		= _floor;
	pipe->gapsize	= 3;  // default to 3, game changes w/ difficulty selection

	VERA.control	= 0;
	VERA.address	= VRAMlo(_mapbase);
	VERA.address_hi = VRAMhi(_mapbase) | VERA_INC_2;
	// clear all above-ground tiles in the layer 1 map
	for ( i=0 ; i<14*32 ; i++)
	{
		VERA.data0 = TILE_space; // blank tile
	}	
}

void update_pipes(pipe_t *pipe)
{
	int8_t row, col;
	uint8_t tile, i;
	int16_t last, current;
	
	// 32 columns * 16 pixels per column, plus two bits for subpixel amt.

	if (pipe->speed==0) { return; }
	last = pipe->scroll >> 2 & 0x0f;
	pipe->scroll = pipe->scroll + pipe->speed & scrollmask;
	// if not spawning pipes or in the process of spawning one, we're done
	if (!pipe->active && pipe->column > 1) { return; }
	current = pipe->scroll >> 2;
	// if not scrolled past a tile boundary, then we're done
	if ((current & 0x0f) > last) { return; }
	
	// note - the early exit for !spawning has a potential bug:
	// if the game ever disables spawning but does not also clear the
	// screen, then previous pipes will not be erased when they scroll
	// back around.
	
	// draw next tile column
	pipe->column--;
	
	// start waiting to draw the next pipe
	// *3 is so we can put a longer delay for the first pipe @ game start
	if (pipe->column > _pipespacing * 3)
	//column is unsigned & just loops to 255 so this is: "if column < 0" 
	{
		pipe->column=_pipespacing; // set gap before next pipe spawns
	}
	if (pipe->h2 != 255)
	{
		if (pipe->column < 5 && pipe->column >=2)
		{
			// update ceiling/floor for pipe gap
			pipe->ceiling	= (pipe->h2+1)*16 - 4; //-4 for bird pixel offset within sprite
			pipe->floor		= pipe->ceiling + 16 * pipe->gapsize - _birdheight;
		}
		else
		{
			// reset ceiling / floor
			pipe->ceiling	= _ceiling;
			pipe->floor		= _floor;
		}
	}
	if (pipe->column == 1)
	{
		pipe->height = (uint8_t)rand()%7 + 1;
		//pipe->height = 5; // for helping set the hitbox
		tile=TILE_pipel;
	}
	else if (pipe->column == 0)
	{
		pipe->h2 = pipe->h1;
		pipe->h1 = pipe->height;
		tile=TILE_piper;
	}
	else
	{
		tile=TILE_space;
		pipe->height=0xff;
	}

	// draw a column of pipe tiles
	col = ((current / 16) + 21) % 32;
	VERA.control = 0;
	VERA.address = VRAMlo(_mapbase) + 2*col;
	VERA.address_hi = VRAMhi(_mapbase) | VERA_INC_64;
	for ( row=0 ; row<14 ; row++ )
	{
		if (row==pipe->height)
		{
			VERA.data0=TILE_pipetopr+pipe->column;
			for (i=0 ; i < pipe->gapsize ; i++)
				VERA.data0=TILE_space;
			VERA.data0=TILE_pipebotr+pipe->column;
			row += 2 + pipe->gapsize;
		}
		VERA.data0=tile;
	}
}
