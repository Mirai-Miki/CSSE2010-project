/*
 * score.c
 *
 * Written by Peter Sutton. Modified by Michael Bossner
 */

#include "score.h"
#include "serialio.h"
#include "terminalio.h"
#include <stdio.h>
#include <avr/pgmspace.h>

uint32_t score;

static void score_updater(void);

void init_score(void) {
	score = 0;
	score_updater();
}

void add_to_score(uint16_t value) {
	score += value;
	score_updater();
}

uint32_t get_score(void) {
	return score;
}

void score_updater(void) {
	move_cursor(0, 1);
	printf_P(PSTR("Score: %4u"), score);
}
