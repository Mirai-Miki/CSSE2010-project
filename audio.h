/*
* audio.h
*
* Functions and Macros for playing game audio
*
*Author: Michael Bossner
*/

#ifndef AUDIO_H_
#define AUDIO_H_

#include <stdint.h>

// No track is to be loaded
#define NO_TRACK -1
// Frog jumping audio track
#define FROG_JUMP 1
// Frog died audio track
#define FROG_DIED 2
// Frog made it to the other side audio track
#define FROG_MADE_IT 3
// Frog filled the riverbank audio track
#define FROG_LEVELUP 4
// Frog won the game audio track
// Unused while there are infinite levels
#define WINNER 5
// Game Over audio track
#define GAME_OVER 6
// Used for turning off audio on DDRD with a bit mask eg. (DDRD &= DDRD4_OFF)
#define DDRD4_OFF 0xEF

// Initalises the audio hardware for use in playing tones to DDRD4
void init_audio(void);

/* 
* Loads and plays the audio tracks. A track must be loaded to start the audio
* and the function must be called frequently after with the NO_TRACK macro
* to continue playing the audio track to completion.
* FROG_DIED, FROG_MADE_IT, WINNER and GAME_OVER tracks will play to completion
* without another call to the function. Done by design to delay the game in
* certain places.
*/
void play_audio(int track);

#endif