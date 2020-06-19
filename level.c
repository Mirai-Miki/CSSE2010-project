/*
* level.c
*
* Author: Michael Bossner
*/

#include "level.h"
#include "pixel_colour.h"
#include "serialio.h"
#include "terminalio.h"
#include "ledmatrix.h"
#include "game.h"
#include "countdown.h"
#include "audio.h"

#include <stdio.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#define F_CPU 8000000L
#include <util/delay.h>

////////////////////////////// Global variables ////////////////////////////////

// Indexing for the patterns in the arrays
#define PATTERN_1 0
#define PATTERN_2 1
#define PATTERN_3 2
#define PATTERN_4 3
#define PATTERN_5 4
// Maximum number of patterns stored
#define MAX_NUM_PATTERNS 5

// Initial speeds for the rows
#define ROW1_SPEED 1000
#define ROW2_SPEED 1300
#define ROW3_SPEED 865
#define ROW4_SPEED 1225
#define ROW5_SPEED 1150
// Number of rows
#define ROWS 5
// Number of Vehicle Lanes
#define NUM_LANES 3
// Number of Log Channels
#define NUM_CHANNELS 2
// The divider to be used on the row speeds at the end of each level.
// (ROW_SPEED/SPEED_INCREAS)
#define SPEED_INCREAS 1.3

// A multidimensional array for storing vehicle colours in different patterns
PixelColour vehicle_colour_list[MAX_NUM_PATTERNS][NUM_LANES] = {
	{ COLOUR_RED, COLOUR_YELLOW, COLOUR_RED },
	{ COLOUR_YELLOW, COLOUR_RED, COLOUR_YELLOW },
	{ COLOUR_RED, COLOUR_YELLOW, COLOUR_YELLOW },
	{ COLOUR_YELLOW, COLOUR_YELLOW, COLOUR_YELLOW },
	{ COLOUR_RED, COLOUR_RED, COLOUR_RED }
};

// A multidimensional array for storing lane data in different patterns
static uint64_t lane_data_list[MAX_NUM_PATTERNS][NUM_LANES] = {
	{
		0b1100001100011000110000011001100011000011000110001100000110011000,
		0b0011100000111000011100000111000011100001110001110000111000011100,
		0b0000111100001111000011110000111100001111000001111100001111000111
	},
	{
		0b1100001100011000110011011001100011000011011100001100000110011000,
		0b0011001100111000011001100111000011100001110001100110111000011100,
		0b0000111100001111000011101100111100001111000001111100001111000111
	},
	{
		0b1100001100011000111000011001100011000011100000001110000110011000,
		0b0011110000111000011100000111000011100001110001110000111000011100,
		0b0000111100001111000011110000111110001111100001111100001111000111
	},
	{
		0b1100001100011000111100011001100011000011000110001111000110011000,
		0b0011100000111000011111000111000011100001111101110000111000011100,
		0b0000111100001111000011110000111110000110000001111100001111000111
	},
	{
		0b1110001110011000110000011111100011000011000110001110000110011000,
		0b0011111000111000011100000111000011110001110001110000111000011100,
		0b0000111100001111000011110000111111111111000001111100001111000111
	}
};

// A multidimensional array for storing log data in different patterns
static uint32_t log_data_list[MAX_NUM_PATTERNS][NUM_CHANNELS] = {
	{
		0b11110001100111000111100011111000,
		0b11100110111101100001110110011100
	},
	{
		0b11100001100111000111100011111000,
		0b11100110111101101101110110011000
	},
	{
		0b11111000000111000111100011110000,
		0b11100110111001100001110110011000
	},
	{
		0b11110001100110000111000011110000,
		0b11100110111001100001100110011100
	},
	{
		0b11000001100111000111100011111000,
		0b10000110110001100001100110011100
	},
};

// An array for storing row speeds. The speeds will change every level.
static uint16_t row_speed[ROWS];

// An array that stores the initial row speeds for each row.
// These values will not change.
static const uint16_t initial_row_speed[ROWS] = {
	ROW1_SPEED,
	ROW2_SPEED,
	ROW3_SPEED,
	ROW4_SPEED,
	ROW5_SPEED
};

uint8_t pattern;
uint8_t level;


/////////////////// Function Prototypes for Helper Functions ///////////////////

static void levelup(void);
static void level_v_updater(void);

/////////////////////////////// Public Functions ///////////////////////////////

// Initialises the game for use with levels
void init_level(void) {
	pattern = PATTERN_1;
	level = 1;
	level_v_updater();
	for(uint8_t i = 0; i < ROWS; i++) {
		row_speed[i] = initial_row_speed[i];
	}
}

// Returns the current level
uint8_t get_level(void) {
	uint8_t return_value = level;
	return return_value;
}

// Returns the lane data for the particular lane requested.
// Depending on the level different patterns will be provided.
uint64_t get_lane_data(uint8_t lane) {
	uint64_t return_value = lane_data_list[pattern][lane];
	return return_value;
}

// Returns the log data for the particular channel requested.
// Depending on the level different patterns will be provided.
uint32_t get_log_data(uint8_t channel) {
	uint32_t return_value = log_data_list[pattern][channel];
	return return_value;
}

// Returns the colours for the vehicles in the particlar lane requested.
// Depending on the level different colours will be provided.
PixelColour get_lane_colours(uint8_t lane) {
	PixelColour return_value = vehicle_colour_list[pattern][lane];
	return return_value;
}

// Returns the movement speed for the row requested.
// Depending on the level differant speeds will be provided
uint16_t get_row_speed(uint8_t row) {
	uint16_t return_value = row_speed[row];
	return return_value;
}

// Controls the logic for the end of a level and start of a new level
void level_updater(void) {
	pause_countdown(TRUE);
	for(uint8_t i = 0; i < 32; i++) {
		play_audio(NO_TRACK);
		_delay_ms(50);
		if(i%2) {
			ledmatrix_shift_display_left();
		}
	};
	levelup();
	level_v_updater();
}

/////////////////////////////// Private (Helper) Functions /////////////////////

// A helper function that controls most of the end of level features
static void levelup(void) {
	if(level > 1) {
		// reduce time for scroll speeds
		for(uint8_t i = 0; i < 5; i++) {
			uint16_t temp = row_speed[i];
			temp /= SPEED_INCREAS;
			row_speed[i] = temp;
		}
	}
	if(pattern == PATTERN_5) {
		pattern = PATTERN_1;
		} else {
		pattern++;
	}
	level++;
}

// A helper function that updates the terminal display in regards to levels
static void level_v_updater(void) {
	move_cursor(30, 1);
	printf_P(PSTR("Level: %u"), get_level());
}