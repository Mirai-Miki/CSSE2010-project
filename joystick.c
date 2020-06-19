/*
* joystick.c
*
*Author: Michael Bossner
*/

#include <stdio.h>
#include <avr/io.h>

#include "joystick.h"
#include "timer0.h"

////////////////////////////// Global variables ////////////////////////////////

// time before a move can repeat
#define REPEAT_MOVE 250
// Multiplier sensitivity for the joystick from rest position
// (REST_VALUE*MOVE_JOYSTICK) * or / as rest is in the middle.
#define MOVE_JOYSTICK 1.3
#define MOVE_JOYSTICK_DIAGONAL 1.1
// max movements allowed to be queued
#define MAX_QUEUE_SIZE 8

// these values should not be changed after init_joystick is called.
static uint16_t JOYSTICK_X_REST;
static uint16_t JOYSTICK_Y_REST;
// The time a joystick move was last queued
static uint32_t joystick_last_moved;
// Queue of joystick moves
static volatile uint8_t joystick_queue[MAX_QUEUE_SIZE];
static volatile uint8_t queue_length;

/////////////////// Function Prototypes for Helper Functions ///////////////////

static uint16_t get_joystick_x(void);
static uint16_t get_joystick_y(void);
static void joystick_move_helper(uint8_t move);

/////////////////////////////// Public Functions ///////////////////////////////

// Initialises the joystick hardware for use in the game
void init_joystick(void) {
	// Set up ADC - AVCC reference, right adjust
	ADMUX = (1<<REFS0);

	// Turn on the ADC (but don't start a conversion yet). Choose a clock
	// divider of 64. (The ADC clock must be somewhere
	// between 50kHz and 200kHz. We will divide our 8MHz clock by 64
	// to give us 125kHz.)
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1);

	JOYSTICK_X_REST = get_joystick_x();
	JOYSTICK_Y_REST = get_joystick_y();

	clear_joystick_queue();

	joystick_last_moved = get_current_time();
}

// If the joystick has been moved past a certain point and the rest time has
// past a move will be queued and the rest time will be reset.
void joystick_move(void) {
	if((get_current_time() >= (joystick_last_moved + REPEAT_MOVE)) &&
	(queue_length <= MAX_QUEUE_SIZE)) {

		if((get_joystick_y() >= JOYSTICK_Y_REST*MOVE_JOYSTICK_DIAGONAL) &&
		(get_joystick_x() >= JOYSTICK_X_REST*MOVE_JOYSTICK_DIAGONAL)){
				joystick_move_helper(MOVE_UP_LEFT);
			}
		else if((get_joystick_y() >= JOYSTICK_Y_REST*MOVE_JOYSTICK_DIAGONAL) &&
		(get_joystick_x() <= JOYSTICK_X_REST/MOVE_JOYSTICK_DIAGONAL)) {
			joystick_move_helper(MOVE_UP_RIGHT);
		}
		else if((get_joystick_y() <= JOYSTICK_Y_REST/MOVE_JOYSTICK_DIAGONAL) &&
		(get_joystick_x() >= JOYSTICK_X_REST*MOVE_JOYSTICK_DIAGONAL)) {
			joystick_move_helper(MOVE_DOWN_LEFT);
		}
		else if((get_joystick_y() <= JOYSTICK_Y_REST/MOVE_JOYSTICK_DIAGONAL) &&
		(get_joystick_x() <= JOYSTICK_X_REST/MOVE_JOYSTICK_DIAGONAL)) {
			joystick_move_helper(MOVE_DOWN_RIGHT);
		}
		else if(get_joystick_y() >= JOYSTICK_Y_REST*MOVE_JOYSTICK) {
			joystick_move_helper(MOVE_UP);
		}
		else if(get_joystick_x() >= JOYSTICK_X_REST*MOVE_JOYSTICK) {
			joystick_move_helper(MOVE_LEFT);
		}
		else if(get_joystick_x() <= JOYSTICK_X_REST/MOVE_JOYSTICK) {
			joystick_move_helper(MOVE_RIGHT);
		}
		else if(get_joystick_y() <= JOYSTICK_Y_REST/MOVE_JOYSTICK) {
			joystick_move_helper(MOVE_DOWN);
		}
	}
}

// Pulls a joystick move out of the queue and returns the value.
// If no move is in the queue return 0
uint8_t get_joystick_move(void) {
	uint8_t retur_value;
	if(queue_length > 0) {
		retur_value = joystick_queue[0];
		queue_length--;
		return retur_value;
	} else {
		return 0;
	}
}

// Clears the entire joystick queue
void clear_joystick_queue(void) {
	for(uint8_t i = 0; i < MAX_QUEUE_SIZE; i++) {
		joystick_queue[i] = 0;
		queue_length = 0;
	}
}

/////////////////////////////// Private (Helper) Functions /////////////////////

// Simple helper function
static void joystick_move_helper(uint8_t move) {
	joystick_queue[queue_length] = move;
	queue_length++;
	joystick_last_moved = get_current_time();
}

// Converts an input on the ADC and returns the value of the X axis
static uint16_t get_joystick_x(void) {
	ADMUX &= ~1;
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC)) {
		; /* Wait until conversion finished */
	}
	uint16_t value = ADC; // read the value
	return value;
}

// Converts an input on the ADC and returns the value of the Y axis
static uint16_t get_joystick_y(void) {
	ADMUX |= 1;
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC)) {
		; /* Wait until conversion finished */
	}
	uint16_t value = ADC; // read the value
	return value;
}
