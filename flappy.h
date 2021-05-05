#ifndef __FLAPPY_H__
#define __FLAPPY_H__

#include "pipes.h"
#include "bird.h"
#include "input.h"
#include "banner.h"
#include "scoreboard.h"

#define IRQvector	(*(uint16_t*)IRQVECTOR)

static uint8_t	spriteregs[_maxsprites][8]; // shadow registers
static uint8_t	frame;
static uint16_t SystemIRQ;
static pipe_t	pipe;

static const uint8_t	palette[96];

void load_vera(const unsigned long address, const uint8_t *data, uint16_t size);
void vload(const char* filename, const uint8_t bank, const uint16_t address);
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

#endif
