/*
* life.h
*
*Author: Michael Bossner
*/

#ifndef LIFE_H_
#define LIFE_H_

#include <stdint.h>

/*
 * Initilises the hardware and game for use with lives.
 * Must be called first for lives to function correctly.
 */
void init_lives(void);


/*
 * Adds a life to the life count if the player does not already have the max
 * amount of lives and updates the terminal view.
 */
void add_life(void);

/*
 * Returns the amount of remaining lives the player has.
 */
uint8_t get_lives(void);

/*
 * if the frog is dead 1 life will be removed. If more then 1 life remains
 * the frog will be moved back to the start position and buttons and serial
 * input will be cleared. Updates the terminal view.
 */
void remove_life(void);

#endif