/* Commander X16 architecture-dependent definitions
 * these are values I'm using instead of defines/functions/values
 * built into cc65 either for version compatability fluidity reasons
 * or even sheer ignorance that cc65 equivalents actually exist. ;)
 * 
*/

#ifndef __SYSDEFS_H__
#define __SYSDEFS_H__

//#define R39
//#define debug
#define profiling

// Game Parameters
#define _ceiling		-64
#define _floor			(14 * 16 - 19) // row 14, 16px per tile, 19 for bird hitbox

/*
#define _gravity 4 // 7
#define _birdflapthrust -64 // -77
*/
//#define _gravity 6 // 7
//#define _birdflapthrust -90 // -77

#define _birdstartx			72
#define _birdstarty			120
#define _birdanimspeed		4
#define _birdanimsteps		1
#define _birdanimframes		3
#define _birdrotatedelay	6
#define _birdheight			16

#define _pipespacing	8

#define _bannerx			(320/2 - (64*3/2))
#define _bannery			4
#define _reporty			(_bannery + 64)
#define _hiscoreoffsetx		(64*3-23)
#define _hiscoreoffsety		38
#define _hiscorespacing		46
#define _bannertitlespec	0xf2 // hhwwpppp
#define _bannerreportspec	0xf3
#define _banneroptionspec	0x72 // 64x16, palette 2
#define _banneropnumspec	0x50 // 16x16, palette 0

#define BANNER_1x3			0x13
#define BANNER_2x3			0x23
#define	BANNER_3x3			0x33
#define BANNER_1x2			0x12
#define BANNER_1x1			0x11

#define BANNER_TITLE		SPRadr(_bannerbase)
#define BANNER_GETREADY		SPRadr(_bannerbase + 3 * 64 * 64 / 2)
#define BANNER_GAMEOVER		SPRadr(_bannerbase + 6 * 64 * 64 / 2)
#define BANNER_REPORT		SPRadr(_reportbase)
#define BANNER_DIFFICULTY	SPRadr(_difficultbase)
#define BANNER_WIMPY		SPRadr(_difficultbase + (2 * 64 x 16 / 2 ))
#define BANNER_STANDARD		SPRadr(_difficultbase + (4 * 64 x 16 / 2 ))
#define BANNER_HARD			SPRadr(_difficultbase + (6 * 64 x 16 / 2 ))
#define BANNER_BRUTAL		SPRadr(_difficultbase + (8 * 64 x 16 / 2 ))
#define BANNER_DARKSOULS	SPRadr(_difficultbase + (10 * 64 x 16 / 2 ))

#define N_OPTIONS			6
#define BANNER_OPTION(I)	(SPRadr(_difficultbase + ( (I) * (64 * 16))))
#define BANNER_DIGIT(I)		(SPRadr(_digitbase))

// VRAM layout
#define _mapbase  	((unsigned long)0x04000)
#define _spritebase ((unsigned long)0x08000)
#define _tilebase 	((unsigned long)0x15000)
#define _bgbase   	((unsigned long)0x16000)

#define _birdbase		_spritebase
#define _bannerbase		((_spritebase) + 0x1800)
#define _reportbase		((_bannerbase) + 0x4800)
#define _difficultbase	((_reportbase) + 0x4800) // 0x12800
#define _digitbase		(_tilebase) // 0-f are the first 16 tiles

#define _tilebytes	0x80
#define _maxsprites 64
#define _fmvoices	8
#define _psgvoices	16


// pre-shifted versions of the resource locations in VRAM
// - bitshifted for use in writes to VERA bus-attached registers
//#define _mapbaseLo (_mapbase & 0xffff)
//#define _mapbaseHi (_mapbase >> 16 & 0x01)

#ifdef R39
#define YMBase			0x9f40	// Base address of YM2151 FM synth
#define JOYBASE			0xa031
#define SETRAMBANK		(*(uint8_t*) 0x00)
#define SETROMBANK		(*(uint8_t*) 0x01)
#else
#define YMBase			0x9fe0	// Base address of YM2151 FM synth
#define JOYBASE			0xa031  // only 2 of the joysticks are valid
#define SETRAMBANK		(*(uint8_t*) 0x9f61)
#define SETROMBANK		(*(uint8_t*) 0x9f60)
#endif

#define _SPRITEREGS		0x1fc00 // VRAM address of sprite control regs
#define _PSGREGS		0x1f9c0	// PSG Control register base in VRAM
#define _PALREGS		0x1fa00	// VRAM location of the color palette

// some system vectors
#define IRQVECTOR		0x0314	// RAM vector to the IRQ handler
#define SCANJOY			0xff53  // kernal routines for JS reading
#define GETJOY			0xFF56  // .. the Kernal already stores thes
								// in BANKRAM, so we can just go there
								// and read the values directly w/o
								// calling the kernal

// Macros to convert 17-bit VRAM addresses into 16-bit or 'bank #'
#define VRAMlo(a)	((a) & 0xffff)
#define VRAMhi(a)	((a) >> 16 & 0x01)

// Macros to convert VRAM addresses into Hi/Low Addr bytes in SPR regs.
#define SPRlo(a)	((a)>>5  & 0xff)
#define SPRhi(a)	((a)>>13 & 0x0f)
#define SPRadr(a)	(SPRlo(a) | SPRhi(a) << 8)

// Macro to calculate Sprite ADDR step from size in bytes
#define SPRaddrstep(BYTES)	(((BYTES)>>5)&0xff)

#endif

