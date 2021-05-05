#include <joystick.h>
#include <conio.h>

#include "input.h"
#include "sysdefs.h"

controller_t ctrlstate;
uint8_t joynum = 0;

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
		ctrlstate.current = (joystick+joynum)->state & 0xf0ff ^ 0xffff;
	}
	if (kbhit()) {
		cgetc();
		ctrlstate.current |= IOFLAG_kb;
	}
	// TODO: put the mouse buttons into the ctrlstate
	ctrlstate.pressed  = (ctrlstate.last ^ ctrlstate.current) & ctrlstate.current;
	ctrlstate.released = (ctrlstate.last ^ ctrlstate.current) & ctrlstate.last;
	SETRAMBANK = bank;
}

/*
void foo()
{
	//_joy_stddrv:    .asciiz "cx16-std.joy"
	joy_load_driver(joy_static_stddrv);
	
}
*/
