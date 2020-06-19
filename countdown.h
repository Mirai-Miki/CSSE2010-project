/*
* countdown.h
*
* Functions for the countdown timer in the game.
*
* Author: Michael Bossner
*/

#ifndef COUNTDOWN_H_
#define COUNTDOWN_H_

#include <stdint.h>

/*
 * Initilises the countdown for use in the game
 */
void init_countdown(void);

/*
 * Resets the countdown back to the time limit given
 */
void reset_countdown(void);

/*
 * Pauses the countdown. set=1 to pause and set=0 to unpause.
 */
void pause_countdown(uint8_t set);

#endif