#ifndef __INPUT_H__
#define __INPUT_H__

#include <stdint.h>

typedef struct controller_t {
  uint16_t
    last,		// previous state of the controller
    current,	// current state ( 1 = off, 0 = pressed )
    pressed,    // 1 = pressed on this frame, 0 = not pressed this frame
    released;	// 1 = released on this frame, 0 = not released this frame
} controller_t;

typedef struct joy_t {
	uint16_t state;
	uint8_t  detected;
} joy_t;

#define joy0	 (*(uint8_t*) JOYBASE-1)

#define IOFLAG_kb	0x0001
#define IOFLAG_m1	0x0002
#define IOFLAG_m2	0x0004

extern uint8_t joynum;
extern controller_t ctrlstate;
extern void check_input();


#endif

/* - from the Kernal source:
;---------------------------------------------------------------
; joystick_get
;
; Function:  Return the state of a given joystick.
;
; Pass:      a    number of joystick (0 or 1)
; Return:    a    byte 0:      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
;                         NES  | A | B |SEL|STA|UP |DN |LT |RT |
;                         SNES | B | Y |SEL|STA|UP |DN |LT |RT |
;
;            x    byte 1:      | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
;                         NES  | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
;                         SNES | A | X | L | R | 1 | 1 | 1 | 1 |
;            y    byte 2:
;                         $00 = joystick present
;                         $FF = joystick not present
;
; Notes:     * Presence can be detected by checking byte 2.
;            * The type of controller is encoded in bits 0-3 in
;              byte 1:
;              0000: NES
;              0001: keyboard (NES-like)
;              1111: SNES
;            * Bits 6 and 7 in byte 0 map to different buttons
;              on NES and SNES.
;--------------------------------------------------------------- */
