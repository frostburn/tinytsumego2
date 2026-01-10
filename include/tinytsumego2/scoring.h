#pragma once
#include "tinytsumego2/stones.h"
#include "tinytsumego2/state.h"

// Large value for capturing the target stones
#define TARGET_CAPTURED_SCORE (1000)

// The maximum regular score (and then some)
#define BIG_SCORE (WIDTH * HEIGHT + 10)

// Defining this disables delay tactics in face of inevitable capture of target stones
// #define NO_CAPTURE_DELAY

// Chinese-like score with bonus for taking the button and saving up ko-threats
float score(state *s);

// Big score for capturing the target. Stone score not included to reduce weird play. Button and ko threat bonuses are included
float target_lost_score(state *s);

// Incentivize delaying if the target stones cannot be saved
float delay_capture(float my_score);
