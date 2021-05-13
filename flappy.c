#include <stdio.h>
#include <conio.h>
#include <stdint.h>
#include <stdlib.h>
#include <cbm.h>

#include "sysdefs.h"
#include "flappy.h"
#include "sound.h"

// ============================ flappy.h begin

#include "pipes.h"
#include "bird.h"
#include "input.h"
#include "banner.h"
#include "scoreboard.h"

#define IRQvector	(*(uint16_t*)IRQVECTOR)

static uint8_t	spriteregs[_maxsprites][8]; // shadow registers
static uint8_t	frameready;
static uint16_t SystemIRQ;
static pipe_t	pipe;

static const uint8_t	palette[128];

void init_game();
void titlescreen();
uint16_t playgame(bird_t* bird);
void gameover(bird_t* bird, uint16_t p_score, uint16_t p_hiscore);
void update_screen();
void clear_screen();
void clear_sprites();
void update_pipes();
void endframe(uint8_t **spriteptr);
static void irq(void);


// ============================ flappy.h end

static soundstate psgvoice[_psgvoices];
static soundstate fmvoice[_fmvoices];
static const sfxframe ding[];
static const sfxframe fall[];
static const sfxframe flap[];
static const sfxframe smack[];

void load_vera(const unsigned long address, const uint8_t *data, uint16_t size)
{

  uint16_t i;

  VERA.control = 0;
  VERA.address = address & 0xffff;
  VERA.address_hi = ((address & 0x10000) >> 16) | VERA_INC_1;

  for ( i=0 ; i < size ; i++  )
  {
    VERA.data0 = data[i];
  }
}

void vload(const char* filename, const uint8_t bank, const uint16_t address)
{
	cbm_k_setnam(filename);
	cbm_k_setlfs(0,8,0);
	cbm_k_load(bank+2,address); // bank+2 is a special functionality of X16 LOAD
}

void init_game()
{
	// these are the patches for the YM SFX
	#include "dingsound.inc" // will switch this to a load instead...
	#include "fallsound.inc"
	#include "flapsound.inc"
	
	// disable sprites and layers while we load graphics / init screen
	VERA.display.video = 0x01; 

	//undo mixed-case mode that cc65 sets by default
 	cbm_k_bsout(CH_FONT_UPPER);

	vload("background.bin",VRAMhi(_bgbase),VRAMlo(_bgbase));
	vload("tiles.bin",VRAMhi(_tilebase),VRAMlo(_tilebase));
	vload("bird.bin",VRAMhi(_birdbase),VRAMlo(_birdbase));
	vload("banners.bin",VRAMhi(_bannerbase),VRAMlo(_bannerbase));
	vload("report.bin",VRAMhi(_reportbase),VRAMlo(_reportbase));
	load_vera(_PALREGS,palette,sizeof(palette));

	// set display to 320x240
	VERA.display.hscale		= 64;
	VERA.display.vscale		= 64;
	
	#ifdef profiling
	// leave 2 pixels of border on the left side of the screen for profiling by changing border color
	VERA.control |= 2;
	VERA.display.hstart = 2;
	VERA.control &= 255-2;
	#endif
	
	VERA.layer0.config		= 0x06; // MapH&W=0,Bitmap=1,depth=2
	VERA.layer0.hscroll		= 0x01<<8;    // sets palette offset in bitmap mode
	VERA.layer0.tilebase	= ((_bgbase >> 9) & 0xfc) | 0x00;

	VERA.layer1.config		= 0x02; // mapH&W=0 (32tiles), 4bpp tile mode
	VERA.layer1.mapbase		= _mapbase >> 9;
	VERA.layer1.tilebase	= ((_tilebase >> 9) & 0xfc) | 0x03; //16x16 tiles
	VERA.layer1.hscroll		= 0;
	VERA.layer1.vscroll		= 0;


	clear_sprites();
	// enable sprites and both layers for display
	VERA.display.video		= 0x71;

	clear_YM(&fmvoice);
	patchYM((uint8_t*)&dingsound,0);
	patchYM((uint8_t*)&dingsound,1);
	patchYM((uint8_t*)&fallsound,2);
	patchYM((uint8_t*)&flapsound,3);
	
	// initialize the RNG
	srand(VIA1.t1_lo);
	
	// install the IRQ handler
	IRQvector = (uint16_t)&irq;
	
	// clear the keyboard buffer
	while (kbhit()) { cgetc(); }
	joynum = 0;
}

