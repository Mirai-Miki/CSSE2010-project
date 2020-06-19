/*
* life.c
*
* Author: Michael Bossner
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "life.h"
#include "serialio.h"
#include "game.h"
#include "buttons.h"
#include "terminalio.h"
#include "countdown.h"
#include "audio.h"
#include "joystick.h"

#define F_CPU 8000000L
#include <util/delay.h>

////////////////////////////// Global variables ////////////////////////////////

// The maximum amount of lives the frog can acquire
#define MAX_LIVES 5
static uint8_t lives;

// Binary values that represent how many lives a frog has from 0 to 5 in that order.
static uint8_t led_lives[6] = {
	0b00000000,
	0b00000100,
	0b00001100,
	0b00011100,
	0b00111100,
	0b01111100
};

/////////////////// Function Prototypes for Helper Functions ///////////////////

static void life_v_updater(void);

/////////////////////////////// Public Functions ///////////////////////////////

// Sets the leds on PORTA to outputs and initilises lives to 3 then updates the
// terminal display.
void init_lives(void) {
	//Set output ports for life LEDs
	DDRA |= (1<<DDRA2) | (1<<DDRA3) | (1<<DDRA4) | (1<<DDRA5) | (1<<DDRA6);

	lives = 3;
	life_v_updater();
}

// Adds a life if the player does not have max lives
void add_life(void) {
	if(lives != MAX_LIVES) {
		lives++;
		life_v_updater();
	}
}

// returns the number of lives
uint8_t get_lives(void) {
	uint8_t return_value = lives;
	return return_value;
}

// Removes a life if the frog is dead and returns him to the start position.
// Also clears all input and updates the display.
void remove_life(void) {
	if(is_frog_dead()) {
		pause_countdown(1);
		redraw_frog();
		lives--;
		life_v_updater();
		if(get_lives() > 0) {
			play_audio(FROG_DIED);
			put_frog_in_start_position();
			clear_button_queue();
			clear_serial_input_buffer();
			clear_joystick_queue();
		}
	}
}

/////////////////////////////// Private (Helper) Functions /////////////////////

// Helper function for updating the terminal view and LEDs.
static void life_v_updater(void) {
	//Uses LEDs to show current lives
	uint8_t temp = PORTA & 0b10000011;
	PORTA = temp | led_lives[lives];
	move_cursor(15, 1);
	printf_P(PSTR("Lives: %u"), lives);
}