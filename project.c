/*
 * FroggerProject.c
 *
 * Main file
 *
 * Author: Peter Sutton. Modified by Michael Bossner
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "ledmatrix.h"
#include "scrolling_char_display.h"
#include "buttons.h"
#include "serialio.h"
#include "terminalio.h"
#include "score.h"
#include "timer0.h"
#include "game.h"
#include "level.h"
#include "life.h"
#include "countdown.h"
#include "audio.h"
#include "joystick.h"
#include "highscore.h"

#define F_CPU 8000000L
#include <util/delay.h>

// ASCII code for Escape character
#define ESCAPE_CHAR 27

static uint32_t current_time, lmt_lane_0, lmt_lane_1, lmt_lane_2, lmt_channel_0,
lmt_channel_1, button_repeat_time;

static char serial_input, escape_sequence_char;
static uint8_t characters_into_escape_sequence = 0;
int8_t button = NO_BUTTON_PUSHED;
int8_t joystick = NO_BUTTON_PUSHED;
#define BUTTON_REPEAT 250

// Function prototypes - these are defined below (after main()) in the order
// given here
void initialise_hardware(void);
void splash_screen(void);
void new_game(void);
void play_game(void);
void handle_game_over(void);

/////////////////////////////// Private (Helper) Functions /////////////////////
static void move_lanes(void);
static void process_serial_in(void);
static void process_input(void);
static void process_diagonal_move(void);

/////////////////////////////// main //////////////////////////////////
int main(void) {
	// Setup hardware and call backs. This will turn on
	// interrupts.
	initialise_hardware();

	// Show the splash screen message. Returns when display
	// is complete
	splash_screen();

	while(1) {
		new_game();
		play_game();
		handle_game_over();
	}
}

void initialise_hardware(void) {
	ledmatrix_setup();
	init_button_interrupts();
	// Setup serial port for 19200 baud communication with no echo
	// of incoming characters
	init_serial_stdio(19200,0);
	init_timer0();
	init_audio();
	init_highscore();
	init_joystick();

	// Turn on global interrupts
	sei();
}

void splash_screen(void) {
	// Clear terminal screen and output a message
	clear_terminal();
	set_display_attribute(FG_GREEN);
	draw_highscore_screen();
	move_cursor(37,2);
	printf_P(PSTR("Frogger"));
	move_cursor(16,3);
	printf_P(PSTR("CSSE2010/7201 project by Michael Bossner S4427719"));

	// Output the scrolling message to the LED matrix
	// and wait for a push button to be pushed.
	ledmatrix_clear();
	while(1) {
		set_scrolling_display_text("FROGGER   S4427719", COLOUR_GREEN);
		// Scroll the message until it has scrolled off the
		// display or a button is pushed
		while(scroll_display()) {
			_delay_ms(150);
			if(button_pushed() != NO_BUTTON_PUSHED) {
				return;
			}
		}
	}
}

void new_game(void) {
	// Clear the serial terminal
	clear_terminal();
	hide_cursor();

	// Initialise the game and display
	initialise_game();

	// Initialise the score
	init_score();
	init_lives();
	init_level();
	init_countdown();

	// Clear all button pushes or serial inputs if any are waiting
	clear_button_queue();
	clear_serial_input_buffer();
	clear_joystick_queue();
}

void play_game(void) {
	// Get the current time and remember this as the last time the vehicles
	// and logs were moved.
	current_time = get_current_time();
	lmt_lane_0 = current_time; // lmt = last move time
	lmt_lane_1 = current_time;
	lmt_lane_2 = current_time;
	lmt_channel_0 = current_time;
	lmt_channel_1 = current_time;
	button_repeat_time = current_time;

	// We play the game while the frog is alive
	while(get_lives() > 0) {
		if(!is_frog_dead() && frog_has_reached_riverbank()) {
			// Frog reached the other side successfully but the
			// riverbank isn't full, put a new frog at the start
			if(is_riverbank_full()) {
				play_audio(FROG_LEVELUP);
				level_updater();
				initialise_game();
				add_life();
			} else {
				play_audio(FROG_MADE_IT);
				put_frog_in_start_position();
			}
		}
		// Check for input - which could be a button push or serial input.
		// Serial input may be part of an escape sequence, e.g. ESC [ D
		// is a left cursor key press. At most one of the following three
		// variables will be set to a value other than -1 if input is available.
		// (We don't initalise button to -1 since button_pushed() will return -1
		// if no button pushes are waiting to be returned.)
		// Button pushes take priority over serial input. If there are both then
		// we'll retrieve the serial input the next time through this loop
		serial_input = -1;
		escape_sequence_char = -1;

		joystick = get_joystick_move();
		if(!joystick) {
			button = button_pushed();
			if(button != NO_BUTTON_PUSHED) {
				button_repeat_time = current_time;
			}

			if(is_button_held() != NO_BUTTON_PUSHED &&
					current_time >= button_repeat_time + BUTTON_REPEAT) {
						button = is_button_held();
						button_repeat_time = current_time;
			}

			if(button == NO_BUTTON_PUSHED) {
				process_serial_in();
			}
		}

		else if(joystick >= JOYSTICK_DIAGONAL_MOVE) {
			process_diagonal_move();
		}
		
		process_input();
		move_lanes();
		remove_life();
		play_audio(NO_TRACK);
	}
	// We get here if the frog is out of lives or the riverbank is full
	// The game is over.
}

void handle_game_over() {
	pause_countdown(TRUE);
	clear_terminal();

	// unused if statement as the player cannot win with infinite levels
	if(!is_frog_dead()) {
		move_cursor(37,2);
		printf_P(PSTR("WINNER!!"));
		play_audio(WINNER);
	} else {
		move_cursor(37,2);
		printf_P(PSTR("GAME OVER"));
		play_audio(GAME_OVER);
	}
	draw_gameover_screen();
	
	clear_button_queue();
	move_cursor(26,3);
	printf_P(PSTR("Press a button to start again"));
	while(button_pushed() == NO_BUTTON_PUSHED) {
		; // wait
	}
}

/////////////////////////////// Private (Helper) Functions /////////////////////

static void move_lanes(void) {
	current_time = get_current_time();
	if(!is_frog_dead() && current_time >= lmt_lane_0 +
	get_row_speed(FIRST_VEHICLE_ROW_SPEED)) {
		scroll_vehicle_lane(0, 1);
		lmt_lane_0 = current_time;
	}
	if(!is_frog_dead() && current_time >= lmt_lane_1 +
	get_row_speed(SECOND_VEHICLE_ROW_SPEED)) {
		scroll_vehicle_lane(1, -1);
		lmt_lane_1 = current_time;
	}
	if(!is_frog_dead() && current_time >= lmt_lane_2 +
	get_row_speed(THIRD_VEHICLE_ROW_SPEED)) {
		scroll_vehicle_lane(2, 1);
		lmt_lane_2 = current_time;
	}
	if(!is_frog_dead() && current_time >= lmt_channel_0 +
	get_row_speed(FIRST_RIVER_ROW_SPEED)) {
		scroll_river_channel(0, -1);
		lmt_channel_0 = current_time;
	}
	if(!is_frog_dead() && current_time >= lmt_channel_1 +
	get_row_speed(SECOND_RIVER_ROW_SPEED)) {
		scroll_river_channel(1, 1);
		lmt_channel_1 = current_time;
	}
}

static void process_serial_in(void) {
	if(serial_input_available()) {
		// Serial data was available - read the data from standard input
		serial_input = fgetc(stdin);
		// Check if the character is part of an escape sequence
		if(characters_into_escape_sequence == 0 && serial_input == ESCAPE_CHAR) {
			// We've hit the first character in an escape sequence (escape)
			characters_into_escape_sequence++;
			serial_input = -1; // Don't further process this character
			}
		else if(characters_into_escape_sequence == 1 && serial_input == '[') {
			// We've hit the second character in an escape sequence
			characters_into_escape_sequence++;
			serial_input = -1; // Don't further process this character
			}
		else if(characters_into_escape_sequence == 2) {
			// Third (and last) character in the escape sequence
			escape_sequence_char = serial_input;
			serial_input = -1;  // Don't further process this character - we
			// deal with it as part of the escape sequence
			characters_into_escape_sequence = 0;
			}
		else {
			// Character was not part of an escape sequence (or we received
			// an invalid second character in the sequence). We'll process
			// the data in the serial_input variable.
			characters_into_escape_sequence = 0;
			}
	}
}

static void process_input(void) {
	// Process the input.
	if(button==3 || escape_sequence_char=='D' || serial_input=='A' ||
	serial_input=='a' || joystick==MOVE_LEFT) {
		// Attempt to move left
		play_audio(FROG_JUMP);
		move_frog_to_left();
	} else if(button==2 || escape_sequence_char=='A' || serial_input=='W' ||
	serial_input=='w' || joystick==MOVE_UP) {
		// Attempt to move forward
		play_audio(FROG_JUMP);
		move_frog_forward();
	} else if(button==1 || escape_sequence_char=='B' || serial_input=='S' ||
	serial_input=='s' || joystick==MOVE_DOWN) {
		// Attempt to move down
		play_audio(FROG_JUMP);
		move_frog_backward();
	} else if(button==0 || escape_sequence_char=='C' || serial_input=='D' ||
	serial_input=='d' || joystick==MOVE_RIGHT) {
		// Attempt to move right
		play_audio(FROG_JUMP);
		move_frog_to_right();
	} else if(serial_input == 'p' || serial_input == 'P') {
		pause_timer(1);
		pause_countdown(1);
		uint8_t temp = DDRD;
		DDRD &= DDRD4_OFF;

		serial_input = -1;
		while(1) {
			process_serial_in();
			if(serial_input == 'p' || serial_input == 'P') {
				break;
			}
		}
		pause_timer(0);
		pause_countdown(0);
		DDRD = temp;
		clear_button_queue();
		clear_serial_input_buffer();
		clear_joystick_queue();
	}
	// else - invalid input or we're part way through an escape sequence -
	// do nothing
}

static void process_diagonal_move(void) {
	if(joystick == MOVE_UP_LEFT) {
		play_audio(FROG_JUMP);
		move_frog_up_left();
	}
	else if(joystick == MOVE_UP_RIGHT) {
		play_audio(FROG_JUMP);
		move_frog_up_right();
	}
	else if(joystick == MOVE_DOWN_LEFT) {
		play_audio(FROG_JUMP);
		move_frog_down_left();
	}
	else if(joystick == MOVE_DOWN_RIGHT) {
		play_audio(FROG_JUMP);
		move_frog_down_right();
	}
}