void clear_sprites()
{
	uint8_t r;
	asm ("SEI");
	VERA.control	= 0;
	VERA.address 	= VRAMlo(_SPRITEREGS);
	VERA.address_hi	= VRAMhi(_SPRITEREGS) | VERA_INC_1;
	for (r=0 ; r < _maxsprites ; r += 1)
	{
		spriteregs[r][0] = 0;
		spriteregs[r][1] = 0;
		spriteregs[r][2] = 0;
		spriteregs[r][3] = 0;
		spriteregs[r][4] = 0;
		spriteregs[r][5] = 0;
		spriteregs[r][6] = 0xff; // signals "unused sprite"
		spriteregs[r][7] = 0;
		VERA.data0 = 0;
		VERA.data0 = 0;
		VERA.data0 = 0;
		VERA.data0 = 0;
		VERA.data0 = 0;
		VERA.data0 = 0;
		VERA.data0 = 0;
		VERA.data0 = 0;
	}
	asm ("CLI");
}

void clear_screen()
{
	int i;

	// point VERA data registers at the tilemap
	VERA.control	= 0;
	VERA.address	= VRAMlo(_mapbase);
	VERA.address_hi = VRAMhi(_mapbase) | VERA_INC_1;
	// clear all tiles in the layer 1 map
	for ( i=0 ; i<32*32 ; i++)
	{
		VERA.data0 = TILE_space; // blank tile
		VERA.data0 = 0x00;
	}
	// draw the ground
	VERA.address	= VRAMlo(_mapbase) + (14 * 32 * 2); // row 14, col 0
	VERA.address_hi = VRAMhi(_mapbase) | VERA_INC_2; // set step=2
	for (i=0 ; i<32 ; i++)
	{
	  VERA.data0 = TILE_ground;
	}
	for (i=0 ; i<32 ; i++)
	{
	  VERA.data0 = TILE_underground;
	}
}

void update_screen(const int16_t scroll)
{
	uint8_t  VH , VC;
	uint16_t VL;
	uint8_t r = 0;
	
	VC = VERA.control;
	VH = VERA.address_hi;
	VL = VERA.address;
	
	VERA.layer1.hscroll = scroll;

	VERA.control	&= 0xfe;
	VERA.address	= VRAMlo(_SPRITEREGS);
	VERA.address_hi = VRAMhi(_SPRITEREGS) | VERA_INC_1;
	while (spriteregs[r][6] < 0xfe && r < _maxsprites)
	{
		VERA.data0 = spriteregs[r][0];
		VERA.data0 = spriteregs[r][1];
		VERA.data0 = spriteregs[r][2];
		VERA.data0 = spriteregs[r][3];
		VERA.data0 = spriteregs[r][4];
		VERA.data0 = spriteregs[r][5];
		VERA.data0 = spriteregs[r][6];
		VERA.data0 = spriteregs[r][7];
		r += 1;
	}
	while (spriteregs[r][6] != 0xff && r < _maxsprites)
	{
		VERA.data0 = 0;
		VERA.data0 = 0;
		VERA.data0 = 0;
		VERA.data0 = 0;
		VERA.data0 = 0;
		VERA.data0 = 0;
		VERA.data0 = 0;
		VERA.data0 = 0;
		spriteregs[r][0] = 0;
		spriteregs[r][1] = 0;
		spriteregs[r][2] = 0;
		spriteregs[r][3] = 0;
		spriteregs[r][4] = 0;
		spriteregs[r][5] = 0;
		spriteregs[r][6] = 0xff;
		spriteregs[r][7] = 0;
		r += 1;
	}
	VERA.address=VL;
	VERA.address_hi = VH;
	VERA.control = VC;
}

