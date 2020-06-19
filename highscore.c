/*
* highscore.c
*
* Author: Michael Bossner
*/

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <ctype.h>

#include "highscore.h"
#include "terminalio.h"
#include "audio.h"
#include "score.h"
#include "level.h"
#include "game.h"
#include "serialio.h"

////////////////////////////// Global variables ////////////////////////////////

// X/Y coordinates for the start and end of the highscore area
#define START_POS_X 23
#define END_POS_X 57
#define START_POS_Y 5
#define END_POS_Y 23
// X Axis placement of the Columns
#define RANK START_POS_X + 3
#define SCORE START_POS_X + 13
#define NAME START_POS_X + 22
// Y Axis placement of the Rows
#define CELL START_POS_Y + 4
#define RANK1 START_POS_Y + 6
#define RANK2 START_POS_Y + 8
#define RANK3 START_POS_Y + 10
#define RANK4 START_POS_Y + 12
#define RANK5 START_POS_Y + 14
#define INPUT_NAME_X END_POS_X + 5
#define INPUT_NAME_Y START_POS_Y + 7

// Left cursor ascii value
#define LEFT_CURSOR 68
// Escape Character ascii value
#define ESCAPE_CHAR 27
// Enter key ascii value
#define ENTER 10
// SpaceBar key ascii value
#define SPACE 32
// No input has been provided
#define NO_INPUT -1

// Signature for checking if the data is ours
#define SIGNITURE 44277196
static uint32_t EEMEM signiture;

// Max characters allowed in a high score name
#define MAX_NAME_SIZE 11
// What is placed if there is no name available
#define NO_NAME "NO NAME"
// What is placed if there is no score available
#define NO_SCORE 0

// Memory is assigned for all high score data
static uint8_t EEMEM rank1_name[MAX_NAME_SIZE];
static uint8_t EEMEM rank2_name[MAX_NAME_SIZE];
static uint8_t EEMEM rank3_name[MAX_NAME_SIZE];
static uint8_t EEMEM rank4_name[MAX_NAME_SIZE];
static uint8_t EEMEM rank5_name[MAX_NAME_SIZE];
static uint32_t EEMEM rank1_score;
static uint32_t EEMEM rank2_score;
static uint32_t EEMEM rank3_score;
static uint32_t EEMEM rank4_score;
static uint32_t EEMEM rank5_score;

// input variables
static char serial_input, escape_sequence_char;
static uint8_t characters_into_escape_sequence = NO_INPUT;
// The name that is input for a highscore winner
static uint8_t name_input[MAX_NAME_SIZE];
// Game Over flag
static uint8_t gameover_flag;

/////////////////// Function Prototypes for Helper Functions ///////////////////

static void terminal_draw_border(void);
static void input_highscore(void);
static void process_serial_in(void);

/////////////////////////////// Public Functions ///////////////////////////////

// Initialises the hardware for high score use. If the hardware has not already
// been initialised before then space will be allocated and readied for use in the
// future.
void init_highscore(void) {
	uint32_t temp = eeprom_read_dword(&signiture);
	if(temp != SIGNITURE) {
		// EEPROM not initalised for highscores
		// sets the signiture for the EEPROM memory
		eeprom_update_dword(&signiture, (uint32_t)SIGNITURE);
		// set all names to "NO NAME"
		eeprom_update_block((const void*)NO_NAME, (void*)rank1_name, MAX_NAME_SIZE);
		eeprom_update_block((const void*)NO_NAME, (void*)rank2_name, MAX_NAME_SIZE);
		eeprom_update_block((const void*)NO_NAME, (void*)rank3_name, MAX_NAME_SIZE);
		eeprom_update_block((const void*)NO_NAME, (void*)rank4_name, MAX_NAME_SIZE);
		eeprom_update_block((const void*)NO_NAME, (void*)rank5_name, MAX_NAME_SIZE);
		// set all highscores to 0
		eeprom_update_dword(&rank1_score, (uint32_t)NO_SCORE);
		eeprom_update_dword(&rank2_score, (uint32_t)NO_SCORE);
		eeprom_update_dword(&rank3_score, (uint32_t)NO_SCORE);
		eeprom_update_dword(&rank4_score, (uint32_t)NO_SCORE);
		eeprom_update_dword(&rank5_score, (uint32_t)NO_SCORE);
	}
}

