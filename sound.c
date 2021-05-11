#include "sound.h"

void writeYM(const uint8_t a, const uint8_t d)
{
	while (YM.dat & 0x80) {};
	YM.reg = a;
	YM.dat = d;
}

void clear_YM(soundstate* voice)
{
	uint8_t i;
	for (i=0 ; i<255 ; i++)
	{
		WRITEYM(i,0);
	}
	WRITEYM(255,0);
	for (i=0 ; i < _fmvoices ; i++)
	{
		voice[i].delay = 0xff;
		voice[i].data  = (sfxframe*)0xffff;
	}	
}

void patchYM(uint8_t* patch, uint8_t voice)
{
	uint8_t i;
	
	WRITEYM(0x20+voice, *patch);
	++patch;
	WRITEYM(0x38+voice, *patch);
	++patch;
	for (i=0x40+voice ; i<0xf8 ; i+=8)
	{
		WRITEYM(i,*patch);
		++patch;
	}
	WRITEYM(i,*patch);	
}

extern uint8_t update_YM(soundstate* state, const uint8_t voice)
{
	sfxframe *data;
	uint8_t reg, val;
	
	while (! state->delay) {
		data = state->data;
		if (data->delay == 0xff)
		{
			state->delay = 0xff;
			return 0;
		}
		reg = data->reg;
		val = data->val;
		if (reg >= 0x20)
			reg += voice;
		if (reg == 0x08)
			val += voice;
		WRITEYM(reg, val);
		state->delay = data->delay;
		state->data++;
	}
	// todo: move this functionality into the IRQ handler
	--state->delay;
	return 1;
	
}

/*
extern playSFX (soundstate* voice, sfxdata* data,)
{
	voice->delay = 0;
	voice->data  = data;
}
*/