void update_sound()
{
	uint8_t i;
	
	for (i=0 ; i < _fmvoices ; i++)
	{
		if (fmvoice[i].delay != 0xff)
			update_YM(&fmvoice[i],i);
	}
}

void titlescreen(bird_t* bird)
{
	#define pogospeed 2
	#define maxy 16
	#define miny -3
	
	#define BIRDXOFFSET 190
	#define BIRDYOFFSET 21

	banner_t banner;

	uint8_t *spriteptr; // current sprite pointer
	#ifdef debug
	scoreboard_t sa;
	scoreboard_t sb;
	#endif
	
	int16_t target[2] = { miny, maxy };
	uint8_t	t = 0;
    uint8_t delay	= pogospeed;
    uint8_t noinput = 16; // ignore input for this many frames

	spriteptr = &spriteregs[0][0];
	clear_sprites();
	init_pipes(&pipe);
	init_bird(bird, _bannerx+BIRDXOFFSET, _bannery+BIRDYOFFSET);
	init_banner(&banner,BANNER_TITLE,_bannertitlespec,BANNER_1x3);
	printf("%s 0x%04x\n","n tiles     = ", banner.ntiles);
	printf("%s 0x%04x\n","&banner     = ", &banner);
	printf("%s 0x%04x\n","spriteptr   = ", spriteptr);
	printf("%s 0x%04x\n","&spriteregs = ", &spriteregs);
	banner.x = _bannerx;
	banner.y = _bannery;
	banner.speed = 1;
	banner.target = target[t];
	
	#ifdef debug
	init_scoreboard(&sb,0,100,SB_left,SB_hex);
// 	init_scoreboard(&sb,0,120,SB_left,SB_hex);

//	sb.x		= 0;
//	sb.y		= 120;
//	sb.hex		= SB_hex;
//	sb.center	= SB_left;
//	sb.score	= 0;

	#endif
	ctrlstate.pressed	= 0;
	ctrlstate.current	= 0;
	ctrlstate.last		= 0xffff;
	ctrlstate.released	= 0;
	
	while (!ctrlstate.pressed)
	{
		#ifdef debug
//		sa.score = banner.addr;
//		sb.score = banner.step;
//		spriteptr = update_scoreboard(&sa,spriteptr);
//		spriteptr = update_scoreboard(&sb,spriteptr);
		#endif
		if (delay) {
			delay--;
			banner.speed = 0;
		}
		else
		{
			delay = pogospeed;
			banner.speed = 1;
			if (banner.y == banner.target) {
				t = ++t & 0x1;
				banner.target = target[t];
			}
		}
		update_pipes(&pipe);
		spriteptr = update_banner(&banner,spriteptr);
		bird->y = banner.y + BIRDYOFFSET;
		spriteptr = update_bird(bird,spriteptr);
		endframe(&spriteptr);
		if (noinput) {
			noinput--;
			// TODO - make this a functionality of check_input()
			ctrlstate.pressed = 0;
			ctrlstate.released = 0;
		}
	}
	
}

