#ifndef __SOUND_H__
#define __SOUND_H__

#include <stdint.h>
#include "sysdefs.h"

#define YM_KeyDn	0x78 // 0x78
#define	YM_KeyUp	0x00

typedef struct sfxframe {
	uint8_t reg;
	uint8_t val;
	uint8_t delay;
} sfxframe;

typedef struct soundstate {
	uint8_t delay;
	sfxframe* data;
} soundstate;
  
struct YM2151_t {
    uint8_t reg;
    uint8_t dat;
};
#define YM (*(volatile struct YM2151_t*) YMBase)

#define WRITEYM(A,D)			\
	while (YM.dat & 0x80) {};	\
	YM.reg = (A);				\
	YM.dat = (D);

#define YMNOTE(VOICE, NOTE)            \
    WRITEYM(0x28 + (VOICE), (NOTE));   \
    WRITEYM(0x08, YM_KeyUp + (VOICE)); \
    while (YM.dat & 0x80) {}           \
    YM.dat = YM_KeyDn + (VOICE);

#define YMKEYUP(VOICE)	WRITEYM(0x08,(VOICE))

#define PLAYSFX(VOICESTATE,SFXDATA)		\
	(VOICESTATE)->delay = 0;			\
	(VOICESTATE)->data = (SFXDATA);

extern void writeYM(const uint8_t a, const uint8_t d);
extern void clear_YM();
extern void patchYM(uint8_t* patch, uint8_t voice);
extern uint8_t update_YM(soundstate* state, const uint8_t voice);

#endif
