/*
* highscore.h
*
* Functions for adding a high score system to the game
*
*Author: Michael Bossner
*/

#ifndef HIGHSCORE_H_
#define HIGHSCORE_H_

#include <stdint.h>

/*
 * Initialises the hardware for use with highscores. If the EEPROM has not
 * been used before for the highscore data space will be allocated and
 * set up for future use.
 */
void init_highscore(void);

/*
 * Draws the highscore screen and loads highscore data from memory and
 * displays it. If no highscore has been achieved for a position no highscore
 * will be displayed in that spot.
 */
void draw_highscore_screen(void);

/*
 * Draws game over specific stats to the screen and also calls
 * draw_highscore_screen(). If a highscore has been achieved the screen will
 * request the user to input a name and save that name to the memory as well
 * as display the name on the screen.
 */
void draw_gameover_screen(void);

#endif