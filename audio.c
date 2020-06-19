/*
* audio.c
* 
* loading and playing audio tracks for the game
*
*Author: Michael Bossner
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "audio.h"
#include "timer0.h"
#include "game.h"

////////////////////////////// Global variables ////////////////////////////////

// time between each note with no tone
#define REST_TIME 50

// Note frequencies for the buzzer
#define NO_NOTE 0
#define C 261
#define D 294
#define E 329
#define F 349
#define G 391
#define GS 415
#define A 440
#define AS 455
#define B 466
#define CH 523
#define CSH 554
#define DH 587
#define DSH 622
#define EH 659
#define FH 698
#define FSH 740
#define GH 784
#define GSH 830
#define AH 880

static uint16_t freq;
static uint16_t clockperiod;
static float dutycycle;
static uint16_t pulsewidth;
static uint32_t tone_start_time;
static uint32_t rest_start_time;
static uint8_t rest = FALSE;
static uint8_t tone = FALSE;
static uint8_t count = 0;
static uint8_t is_track_loaded = FALSE;
static uint16_t *loaded_track;
static uint16_t *loaded_track_duration;
static uint8_t array_size;
static uint16_t tone_duration;

// Unused track
/*
static uint16_t track_bgm[74] = {A,A,A,F,CH,A,F,CH,A,NO_NOTE,EH,EH,EH,FH,CH,GS,
	F,CH,A,NO_NOTE,AH,A,A,AH,GSH,GH,FSH,FH,FSH,NO_NOTE,AS,DSH,DH,CSH,CH,B,CH,
	NO_NOTE,F,GS,F,A,CH,A,CH,EH,NO_NOTE,AH,A,A,AH,GSH,GH,FSH,FH,FSH,NO_NOTE,AS,
	DSH,DH,CSH,CH,B,CH,NO_NOTE,F,GS,F,CH,A,F,CH,A,NO_NOTE
};

static uint16_t track_bgm_duration[74] = {500,500,500,350,150,500,350,150,650,
	500,500,500,500,350,150,500,350,150,650,500,500,300,150,500,325,175,125,125,
	250,325,250,500,325,175,125,125,250,350,250,500,350,125,500,375,125,650,500,
	500,300,150,500,325,175,125,125,250,325,250,500,325,175,125,125,250,350,250,
	500,375,125,500,375,125,650,650
};
*/

// Audio Tracks
#define TRACK_FROG_JUMP_SIZE 2
static uint16_t track_frog_jump_tone[2] = {A,C};
static uint16_t track_frog_jump_duration[2] = {50,25};

#define TRACK_FROG_DIED_SIZE 5
static uint16_t track_frog_died_tone[5] = {A,F,E,D,C};
static uint16_t track_frog_died_duration[5] = {100,100,75,75,200};
	
#define TRACK_MADE_IT_SIZE 5
static uint16_t track_made_it_tone[5] = {C,D,E,F,A};
static uint16_t track_made_it_duration[5] = {100,100,75,75,200};

#define TRACK_LEVELUP_SIZE 5
static uint16_t track_levelup_tone[5] = {E,C,C,E,A};
static uint16_t track_levelup_duration[5] = {100,100,100,100,200};

// unused track while there is infinite levels
#define TRACK_WINNER_SIZE 6
static uint16_t track_winner_tone[6] = {C,E,E,A,E,A};
static uint16_t track_winner_duration[6] = {100,50,50,50,50,200};

#define TRACK_GAME_OVER_SIZE 3
static uint16_t track_game_over_tone[3] = {E,C,C};
static uint16_t track_game_over_duration[3] = {100,50,200};

/////////////////// Function Prototypes for Helper Functions ///////////////////
static uint16_t freq_to_clock_period(uint16_t freq);
static uint16_t duty_cycle_to_pulse_width(float dutycycle, uint16_t clockperiod);
static void track_helper(void);

/////////////////////////////// Public Functions ///////////////////////////////

// Initialses the audio hardware for use
void init_audio(void) {
freq = 0;
dutycycle = 20;
clockperiod = freq_to_clock_period(freq);
pulsewidth = duty_cycle_to_pulse_width(dutycycle, clockperiod);

// Set the maximum count value for timer/counter 1 to be one less than the clockperiod
OCR1A = clockperiod - 1;

// Set the count compare value based on the pulse width. The value will be 1 less
// than the pulse width - unless the pulse width is 0.
OCR1B = pulsewidth - 1;

// Turns audio output off
DDRD &= DDRD4_OFF;

// Set up timer/counter 1 for Fast PWM, counting from 0 to the value in OCR1A
// before reseting to 0. Count at 1MHz (CLK/8).
// Configure output OC1B to be clear on compare match and set on timer/counter
// overflow (non-inverting mode).
TCCR1A = (1 << COM1B1) | (0 <<COM1B0) | (1 <<WGM11) | (1 << WGM10);
TCCR1B = (1 << WGM13) | (1 << WGM12) | (0 << CS12) | (1 << CS11) | (0 << CS10);
}

