#include <conio.h>
#include <stdio.h>
#include <stdint.h>
#include <cbm.h>
#include "sysdefs.h"
#include "input.h"

//controller_t ctrlstate;
joy_t *j = ((joy_t*)JOYBASE);

#if(0)
void test_joy()
{
	joynum = 0;
	SETRAMBANK = 0;
	while (1)
	{
		check_input();
		cbm_k_chrout(CH_HOME);
		//cprintf("%04x %02x\n\r",j->state ^ 0xffff, j->detected);
		cprintf("%04x %04x %04x %04x",ctrlstate.last, ctrlstate.current, ctrlstate.pressed,&SETRAMBANK);
		waitvsync();
	}
}
#endif

void ta()
{
	uint8_t i;
	const char foo[] = "Hello World";
	char *bar = foo;
	
	printf("%c\n",*bar++);
	printf("%c\n",*bar++);
	printf("%c\n",*bar++);
	printf("%c\n",*bar++);
	printf("%c\n",*bar++);
	printf("%c\n",*bar++);
	printf("%c\n",*bar++);
	printf("%c\n",*bar++);
	printf("%c\n",*bar++);
	printf("%c\n",*bar++);
	printf("%c\n",*bar++);
	while (kbhit()) { cgetc(); }
	while (! kbhit()) {}
}

void main()
{
	ta();
	
}