// Draws the highscore screen to the terminal and collects all highscores from
// memory and displayes it. If the game_over flag has been set and a highscore
// achieved input will be requested by the user for the new highscore.
void draw_highscore_screen(void) {
	terminal_draw_border();

	// Temporary variables to hold the highscore data from memory
	uint8_t temp_rank1_name[MAX_NAME_SIZE];
	uint8_t temp_rank2_name[MAX_NAME_SIZE];
	uint8_t temp_rank3_name[MAX_NAME_SIZE];
	uint8_t temp_rank4_name[MAX_NAME_SIZE];
	uint8_t temp_rank5_name[MAX_NAME_SIZE];
	eeprom_read_block((void*)temp_rank1_name,(const void*)rank1_name,MAX_NAME_SIZE);
	eeprom_read_block((void*)temp_rank2_name,(const void*)rank2_name,MAX_NAME_SIZE);
	eeprom_read_block((void*)temp_rank3_name,(const void*)rank3_name,MAX_NAME_SIZE);
	eeprom_read_block((void*)temp_rank4_name,(const void*)rank4_name,MAX_NAME_SIZE);
	eeprom_read_block((void*)temp_rank5_name,(const void*)rank5_name,MAX_NAME_SIZE);

	uint32_t temp_rank1_score = eeprom_read_dword(&rank1_score);
	uint32_t temp_rank2_score = eeprom_read_dword(&rank2_score);
	uint32_t temp_rank3_score = eeprom_read_dword(&rank3_score);
	uint32_t temp_rank4_score = eeprom_read_dword(&rank4_score);
	uint32_t temp_rank5_score = eeprom_read_dword(&rank5_score);

	// draws the words that will always be displayed during a highscore screen
	move_cursor(START_POS_X +13 ,START_POS_Y+2);
	printf_P(PSTR("Highscore"));

	move_cursor(RANK,CELL);
	printf_P(PSTR(" Rank"));
	move_cursor(SCORE,CELL);
	printf_P(PSTR("Score"));
	move_cursor(NAME,CELL);
	printf_P(PSTR("   Name"));

	set_display_attribute(FG_YELLOW);
	move_cursor(RANK,RANK1);
	printf_P(PSTR("  1  "));
	move_cursor(RANK,RANK2);
	printf_P(PSTR("  2 "));
	move_cursor(RANK,RANK3);
	printf_P(PSTR("  3 "));
	move_cursor(RANK,RANK4);
	printf_P(PSTR("  4  "));
	move_cursor(RANK,RANK5);
	printf_P(PSTR("  5 "));

	// Checks memory for current highscores if no highscore exists the space
	// will be left blank.
	if(temp_rank1_score != NO_SCORE) {
		set_display_attribute(FG_RED);
		move_cursor(SCORE,RANK1);
		printf_P(PSTR(" %u"), temp_rank1_score);

		set_display_attribute(FG_CYAN);
		move_cursor(NAME,RANK1);
		printf_P(PSTR("%s"), temp_rank1_name);
	}
	if(temp_rank2_score != NO_SCORE) {
		set_display_attribute(FG_RED);
		move_cursor(SCORE,RANK2);
		printf_P(PSTR(" %u"), temp_rank2_score);

		set_display_attribute(FG_CYAN);
		move_cursor(NAME,RANK2);
		printf_P(PSTR("%s"), temp_rank2_name);
	}
	if(temp_rank3_score != NO_SCORE) {
		set_display_attribute(FG_RED);
		move_cursor(SCORE,RANK3);
		printf_P(PSTR(" %u"), temp_rank3_score);
		set_display_attribute(FG_CYAN);
		move_cursor(NAME,RANK3);
		printf_P(PSTR("%s"), temp_rank3_name);
	}
	if(temp_rank4_score != NO_SCORE) {
		set_display_attribute(FG_RED);
		move_cursor(SCORE,RANK4);
		printf_P(PSTR(" %u"), temp_rank4_score);
		set_display_attribute(FG_CYAN);
		move_cursor(NAME,RANK4);
		printf_P(PSTR("%s"), temp_rank4_name);
	}
	if(temp_rank5_score != NO_SCORE) {
		set_display_attribute(FG_RED);
		move_cursor(SCORE,RANK5);
		printf_P(PSTR(" %u"), temp_rank5_score);
		set_display_attribute(FG_CYAN);
		move_cursor(NAME,RANK5);
		printf_P(PSTR("%s"), temp_rank5_name);
	}
	set_display_attribute(FG_GREEN);

	// Checks if a highscore has been achieved and adds it to memory and
	// displays it.
	if(gameover_flag == TRUE) {
		if(get_score() > temp_rank1_score) {
			input_highscore();
			eeprom_update_dword(&rank1_score, (uint32_t)get_score());
			eeprom_update_block((const void*)name_input,&rank1_name,MAX_NAME_SIZE);
			move_cursor(RANK,RANK1);
			clear_to_end_of_line();
		}
		else if(get_score() > temp_rank2_score) {
			input_highscore();
			eeprom_update_dword(&rank2_score, (uint32_t)get_score());
			eeprom_update_block((const void*)name_input,&rank2_name,MAX_NAME_SIZE);
			move_cursor(RANK,RANK2);
			clear_to_end_of_line();
		}
		else if(get_score() > temp_rank3_score) {
			input_highscore();
			eeprom_update_dword(&rank3_score, (uint32_t)get_score());
			eeprom_update_block((const void*)name_input,&rank3_name,MAX_NAME_SIZE);
			move_cursor(RANK,RANK3);
			clear_to_end_of_line();
		}
		else if(get_score() > temp_rank4_score) {
			input_highscore();
			eeprom_update_dword(&rank4_score, (uint32_t)get_score());
			eeprom_update_block((const void*)name_input,&rank4_name,MAX_NAME_SIZE);
			move_cursor(RANK,RANK4);
			clear_to_end_of_line();
		}
		else if(get_score() > temp_rank5_score) {
			input_highscore();
			eeprom_update_dword(&rank5_score, (uint32_t)get_score());
			eeprom_update_block((const void*)name_input,&rank5_name,MAX_NAME_SIZE);
			move_cursor(RANK,RANK5);
			clear_to_end_of_line();
		}
		gameover_flag = FALSE;
		draw_highscore_screen();
	}
}