uint16_t playgame(bird_t* bird)
{
	//#define profiling
	//#define debug
	//#define testbird

	banner_t banner;
	scoreboard_t score;
	uint8_t *spriteptr;
	uint16_t lastfloor;

#ifdef debug
	scoreboard_t dbout[5];
	uint8_t d;
#endif
#ifdef testbird
	uint8_t i = 1;
	uint8_t autopilot = 0;
#endif

	// clear all sprites shadow regs
	clear_sprites();

	spriteptr = &spriteregs[0][0];
	init_scoreboard(&score,320/2,4,SB_center,SB_decimal);
#ifdef debug
	init_scoreboard(&dbout[0],0,10,SB_left,SB_hex); 
	init_scoreboard(&dbout[1],0,30,SB_left,SB_hex); 
	init_scoreboard(&dbout[2],0,50,SB_left,SB_hex); 
	init_scoreboard(&dbout[3],0,70,SB_left,SB_hex); 
	init_scoreboard(&dbout[4],0,90,SB_left,SB_hex); 
#endif
	init_bird(bird, _birdstartx, _birdstarty);
	init_banner(&banner,BANNER_GETREADY,_bannertitlespec,BANNER_1x3);
	banner.x = _bannerx;
	banner.y = _bannery;
	banner.target = -64;
	banner.speed  = 0;
	lastfloor = _floor;
	while (kbhit()) { cgetc(); }
	endframe(&spriteptr);
	while (1) {
		if (kbhit()) { cgetc(); ctrlstate.pressed=1; }
		// update game state
		update_pipes(&pipe);
#ifdef debug
		dbout[0].score = ctrlstate.current;
		dbout[1].score = ctrlstate.last;
		dbout[2].score = ctrlstate.pressed;
		dbout[3].score = ctrlstate.released;
		dbout[4].score = banner.speed;
		for (d=0 ; d<5 ; d++)
		{
			spriteptr = update_scoreboard(&dbout[d], spriteptr);
		}
#endif
		if (banner.y != banner.target) {
			// if "Get Ready" banner visible....
			if (!banner.speed) {
				// player has not flapped yet - keep waiting
				if (ctrlstate.pressed)
				{
					// start the game
					banner.speed = 2;
					bird->gravity = _gravity;
					pipe.active = 1;
					score.score = 0;
					#ifdef testbird
					autopilot = 1;
					#endif
				}
			}
			spriteptr = update_banner(&banner,spriteptr);
			if (banner.y == banner.target)
				banner.speed = 0;
		}
		else {
			if (pipe.floor  > lastfloor)
			{
				score.score++;
				PLAYSFX((soundstate*)&fmvoice[0],(sfxframe*)&ding);
			}
			spriteptr = update_scoreboard(&score,spriteptr);
		}
		if (! check_bird(bird,pipe.ceiling,pipe.floor))
		{
			pipe.speed	= 0;
			pipe.active = 0;
			bird->gravity = _gravity - 2;
			// shake pipes and flash a red flash
			// TODO: shake_pipes()
			// TODO: flash_screen()
			init_banner(&banner,BANNER_GAMEOVER,_bannertitlespec,BANNER_1x3);
			banner.x = _bannerx;
			banner.y = -64;
			banner.target = 4;
			banner.speed = 3;
			// skip fall sound if bird near ground
			if (bird->y < _floor - 4)
			{
				//PLAYSFX(&fmvoice[1],(sfxframe*)&smack);
				PLAYSFX(&fmvoice[2],(sfxframe*)&fall);
			}
			while (bird->y < _floor) {
				spriteptr = update_banner(&banner, spriteptr);
				spriteptr = update_bird(bird, spriteptr);
				endframe(&spriteptr);
			}
			//PLAYSFX(&fmvoice[1],(sfxframe*)&smack);
			bird->vy = 0;
			bird->gravity = 0;
			if (bird->y > _floor)
				bird->y = _floor;
			do {
				spriteptr = update_banner(&banner, spriteptr);
				spriteptr = update_bird(bird, spriteptr);
				#ifdef debug
		dbout[0].score = ctrlstate.current;
		dbout[1].score = ctrlstate.last;
		dbout[2].score = ctrlstate.pressed;
		dbout[3].score = ctrlstate.released;
		dbout[4].score = banner.speed;
		for (d=0 ; d<5 ; d++)
		{
			spriteptr = update_scoreboard(&dbout[d], spriteptr);
		}
#endif
				endframe(&spriteptr);
			} while (!ctrlstate.released || banner.speed);
			return score.score;
		}
	// dev/debug code - dont forget to pull this out
		if (ctrlstate.current) {
			if (ctrlstate.current & 4 || ctrlstate.pressed & 0x0080)
				bird->y++;
			if (ctrlstate.current & 8 || ctrlstate.pressed & 0x4000)
				bird->y--;
		}		
		
		#ifdef testbird
		if ((!--i) && autopilot)
		{
			score.score+=1;
//			flap_bird(bird);
			i=21;
		}
		#else
		if (ctrlstate.pressed) {
			flap_bird(bird);
			PLAYSFX((soundstate*)&fmvoice[3],(sfxframe*)&flap);
		}
		#endif
		spriteptr = update_bird(bird,spriteptr);
		lastfloor = pipe.floor;
		#ifdef profiling
		VERA.display.border = 14;
		endframe();
		VERA.display.border = 1;
		#else
		endframe(&spriteptr);
		#endif
	}
}

