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
static uint8_t  frame;
static uint16_t SystemIRQ;
static pipe_t	pipe;

static struct colorstate {
	uint8_t	current;
	uint8_t end;
	uint8_t delay;
	uint8_t active;
} colorstate;

typedef struct param_t {
	uint8_t
		speed,
		gapsize;
	int8_t
		gravity,
		thrust;
	} param_t;

typedef struct pal_t {
	uint16_t	c[_numcolors];
} pal_t;


static const param_t params[5];
static const pal_t* palettes[RAMP_STEPS];

//static const uint8_t palette[128];

void init_game();
uint8_t titlescreen(bird_t* bird, uint8_t difficulty);
uint16_t playgame(bird_t* bird);
void gameover(bird_t* bird, uint16_t p_score, uint16_t p_hiscore, uint8_t p_difficulty);
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
static const sfxframe darksouls[];
static const int16_t optionx[N_OPTIONS];
static const int16_t optiony[N_OPTIONS];
static const int16_t optionaddr[N_OPTIONS];


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

void load(const char* filename, const uint8_t bank, const uint16_t address)
{
	uint8_t b = SETRAMBANK;
	SETRAMBANK = bank;
	cbm_k_setnam(filename);
	cbm_k_setlfs(0,8,0);
	cbm_k_load(0,address);
	SETRAMBANK = b;
}

void vload(const char* filename, const uint8_t bank, const uint16_t address)
{
	cbm_k_setnam(filename);
	cbm_k_setlfs(0,8,0);
	cbm_k_load(bank+2,address); // bank+2 is a special functionality of X16 LOAD
}

