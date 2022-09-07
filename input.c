#include <joystick.h>
#include <conio.h>

#include "input.h"
#include "sysdefs.h"

controller_t ctrlstate;
uint8_t joynum = 1;

/*
void check_input()
{
	uint8_t bank = SETRAMBANK;
	joy_t *joystick = ((joy_t*)JOYBASE);
	
	//asm("jsr (%w)", GETJOY);
	
	ctrlstate.last = ctrlstate.current;
	SETRAMBANK = 0;
	if ((joystick+joynum)->detected == 0)
	{
		// mask the 4LSB of the high byte (joystick type bits)
		// flip the logic of the joystick bits so 1=pressed
		ctrlstate.current = (joystick+joynum)->state & 0xf0ff ^ 0xf0ff;
	}
	else {
		ctrlstate.current = 0;
	}
	if (kbhit()) {
		ctrlstate.key = cgetc();
		ctrlstate.current |= IOFLAG_kb;
	}
	else {
		ctrlstate.key = 0;
		ctrlstate.current &= ~ IOFLAG_kb;
	}
	// TODO: put the mouse buttons into the ctrlstate
	ctrlstate.pressed  = (ctrlstate.last ^ ctrlstate.current) & ctrlstate.current;
	ctrlstate.released = (ctrlstate.last ^ ctrlstate.current) & ctrlstate.last;
	SETRAMBANK = bank;
}
*/

void check_input()
{
	uint8_t bank = SETRAMBANK;
	joy_t *joystick = ((joy_t*)JOYBASE);
	
	//asm("jsr (%w)", GETJOY);
	if (kbhit()) {
		ctrlstate.key = cgetc();
	}
	else {
		ctrlstate.key=0;
	}
	if (ctrlstate.enabled) {
		ctrlstate.last = ctrlstate.current;
		SETRAMBANK = 0;
		if (joystick[joynum].detected == 0)
		{
			// mask the 4LSB of the high byte (joystick type bits)
			// flip the logic of the joystick bits so 1=pressed
			ctrlstate.current = ~joystick[joynum].state;
		}
		else {
			ctrlstate.current = 0;
		}
		if (ctrlstate.key) {
			ctrlstate.current |= IOFLAG_kb;
		}
		else {
			ctrlstate.current &= ~ IOFLAG_kb;
		}
		// TODO: put the mouse buttons into the ctrlstate
		ctrlstate.pressed  = (ctrlstate.last ^ ctrlstate.current) & ctrlstate.current;
		ctrlstate.released = (ctrlstate.last ^ ctrlstate.current) & ctrlstate.last;
		SETRAMBANK = bank;
	}
	else {
		ctrlstate.last 		= 0;
		ctrlstate.current	= 0;
		ctrlstate.pressed	= 0;
		ctrlstate.released	= 0;
	}
}
