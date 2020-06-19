/*
* level.h
*
* Author: Michael Bossner
*/

#ifndef LEVEL_H_
#define LEVEL_H_

#include <stdint.h>
#include "pixel_colour.h"

// Macros for the get_row_speed() indexing
#define FIRST_VEHICLE_ROW_SPEED 0
#define SECOND_VEHICLE_ROW_SPEED 1
#define THIRD_VEHICLE_ROW_SPEED 2
#define FIRST_RIVER_ROW_SPEED 3
#define SECOND_RIVER_ROW_SPEED 4
#define RIVERBANK_ROW_SPEED 7

/*
 * Initialises the game ready for use with levels.
 * Must be called first for levels to function properly.
 */
void init_level(void);

/*
 * Adds one to the level and updates the level number on the terminal.
 */
void level_updater(void);

/*
 * Returns the current level.
 */
uint8_t get_level(void);

/*
 * Returns the lane data for the requested lane.
 */
uint64_t get_lane_data(uint8_t lane);

/*
 * Returns the log data for the requested channel
 */
uint32_t get_log_data(uint8_t channel);

/*
 * Returns the colour for the vehicles in the requested lane
 */
PixelColour get_lane_colours(uint8_t lane);

/*
 * Returns the speed at which the requested row will shift at
 */
uint16_t get_row_speed(uint8_t row);

#endif
 