void init_game()
{
	uint8_t i;

	// these are the patches for the YM SFX
	#include "dingsound.inc" // will switch this to a load instead...
	#include "fallsound.inc"
	#include "flapsound.inc"
	#include "smacksound.inc"
	#include "darksoulsound.inc"

	// disable sprites and layers while we load graphics / init screen
	VERA.display.video &= 0x07;

	//undo mixed-case mode that cc65 sets by default
 	cbm_k_bsout(CH_FONT_UPPER);

	vload("background.bin",VRAMhi(_bgbase),VRAMlo(_bgbase));
	vload("tiles.bin",VRAMhi(_tilebase),VRAMlo(_tilebase));
	vload("bird.bin",VRAMhi(_birdbase),VRAMlo(_birdbase));
	vload("banners.bin",VRAMhi(_bannerbase),VRAMlo(_bannerbase));
	vload("report.bin",VRAMhi(_reportbase),VRAMlo(_reportbase));
	vload("difficulty.bin",VRAMhi(_difficultbase),VRAMlo(_difficultbase));
	load("ramps.bin",1,0xa000);
	for (i=0 ; i < RAMP_STEPS * 2 ; i++)
	{
		palettes[i] = (pal_t*)(_rampbase + sizeof(pal_t) * i);
	}
	SETRAMBANK = 1;
	load_vera(_PALREGS,(uint8_t*)palettes[PAL_BLACK],sizeof(pal_t));
	colorstate.delay = 0;
	colorstate.current = PAL_BLACK;
	colorstate.end = PAL_NORMALB;
	colorstate.active = 0;

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
	VERA.display.video		|= 0x70;

	clear_YM(&fmvoice);
	patchYM((uint8_t*)&dingsound,0);
	patchYM((uint8_t*)&smacksound,7);
	patchYM((uint8_t*)&fallsound,2);
	patchYM((uint8_t*)&flapsound,3);
	patchYM((uint8_t*)&darksoulsound,4);

	// initialize the RNG
	srand(VIA1.t1_lo);

	// install the IRQ handler
	IRQvector = (uint16_t)&irq;

	// clear the keyboard buffer
	while (kbhit()) { cgetc(); }
	joynum = 1;
	ctrlstate.enabled = 1;
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
	if (colorstate.active)
	{
		if (colorstate.delay == 0)
		{
			load_vera(_PALREGS,(uint8_t*)palettes[colorstate.current],sizeof(pal_t));
			if (colorstate.current == colorstate.end)
			{
				colorstate.active = 0;
			}
			else
			{
				if (colorstate.end < colorstate.current)
					--colorstate.current;
				else
					++colorstate.current;
				colorstate.delay = RAMP_SPEED;
			}
		}
		else
		{
			--colorstate.delay;
		}
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

uint8_t titlescreen(bird_t* bird, uint8_t difficulty)
{
	#define pogospeed 2
	#define maxy 16
	#define miny -3

	#define BIRDXOFFSET 190
	#define BIRDYOFFSET 21

	banner_t banner;
	banner_t options[N_OPTIONS];
	banner_t opnums[N_OPTIONS];

	uint8_t i,f;

	#ifdef debug
	scoreboard_t db[4];
	uint8_t	d;
	joy_t *joystick = ((joy_t*)JOYBASE);
	#endif

	uint8_t *spriteptr; // current sprite pointer

	int16_t target[2] = { miny, maxy };
	uint8_t	t = 0;
	uint8_t delay	= pogospeed;
	uint8_t noinput = 16; // ignore input for this many frames
	uint8_t darksoulcounter = 0;

	// keep previous difficulty, or initialize to 1 the first time
	if (difficulty >= 5)
		difficulty = 2;
	spriteptr = &spriteregs[0][0];
	clear_sprites();
	init_pipes(&pipe);
	init_bird(bird, _bannerx+BIRDXOFFSET, _bannery+BIRDYOFFSET);
	init_banner(&banner,BANNER_TITLE,_bannertitlespec,BANNER_1x3);
	banner.x = _bannerx;
	banner.y = _bannery;
	banner.speed = 1;
	banner.target = target[t];

	for (i = 0 ; i < N_OPTIONS ; i++) {
		init_banner(&options[i], BANNER_OPTION(i), _banneroptionspec, BANNER_1x2);
		options[i].x = optionx[i];
		options[i].y = optiony[i];
		options[i].target = optiony[i];
		options[i].speed = 0;
		init_banner(&opnums[i], optionaddr[i], _banneropnumspec, BANNER_1x1);
		opnums[i].x = options[i].x - 20;
		opnums[i].y = options[i].y;
		opnums[i].target = options[i].target;
		opnums[i].speed = 0;
	}
	// we don't use option 0's number banner so hide it offscreen
	opnums[0].y = 300;
	opnums[0].target = 300;
	opnums[0].speed = 0;
	// hide the Dark Souls option offscreen if not selected
	if (difficulty != 4)
	{
		options[5].target = 240;
		options[5].y = 240;
	}
	#ifdef debug
	init_scoreboard(&db[0],0,10,SB_left,SB_hex);
	init_scoreboard(&db[1],0,30,SB_left,SB_hex);
	init_scoreboard(&db[2],0,50,SB_left,SB_hex);
	init_scoreboard(&db[3],0,70,SB_left,SB_hex);
	#endif
	ctrlstate.pressed	= 0;
	ctrlstate.current	= 0;
	ctrlstate.last		= 0;
	ctrlstate.released	= 0;
	for (i=0 ; i<16 ; i++)
	{
		endframe(&spriteptr);
	}
	colorstate.current	= PAL_BLACK;
	colorstate.end		= PAL_NORMALB;
	colorstate.active	= 1;
	while (!ctrlstate.pressed)
	{
		f++;
#ifdef debug
		db[0].score = ctrlstate.current;
		db[1].score = options[0].x;
		db[2].score = options[0].y;
		db[3].score = options[0].target;
		for (d=0 ; d<4 ; d++)
		{
			spriteptr = update_scoreboard(&db[d], spriteptr);
		}
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
		for (i = 0 ; i < N_OPTIONS ; i++)
		{
			spriteptr = update_banner(&options[i], spriteptr);
			if ((i-1 != difficulty)||(f & 16))
			{
				if ((i != 5)||(difficulty == 4))
					spriteptr = update_banner(&opnums[i], spriteptr);
			}
		}
		endframe(&spriteptr);
		switch (ctrlstate.key)
		{
		case '1':
		{
			if (difficulty != 0)
			{
				difficulty = 0;
				options[5].target = 240;
				++noinput;
				PLAYSFX(&fmvoice[0],(sfxframe*)&ding);
			}
			break;
		}
		case '2':
		{
			if (difficulty != 1)
			{
				difficulty = 1;
				options[5].target = 240;
				++noinput;
				PLAYSFX(&fmvoice[0],(sfxframe*)&ding);
			}
			break;
		}
		case '3':
		{
			if (difficulty != 2)
			{
				difficulty = 2;
				options[5].target = 240;
				++noinput;
				PLAYSFX(&fmvoice[0],(sfxframe*)&ding);
			}
			break;
		}
		case '4':
		{
			if (difficulty != 3)
			{
				difficulty = 3;
				options[5].target = 240;
				++noinput;
				PLAYSFX(&fmvoice[0],(sfxframe*)&ding);
			}
			break;
		}
		case '9':
		{
			if (difficulty != 4)
			{
				difficulty = 4;
				options[5].target = optiony[5];
				++noinput;
				PLAYSFX(&fmvoice[4],(sfxframe*)&darksouls);
			}
			break;
		}
		} // end of switch(ctrlstate.key)
		if (ctrlstate.current & 0x20)  // if select is pressed
		{
			++darksoulcounter;
			if (ctrlstate.pressed * 0x20) {
				difficulty++;
				if (difficulty > 3)
					difficulty = 0;
				++noinput;
				options[5].target = 240;
				PLAYSFX(&fmvoice[0],(sfxframe*)&ding);
			}
			if (darksoulcounter >= 180)
			{
				difficulty = 4;
				options[5].target = optiony[5];
				++noinput;
				PLAYSFX(&fmvoice[4],(sfxframe*)&darksouls);
				darksoulcounter = 0;
			}
		}
		else
		{
			darksoulcounter = 0;
		}
		pipe.speed = params[difficulty].speed;
		if (options[5].target != options[5].y)
			options[5].speed = 4;
		if (noinput) {
			noinput--;
			// TODO - make this a functionality of check_input()
			ctrlstate.pressed = 0;
			ctrlstate.released = 0;
		}
	}
	// set game parameters based on chosen difficulty
	pipe.speed			= params[difficulty].speed;
	pipe.gapsize		= params[difficulty].gapsize;
	bird->basegravity	= params[difficulty].gravity;
	bird->thrust		= params[difficulty].thrust;
	return(difficulty);
}

uint8_t* debug_joy(uint8_t* spriteptr)
{
	joy_t *joystick = ((joy_t*)JOYBASE);
	scoreboard_t dbg_sb = { 0, 32, 48, 0, 1 };
	unsigned char bank = RAM_BANK;
	RAM_BANK = 0;

	dbg_sb.score = joystick[1].state;
	spriteptr = update_scoreboard(&dbg_sb, spriteptr);

	dbg_sb.y += 16;
	dbg_sb.score = joystick[1].detected;
	spriteptr = update_scoreboard(&dbg_sb, spriteptr);

	dbg_sb.y += 16;
	dbg_sb.score = ctrlstate.current;
	spriteptr = update_scoreboard(&dbg_sb, spriteptr);

	RAM_BANK = bank;
	return spriteptr;
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
	int16_t crashy;

#ifdef debug
	scoreboard_t dbout[3];
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
	crashy = _floor;

	while (1) {
//		if (kbhit()) {cgetc(); ctrlstate.pressed=1; }
		// update game state
		update_pipes(&pipe);
#ifdef debug
		dbout[0].score = ctrlstate.current;
		dbout[1].score = ctrlstate.last;
		dbout[2].score = ctrlstate.pressed;
		for (d=0 ; d<3 ; d++)
		{
			spriteptr = update_scoreboard(&dbout[d], spriteptr);
		}
#endif
		// spriteptr = debug_joy(spriteptr);
		if (banner.y != banner.target)
		{
			// if "Get Ready" banner visible....
			if (!banner.speed)
			{
				// player has not flapped yet - keep waiting
				if (ctrlstate.pressed)
				{
					// start the game
					banner.speed = 2;
					bird->gravity = bird->basegravity;
					pipe.active = 1;
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
			crashy = bird->y;
			pipe.speed	= 0;
			pipe.active = 0;
			bird->gravity = 3;
			// shake pipes and flash a red flash
			// TODO: shake_pipes()
			// TODO: flash_screen()
			colorstate.current	= PAL_RED;
			colorstate.end		= PAL_NORMALR;
			colorstate.active	= 1;
			init_banner(&banner,BANNER_GAMEOVER,_bannertitlespec,BANNER_1x3);
			banner.x = _bannerx;
			banner.y = -64;
			banner.target = 4;
			banner.speed = 3;
			// skip fall sound if bird near ground
			if (bird->y < _floor - 4)
				PLAYSFX(&fmvoice[2],(sfxframe*)&fall);
			PLAYSFX(&fmvoice[7],(sfxframe*)&smack);
			while (bird->y < _floor) {
				spriteptr = update_banner(&banner, spriteptr);
				spriteptr = update_bird(bird, spriteptr);
				endframe(&spriteptr);
			}
			// don't play smack sound twice if bird hit pipe
			// near the ground
			if (crashy < _maxcrashy)
				PLAYSFX(&fmvoice[7],(sfxframe*)&smack);
			bird->vy = 0;
			bird->gravity = 0;
			if (bird->y > _floor)
				bird->y = _floor;
			// wait for the banner to arrive at the target Y location
			do {
				spriteptr = update_banner(&banner, spriteptr);
				spriteptr = update_bird(bird, spriteptr);
				endframe(&spriteptr);
			} while (banner.speed);
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
		endframe(&spriteptr);
	}
}

void gameover(bird_t *bird, uint16_t p_score, uint16_t p_hiscore, uint8_t p_difficulty)
{
	scoreboard_t	score;
	scoreboard_t	hiscore;
	banner_t		banner;
	banner_t		report;
	banner_t		modelabel;
	banner_t		sparkle;
	uint8_t*		spriteptr;

	int16_t			scorediff = p_score;
	int16_t			y = 240;
	int8_t			vy = -6;
	uint8_t			f = 0;
	uint8_t			done = 0;
	uint8_t			pikapika = 0;

	set_medalcolor(0, p_hiscore); // hide the medal until the score is tallied
	init_scoreboard(&score, _bannerx + _hiscoreoffsetx, _hiscoreoffsety, SB_right, SB_decimal);
	init_scoreboard(&hiscore, _bannerx + _hiscoreoffsetx, _hiscoreoffsety + _hiscorespacing, SB_right, SB_decimal);
	score.score   = 0;
	hiscore.score = p_hiscore;

	init_banner(&banner,BANNER_GAMEOVER,_bannertitlespec,BANNER_1x3);
	banner.x = _bannerx;
	banner.y = _bannery;
	banner.target = banner.y;
	banner.speed = 0;

	init_banner(&report,BANNER_REPORT,_bannerreportspec,BANNER_2x3);
	report.x = _bannerx;
	report.target = _reporty;
	report.speed  = 0;	// due to sprite priority issues, it is not
						// beneficial to use the auto-move feature
						// of the banner type for this object

	init_banner(&modelabel, BANNER_OPTION(p_difficulty+1), _banneroptionspec, BANNER_1x2);
	modelabel.x = _bannerx + _modeoffsetx;
	modelabel.target=_reporty + _modeoffsety;
	modelabel.speed = 0;

	init_banner(&sparkle, BANNER_SPARKLE(0), _bannersparklespec, BANNER_1x1);
	sparkle.x = -16;
	sparkle.y = -16;

	spriteptr = (uint8_t*)spriteregs;
	ctrlstate.released = 0;
	ctrlstate.pressed  = 0;
	ctrlstate.last     = 0xffff;
	ctrlstate.current	= 0;

	while (! done)
	{
		if (report.y != _reporty)
		{
			// report card is moving onto the screen...
			report.y += vy;
			if (report.y < _reporty)
				report.y = _reporty;
			score.y = report.y + _hiscoreoffsety;
			hiscore.y = score.y + _hiscorespacing;
			modelabel.y = report.y + _modeoffsety;
		}
		else if (scorediff)
		{
			if (! (frame & 3))
			{
				// scoreboard is counting up to the player score
				if (scorediff > 20)
				{
					score.score += 4;
					scorediff -= 4;
				}
				else if (scorediff > 10)
				{
					score.score += 2;
					scorediff -= 2;
				}
				else
				{
					++score.score;
					--scorediff;
				}
				// if the score just finished counting,
				// display the medal / NEW flags in the scorecard
				if (!scorediff)
				{
					hiscore.score = set_medalcolor(p_score, p_hiscore);
					if (p_score >= 10)
						pikapika = 1;
				}
			}
		}
		else
		{
			done = (ctrlstate.released != 0);
		}
		if (pikapika) {
			f = frame >> 3; // animate the sparkle every 8th frame
			f &= 3;			// 4 animation frames
			if (!f) {
				//randomize the location of the sparkle for each cycle
				sparkle.x = (uint8_t)rand()%32 + 96;
				sparkle.y = (uint8_t)rand()%32 + 107;
				// first frame is blank, so don't bother rendering

			}
			if (f == 3)
				sparkle.addr = BANNER_SPARKLE(1);
			else
				sparkle.addr = BANNER_SPARKLE(f);
			spriteptr = update_banner(&sparkle, spriteptr);
		}
		spriteptr = update_scoreboard(&score, spriteptr);
		spriteptr = update_scoreboard(&hiscore, spriteptr);
		spriteptr = update_banner(&banner, spriteptr);
		spriteptr = update_banner(&modelabel, spriteptr);
		spriteptr = update_banner(&report, spriteptr);
		spriteptr = update_bird(bird, spriteptr);
		endframe(&spriteptr);
	}

	// TODO: fade to black - yaaay!
	colorstate.current	= PAL_NORMALB;
	colorstate.end		= PAL_BLACK;
	colorstate.active	= 1;
	for(f = 0 ; f < 16 ; f++)
	{
		spriteptr = update_scoreboard(&score, spriteptr);
		spriteptr = update_scoreboard(&hiscore, spriteptr);
		spriteptr = update_banner(&banner, spriteptr);
		spriteptr = update_banner(&modelabel, spriteptr);
		spriteptr = update_banner(&report, spriteptr);
		spriteptr = update_bird(bird, spriteptr);
		endframe(&spriteptr);
	}
	// TODO: whoosh sound

}

void endframe(uint8_t **spriteptr)
{
	uint8_t* s = *spriteptr;


	// flag the current sprite as first unused
	s[6] = 0xfe;
	frameready=1;
	VERA.display.border = 13;
	update_sound(); // todo: make sound use the frameready mechanic
					//       with the IRQ doing the --delay function
	VERA.display.border = 0;
	while (frameready) {}
	VERA.display.border = 15;

	// reset sprite pointer to first sprite
	*spriteptr = &spriteregs[0][0];
	check_input();
	++frame;
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
/*
static const uint8_t palette[128] =
{
  // tile palette
  0xbc,0x06, 0x00,0x00, 0xff,0x0f, 0xf0,0x00, 0xc0,0x00, 0x80,0x00, 0xd4,0x0f, 0xe5,0x0e,
  0x81,0x0f, 0x45,0x0b, 0x78,0x0f, 0xfb,0x0f, 0xb4,0x0d, 0xf0,0x0f, 0xed,0x0f, 0x33,0x0f,

  // background palette
  0xff,0x00, 0xfd,0x0e, 0xc7,0x07, 0xb7,0x06, 0xd7,0x07, 0xdc,0x09, 0xdb,0x0b, 0xdc,0x0b,
  0xdc,0x0c, 0xed,0x0e, 0xec,0x0d, 0x00,0x00, 0xb4,0x0d, 0x4f,0x0e, 0xff,0x0f, 0x33,0x0f,

  // banner palette
  0x00,0x00, 0xbc,0x06, 0xed,0x0b, 0x11,0x01, 0xa6,0x0b, 0xd8,0x0d, 0xfa,0x0f, 0xc3,0x07,
  0x83,0x05, 0xf8,0x0b, 0x91,0x0d, 0xa0,0x0f, 0xda,0x0e, 0x0e,0x0f, 0xff,0x0f, 0x14,0x0e,

  // report card palette (gets manipulated)
  0x00,0x00, 0x22,0x03, 0xa6,0x0b, 0xc9,0x0d, 0xd9,0x0e, 0xda,0x0d, 0xa4,0x0d, 0x91,0x02,
  0x90,0x0c, 0xb0,0x0f, 0xfa,0x0f, 0x60,0x09, 0x30,0x05, 0x0e,0x0f, 0xff,0x0f, 0x14,0x0e
};
*/

static uint8_t spriteregs[_maxsprites][8] = { };
static pipe_t pipe = { 0,0,0,0,0 };
static uint16_t SystemIRQ = 0;
static uint8_t frame = 0;

/*
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
	{0x08, YM_KeyUp, 0}, { 0x0f, 0x81, 0 },
	{0x28, 0x24, 0}, {0x08, YM_KeyDn, 3},
	{0x08, YM_KeyUp, 0}, {0,0,0xff}
};

static const sfxframe darksouls[] = {
	{0x08, YM_KeyUp, 0}, {0x28, 0x2d, 0}, {0x08, YM_KeyDn, 6},
	{0x08, YM_KeyUp, 0}, {0x28, 0x2e, 0}, {0x08, YM_KeyDn, 6},
	{0x08, YM_KeyUp, 0}, {0x28, 0x2f, 0}, {0x08, YM_KeyDn, 6},
	{0x08, YM_KeyUp, 0}, {0x28, 0x2e, 0}, {0x08, YM_KeyDn, 30},
	{0x08, YM_KeyUp, 0}, {0, 0, 0xff}
};
*/

// SFX sequence for the ding sound
static const sfxframe ding[] = {
	{0x28, 0x79, 0}, {0x08, YM_KeyUp, 0}, {0x08, YM_KeyDn, 1},
	{0x08, YM_KeyUp, 7}, {0x28, 0x7e, 0}, {0x08, YM_KeyDn, 1},
	{0x08, YM_KeyUp, 0x00}, {0,0,0xff}
};

static const sfxframe fall[] = {
	{0x28, 0x5e, 0}, {0x08, YM_KeyUp, 0}, {0x08, YM_KeyDn, 4},
	{0x30, 0x20, 0}, {0x28, 0x5d, 3}, {0x30, 0x00, 2}, {0x30, 0x20, 0},
	{0x28, 0x5c, 2}, {0x30, 0x00, 2}, {0x30, 0x20, 0}, {0x28, 0x5a, 2},
	{0x30, 0x00, 2}, {0x30, 0x20, 0}, {0x28, 0x59, 2}, {0x30, 0x00, 2},
	{0x08, YM_KeyUp, 0},
	{0x30, 0x20, 0}, {0x28, 0x58, 2}, {0x30, 0x00, 2}, {0x30, 0x20, 0},
	{0x28, 0x56, 2}, {0x30, 0x00, 2}, {0x30, 0x20, 0}, {0x28, 0x55, 2},
	{0x30, 0x00, 2}, {0x28, 0x54, 2}, {0x28, 0x52, 1}, {0x28, 0x51, 1},
	{0x28, 0x50, 1},
	{0,0,0xff}
};

static const sfxframe flap[] = {
	{0x08, YM_KeyUp, 0}, {0x28, 0x24, 0}, {0x08, YM_KeyDn, 4},
	{0x08, YM_KeyUp, 0}, {0,0,0xff}
};

static const sfxframe smack[] = {
	{0x08, YM_KeyUp, 0}, { 0x0f, 0x81, 0 },
	{0x28, 0x26, 0}, {0x08, YM_KeyDn, 3},
	{0x08, YM_KeyUp, 0}, {0,0,0xff}
};
/*
static const sfxframe smack[] = {
	{0x08, YM_KeyUp, 0}, {0x28, 0x34, 0}, {0x08, YM_KeyDn, 1},
	{0x28, 0x24, 1},{0x28, 0x34, 1},{0x28, 0x24, 1},
	{0x28, 0x34, 1},{0x28, 0x24, 1},
	{0x08, YM_KeyUp, 0}, {0,0,0xff}
};
*/

static const sfxframe darksouls[] = {
	{0x08, YM_KeyUp, 0}, {0x28, 0x30, 0}, {0x08, YM_KeyDn, 6},
	{0x08, YM_KeyUp, 0}, {0x28, 0x31, 0}, {0x08, YM_KeyDn, 6},
	{0x08, YM_KeyUp, 0}, {0x28, 0x32, 0}, {0x08, YM_KeyDn, 6},
	{0x08, YM_KeyUp, 0}, {0x28, 0x31, 0}, {0x08, YM_KeyDn, 30},
	{0x08, YM_KeyUp, 0}, {0, 0, 0xff}
};


// game difficulty parameters
static const param_t params[5] = {

		{ 8, 4, 4, -60 },	// wimpy
		{ 8, 4, 6, -90 },	// standard
		{ 8, 3, 5, -64 },	// hard
		{ 9, 3, 7, -77 },	// brutal
		{ 9, 2, 4, -55 }	// dark souls
};

// x coordinates for the menu option items
static const int16_t optionx[N_OPTIONS] = {
	96, 130, 130, 130, 130, 130
};

// y coordinates for the menu option items
static const int16_t optiony[N_OPTIONS] = {
	100, 120, 140, 160, 180, 200
};

// VRAM locations for the menu option number sprites
// (needed because we use banners instead of scoreboards)
static const int16_t optionaddr[N_OPTIONS] = {
	SPRadr(_tilebase),			// 0
	SPRadr(_tilebase + 1 * 16 * 16 /2),	// 1
	SPRadr(_tilebase + 2 * 16 * 16 /2),	// 2
	SPRadr(_tilebase + 3 * 16 * 16 /2),	// 3
	SPRadr(_tilebase + 4 * 16 * 16 /2),	// 4
	SPRadr(_tilebase + 9 * 16 * 16 /2)	// 9
};

void main()
{

	bird_t bird;
	uint16_t score;

	static uint8_t difficulty = 1;
	static uint16_t hiscore[5] = {0,0,0,0,0};

	SystemIRQ = IRQvector;
	init_game();
	clear_screen();

	while (1) {

		difficulty = titlescreen(&bird, difficulty);
		score = playgame(&bird);
		gameover(&bird, score, hiscore[difficulty], difficulty);
		// wait to actually update the hiscore so that the
		// gameover() routine can detect a new hi score
		if (score >= hiscore[difficulty])
			hiscore[difficulty] = score;
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