void gameover(bird_t *bird, uint16_t p_score, uint16_t p_hiscore)
{
	scoreboard_t	score;
	scoreboard_t	hiscore;
	banner_t		banner;
	banner_t		report;
	uint8_t*		spriteptr;
	
	uint8_t			f = 60;
	
	set_medalcolor(0, p_hiscore); // hide the medal until the score is tallied
	set_medalcolor(p_score, p_hiscore);
	init_scoreboard(&score, _bannerx + _hiscoreoffsetx, _hiscoreoffsety, SB_right, SB_decimal);
	init_scoreboard(&hiscore, _bannerx + _hiscoreoffsetx, _hiscoreoffsety + _hiscorespacing, SB_right, SB_decimal);
	score.score   = p_score;
	hiscore.score = p_hiscore;
	
	init_banner(&banner,BANNER_GAMEOVER,_bannertitlespec,BANNER_1x3);
	banner.x = _bannerx;
	banner.y = _bannery;
	banner.target = banner.y;
	banner.speed = 0;

	init_banner(&report,BANNER_REPORT,_bannerreportspec,BANNER_2x3);
	report.x = _bannerx;
	report.y = _reporty;
	report.target = _reporty;
	report.speed  = 0;	// due to sprite priority issues, it is not
						// beneficial to use the auto-move feature
						// of the banner type for this object
	
	spriteptr = (uint8_t*)spriteregs;
	ctrlstate.released = 0;
	ctrlstate.pressed  = 0;
	ctrlstate.last     = 0xffff;
	ctrlstate.current	= 0;
	
	while ((! ctrlstate.released) || f > 0 )
//	while (1)
	{
		if (f > 0)
			--f;
		spriteptr = update_scoreboard(&score, spriteptr);
		spriteptr = update_scoreboard(&hiscore, spriteptr);
		spriteptr = update_banner(&banner, spriteptr);
		spriteptr = update_banner(&report, spriteptr);
		spriteptr = update_bird(bird, spriteptr);
		endframe(&spriteptr);
	}
	
	// TODO: fade to black
	// TODO: whoosh sound
	
}

void endframe(uint8_t **spriteptr)
{
	uint8_t* s = *spriteptr;

	// flag the current sprite as first unused
	s[6] = 0xfe;
	frameready=1;
	update_sound(); // todo: make sound use the frameready mechanic
					//       with the IRQ doing the --delay function
	while (frameready) {}

	// reset sprite pointer to first sprite
	*spriteptr = &spriteregs[0][0];
	check_input();
}

void irq(void)
{
	if (frameready)
	{
		update_screen(pipe.scroll >> 2);
		// signal beginning of next frame to main program
		frameready=0;
	}
	SETROMBANK = 0;	
	asm("jmp (_SystemIRQ)");
}

// TODO: move this data into a .BIN and vload() it directly to VERA
static const uint8_t palette[128] =
{
  // background palette
  0xbc,0x06, 0x00,0x00, 0xff,0x0f, 0xf0,0x00, 0xc0,0x00, 0x80,0x00, 0xd4,0x0f, 0xe5,0x0e,
  0x81,0x0f, 0x45,0x0b, 0x78,0x0f, 0xfb,0x0f, 0xb4,0x0d, 0x4f,0x0e, 0xed,0x0f, 0x33,0x0f,

  // tile palette
  0xff,0x00, 0xfd,0x0e, 0xc7,0x07, 0xb7,0x06, 0xd7,0x07, 0xdc,0x09, 0xdb,0x0b, 0xdc,0x0b,
  0xdc,0x0c, 0xed,0x0e, 0xec,0x0d, 0x00,0x00, 0xb4,0x0d, 0x4f,0x0e, 0xff,0x0f, 0x33,0x0f,
  
  // banner palette
  0x00,0x00, 0xbc,0x06, 0xed,0x0b, 0x11,0x01, 0xa6,0x0b, 0xd8,0x0d, 0xfa,0x0f, 0xc3,0x07,
  0x83,0x05, 0xf8,0x0b, 0x91,0x0d, 0xa0,0x0f, 0xda,0x0e, 0x0e,0x0f, 0xff,0x0f, 0x14,0x0e,
  
  // report card palette (gets manipulated)
  0x00,0x00, 0x22,0x03, 0xa6,0x0b, 0xc9,0x0d, 0xd9,0x0e, 0xda,0x0d, 0xa4,0x0d, 0x91,0x02, 
  0x90,0x0c, 0xb0,0x0f, 0xfa,0x0f, 0x60,0x09, 0x30,0x05, 0x0e,0x0f, 0xff,0x0f, 0x14,0x0e
};