// Draws stats to the game over screen and sets the gameover_flag.
// Also calls draw_highscore_screen()
void draw_gameover_screen(void) {
	gameover_flag = TRUE;
	move_cursor(1,START_POS_Y+4);
	printf_P(PSTR("Score: %u"), get_score());
	move_cursor(1,START_POS_Y+6);
	printf_P(PSTR("level: %u"), get_level());
	draw_highscore_screen();
}

/////////////////////////////// Private (Helper) Functions /////////////////////

// Draws the border for the highscore screen
static void terminal_draw_border(void) {
	// Top horizontal border line
	set_display_attribute(FG_MAGENTA);
	draw_horizontal_line(START_POS_Y, START_POS_X, END_POS_X);
	// Bottom horizontal border line
	set_display_attribute(FG_MAGENTA);
	draw_horizontal_line(END_POS_Y, START_POS_X, END_POS_X);
	// Left vertical border line
	set_display_attribute(FG_MAGENTA);
	draw_vertical_line(START_POS_X, START_POS_Y, END_POS_Y);
	// Right vertical border line
	set_display_attribute(FG_MAGENTA);
	draw_vertical_line(END_POS_X, START_POS_Y, END_POS_Y);
	set_display_attribute(FG_GREEN);
}

// Controls the logic for inputing a high score winners name
static void input_highscore(void) {
	// clears name_input
	for(uint8_t i = 0; i<MAX_NAME_SIZE; i++) {
		name_input[i] = 0;
	}

	move_cursor(END_POS_X+5, START_POS_Y+4);
	printf_P(PSTR("NEW HIGH SCORE!!"));
	move_cursor(END_POS_X+5, START_POS_Y+6);
	printf_P(PSTR("Enter a name"));
	move_cursor(INPUT_NAME_X, INPUT_NAME_Y);
	show_cursor();

	uint8_t i = 0;
	while(1) {
		if(serial_input_available()) {
			
			set_display_attribute(FG_CYAN);
			process_serial_in();
			
			if(serial_input == ENTER) {
				break;
			}
			else if(((serial_input == SPACE) ||
			(serial_input >= 65 && serial_input <= 90) ||
			(serial_input >= 97 && serial_input <= 122)) &&
			(i < 10)) {
				name_input[i] = serial_input;
				i++;
				move_cursor(INPUT_NAME_X, INPUT_NAME_Y);
				printf_P(PSTR("%s"), name_input);
				move_cursor(INPUT_NAME_X+i, INPUT_NAME_Y);
				clear_to_end_of_line();
			}
			else if((escape_sequence_char == LEFT_CURSOR) && (i > 0)) {
				i--;
				name_input[i] = 0;
				move_cursor(INPUT_NAME_X, INPUT_NAME_Y);
				printf_P(PSTR("%s"), name_input);
				move_cursor(INPUT_NAME_X+i, INPUT_NAME_Y);
				clear_to_end_of_line();
			}
			serial_input = NO_INPUT;
			escape_sequence_char = NO_INPUT;
		}
	}
	hide_cursor();
	move_cursor(INPUT_NAME_X, INPUT_NAME_Y);
	clear_to_end_of_line();
	move_cursor(INPUT_NAME_X, INPUT_NAME_Y-1);
	clear_to_end_of_line();
	set_display_attribute(FG_GREEN);
}

// Processes the serial input for escape characters
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