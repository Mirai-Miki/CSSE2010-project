/*
* countdown.c
*
* Author: Michael Bossner
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "countdown.h"
#include "serialio.h"
#include "terminalio.h"
#include "game.h"
#include "audio.h"
#include "joystick.h"

////////////////////////////// Global variables ///////////////////////////////

// SSD Digigts 0 through to 9 represented as hex numbers
static uint16_t SSD_digits[10] = {
	0x3F,
	0x06,
	0x5B,
	0x4F,
	0x66,
	0x6D,
	0x7D,
	0x07,
	0x7F,
	0x6F,
};

// Countdown clock
static volatile uint16_t countdown;
// Pause Flag
static uint8_t pause;
// SSD_CC for changing between the 2 SSD displays
static volatile uint8_t ssd_cc;
// start time for the Countdown (10000 = 10sec)
#define TIME_LIMIT 2000

/////////////////// Function Prototypes for Helper Functions ///////////////////
static void update_countdown(void);

/////////////////////////////// Public Functions ///////////////////////////////

// initilises the countdown for use during the game
void init_countdown(void) {
	countdown = TIME_LIMIT;
	pause = FALSE;

	// clear timer
	TCNT2 = 0;
	// set 10ms ticks
	OCR2A = 77;
	// CTC mode
	TCCR2A = (1<<WGM21);
	TCCR2B = (1<<CS21) | (1<<CS20) | (1<<CS22);
	// sets interrupts for every 10ms
	TIMSK2 |= (1<<OCIE2A);
	// clears the interrupt flag
	TIFR2 &= (1<<OCF2A);

	// Set all of Port C as an output
	DDRC = 0xFF;
	DDRA |= (1<<DDRA7);
}

// resets the countdown
void reset_countdown(void) {
	countdown = TIME_LIMIT;
	TCNT1 = 0;
	pause = FALSE;
}

// Sets the pause flag.
// Resets the interrupt flag is unpaused.
void pause_countdown(uint8_t set) {
	pause = set;
	if(!pause) {
		TIFR1 = (1<<OCF1A);
	}
}

/////////////////////////////// Private (Helper) Functions /////////////////////

// Updates the SSD displays with the correct digits for the countdown
// Also sets the frog to dead if the countdown reaches 0
static void update_countdown(void) {
	if(countdown >= 1000) {
		if(ssd_cc) {
			PORTC = SSD_digits[(countdown/1000)%10];
			} else {
			PORTC = SSD_digits[(countdown/100)%10];
		}
	}
	else if(countdown >= 100) {
		if(ssd_cc) {
			PORTC = 0x00;
			} else {
			PORTC = SSD_digits[(countdown/100)%10];
		}
	}
	else {
		if(ssd_cc) {
			PORTC = 0xBF;
			} else {
			PORTC = SSD_digits[(countdown/10)%10];
		}
	}

	if(countdown <= 0 && get_frog_row() != 7) {
		set_frog_dead(TRUE);
	}
}

// Timer interupt set to go off every 10ms.
// Checks for a joystick move and updates the countdown.
// I used this timer for the joystick check as it fires less often then the main
// timer.
ISR(TIMER2_COMPA_vect) {
	if(!pause) {
		countdown--;
		joystick_move();
	}
	ssd_cc = 0x80 ^ ssd_cc;
	if(ssd_cc) {
		PORTA ^= 0b10000000;
	} else {
		PORTA &= 0b01111111;
	}
	update_countdown();
}