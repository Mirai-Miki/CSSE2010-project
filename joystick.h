/*
* joystick.h
*
* Functions for Joystick control in the game.
*
*Author: Michael Bossner
*/

#ifndef JOYSTICK_H_
#define JOYSTICK_H_

#include <stdint.h>

// Joystick movement Macros
#define MOVE_UP 1
#define MOVE_LEFT 2
#define MOVE_RIGHT 3
#define MOVE_DOWN 4
// Joystick diagonal movement Macros
#define MOVE_UP_LEFT 5
#define MOVE_UP_RIGHT 6
#define MOVE_DOWN_LEFT 7
#define MOVE_DOWN_RIGHT 8
// All values >= to JOYSTICK_DIAGONAL_MOVE are diagonal movements.
#define JOYSTICK_DIAGONAL_MOVE 5

/*
 * Initialises the hardware for joystick use.
 * Must be done for the joystick to work.
 */
void init_joystick(void);

/*
 * A function for checking the position of the joystick and stacking moves
 * in a queue after a rest time has past.
 */
void joystick_move(void);

/*
 * A function that will pop a movement off the queue and return the value.
 * If no movement is in the queue a 0 will be returned.
 * Movement values that can be returned are 1 up to and including 8.
 */
uint8_t get_joystick_move(void);

/*
 * This function will clear the entire joystick movement queue.
 */
void clear_joystick_queue(void);

#endif