static uint8_t spriteregs[_maxsprites][8] = { };
static pipe_t pipe = { 0,0,0,0,0 };
static uint16_t SystemIRQ = 0;
static uint8_t frame = 0;

// SFX sequence for the ding sound
static const sfxframe ding[] = {
	{0x28, 0x76, 0}, {0x08, YM_KeyUp, 0}, {0x08, YM_KeyDn, 1},
	{0x08, YM_KeyUp, 7}, {0x28, 0x7c, 0}, {0x08, YM_KeyDn, 1},
	{0x08, YM_KeyUp, 0x00}, {0,0,0xff}
};

static const sfxframe fall[] = {
	{0x28, 0x5c, 0}, {0x08, YM_KeyUp, 0}, {0x08, YM_KeyDn, 4},
	{0x30, 0x20, 0}, {0x28, 0x5a, 3}, {0x30, 0x00, 2}, {0x30, 0x20, 0},
	{0x28, 0x59, 2}, {0x30, 0x00, 2}, {0x30, 0x20, 0}, {0x28, 0x58, 2},
	{0x30, 0x00, 2}, {0x30, 0x20, 0}, {0x28, 0x56, 2}, {0x30, 0x00, 2},
	{0x08, YM_KeyUp, 0},
	{0x30, 0x20, 0}, {0x28, 0x55, 2}, {0x30, 0x00, 2}, {0x30, 0x20, 0},
	{0x28, 0x54, 2}, {0x30, 0x00, 2}, {0x30, 0x20, 0}, {0x28, 0x52, 2},
	{0x30, 0x00, 2}, {0x28, 0x51, 2}, {0x28, 0x50, 1}, {0x28, 0x4e, 1},
	{0x28, 0x4c, 1},
	{0,0,0xff}
};

static const sfxframe flap[] = {
	{0x08, YM_KeyUp, 0}, {0x28, 0x21, 0}, {0x08, YM_KeyDn, 4},
	{0x08, YM_KeyUp, 0}, {0,0,0xff}
};

static const sfxframe smack[] = {
	{0x08, YM_KeyUp, 0}, {0,0,0xff}
};

void main()
{

	bird_t bird;

	static uint16_t score = 0;
	static uint16_t hiscore = 0;

	SystemIRQ = IRQvector;
	init_game();
	clear_screen();
/*
	YMNOTE(0x76,0);
	endframe();
	YMKEYUP(0);
	endframe();
	endframe();
	endframe();
	endframe();
	endframe();
	endframe();
	endframe();
	endframe();
	YMNOTE(0x7c,0);
	endframe();
	YMKEYUP(0);
*/

	while (1) {
		titlescreen(&bird);
		score = playgame(&bird);
		gameover(&bird, score, hiscore);
		// wait to actually update the hiscore so that the
		// gameover() routine can detect a new hi score
		if (score >= hiscore)
			hiscore = score;
	}
	// exit from the game (don't think I'm going to implement this)

	// restore the screen to some state - probably need to just make this a function
	// as the below code certainly doesn't leave things in a useable state for BASIC anyway...
	//vpoke (0x0f,0x2000,1);
	//vpoke (0x0f,0x3000,0);
	//vpoke (0x1f,0x0001,128);
	//VERA.data0 = 128;

	// or....
	// call coldstart vector at $FFFC
}