// Loads and plays audio tracks
void play_audio(int track) {
	// Turns the volume down if switch S6 is set to 1
	if(PIND & 0x04) {
		dutycycle = 0.2;
	} else {
		dutycycle = 20;
	}
	// If a track is loaded it will continue to play
	if((track == NO_TRACK) && (is_track_loaded == TRUE)) {
		// if in a rest period during a track and the rest time has expired 
		// turn rest off
		if(rest && (count < array_size) &&
		(get_current_time() >= (rest_start_time + REST_TIME))) {
			rest = FALSE;
		}
		// If a tone is playing and the tone duration has been reached turn the
		// tone off and enter the rest period
		else if(!rest && tone &&
		(get_current_time() >= (tone_start_time + tone_duration))) {
			// Turn audio output off
			DDRD &= DDRD4_OFF;			
			rest_start_time = get_current_time();
			rest = TRUE;
			tone = FALSE;
		}
		// If the rest period has ended and there is still more track left
		// play the next tone in the track
		else if(!rest && !tone && (count < array_size))  {
			track_helper();
		}
		// If the rest period has ended and there is no more track left
		// unload the track from use
		else if(rest && (count >= array_size)) {
			rest = FALSE;
			is_track_loaded = FALSE;
		}
	}
	// load frog jump track
	else if(track == FROG_JUMP) {
		is_track_loaded = TRUE;
		count = 0;
		loaded_track = track_frog_jump_tone;
		loaded_track_duration = track_frog_jump_duration;
		array_size = TRACK_FROG_JUMP_SIZE;
		track_helper();
	}
	// load frog died track
	else if(track == FROG_DIED) {
		is_track_loaded = TRUE;
		count = 0;
		loaded_track = track_frog_died_tone;
		loaded_track_duration = track_frog_died_duration;
		array_size = TRACK_FROG_DIED_SIZE;
		track_helper();
		// Delay game until finished used instead of _delay_ms()
		while(is_track_loaded == TRUE) {			
			play_audio(NO_TRACK);
		}
	}
	// load frog made it track
	else if(track == FROG_MADE_IT) {
		is_track_loaded = TRUE;
		count = 0;
		loaded_track = track_made_it_tone;
		loaded_track_duration = track_made_it_duration;
		array_size = TRACK_MADE_IT_SIZE;
		track_helper();
		// Delay game until finished used instead of _delay_ms()
		while(is_track_loaded == TRUE) {
			play_audio(NO_TRACK);
		}
	}
	// load level up track
	else if(track == FROG_LEVELUP) {
		is_track_loaded = TRUE;
		count = 0;
		loaded_track = track_levelup_tone;
		loaded_track_duration = track_levelup_duration;
		array_size = TRACK_LEVELUP_SIZE;
		track_helper();
	}
	// load winner track
	else if(track == WINNER) {
		is_track_loaded = TRUE;
		count = 0;
		loaded_track = track_winner_tone;
		loaded_track_duration = track_winner_duration;
		array_size = TRACK_WINNER_SIZE;
		track_helper();
		// Delay game until finished used instead of _delay_ms()
		while(is_track_loaded == TRUE) {
			play_audio(NO_TRACK);
		}
	}
	// load game over track
	else if(track == GAME_OVER) {
		is_track_loaded = TRUE;
		count = 0;
		loaded_track = track_game_over_tone;
		loaded_track_duration = track_game_over_duration;
		array_size = TRACK_GAME_OVER_SIZE;
		track_helper();
		// Delay game until finished used instead of _delay_ms()
		while(is_track_loaded == TRUE) {
			play_audio(NO_TRACK);
		}
	}
}

/////////////////////////////// Private (Helper) Functions /////////////////////

// For a given frequency (Hz), return the clock period (in terms of the
// number of clock cycles of a 1MHz clock)
static uint16_t freq_to_clock_period(uint16_t freq) {
	return (1000000UL / freq);
}

// Return the width of a pulse (in clock cycles) given a duty cycle (%) and
// the period of the clock (measured in clock cycles)
static uint16_t duty_cycle_to_pulse_width(float dutycycle, uint16_t clockperiod) {
	return (dutycycle * clockperiod) / 100;
}

// Controls what tones will be played and increments the array counter
static void track_helper(void) {
	tone = TRUE;
	freq = loaded_track[count];
	clockperiod = freq_to_clock_period(freq);
	pulsewidth = duty_cycle_to_pulse_width(dutycycle, clockperiod);
	OCR1A = clockperiod - 1;
	OCR1B = pulsewidth - 1;
	tone_duration = loaded_track_duration[count];
	tone_start_time = get_current_time();
	if((loaded_track[count] != NO_NOTE) && !(PIND & 0x08)) {
		DDRD |= (1<<DDRD4);
	}
	count++;
}
