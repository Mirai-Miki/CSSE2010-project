/*
 * buttons.h
 *
 * Author: Peter Sutton. Modified by Michael Bossner
 *
 * We assume four push buttons (B0 to B3) are connected to pins B0 to B3. We configure
 * pin change interrupts on these pins.
 */


#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <stdint.h>

#define NO_BUTTON_PUSHED (-1)

/* Set up pin change interrupts on pins B0 to B3.
 * It is assumed that global interrupts are off when this function is called
 * and are enabled sometime after this function is called.
 */
void init_button_interrupts(void);

/* Return the last button pushed (0 to 3) or -1 (NO_BUTTON_PUSHED) if
 * there are no button pushes to return. (A small queue of button pushes
 * is kept. This function should be called frequently enough to
 * ensure the queue does not overflow. Excess button pushes are
 * discarded.)
 */
int8_t button_pushed(void);

// Clears the button queue
void clear_button_queue(void);

/* Returns the current button held. If more then one button is held the most
 * recent button to be held is returned. If no button is held NO_BUTTON_PUSHED
 * is returned.
 */
int8_t is_button_held(void);


#endif /* BUTTONS_H_ */