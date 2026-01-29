#pragma once
#include "tinytsumego2/stones.h"
#include "tinytsumego2/state.h"

// Conversion factor between 32-bit floating-point and 16-bit fixed-point
#define FLOAT_TO_SCORE_Q7 (128)

// Large value for capturing the target stones
#define TARGET_CAPTURED_SCORE (200)
#define TARGET_CAPTURED_SCORE_Q7 (TARGET_CAPTURED_SCORE * FLOAT_TO_SCORE_Q7)

// The maximum regular score (and then some)
#define BIG_SCORE (WIDTH * HEIGHT + 10)
#define BIG_SCORE_Q7 (BIG_SCORE * FLOAT_TO_SCORE_Q7)

// Delaying inevitable capture is incentivized
#define DELAY_Q7 (FLOAT_TO_SCORE_Q7 / 2)
#define DELAY_BONUS (0.5f)

// Taking the button awards 1/4 points. Not 1/2 to make it clear that playing the last dame is preferable to passing.
#define BUTTON_Q7 (FLOAT_TO_SCORE_Q7 / 4)
#define BUTTON_BONUS (BUTTON_Q7 / (float) FLOAT_TO_SCORE_Q7)

// Saving up abstract "external" ko-threats is incentivized
#define KO_THREAT_Q7 (FLOAT_TO_SCORE_Q7 / 32)
#define KO_THREAT_BONUS (KO_THREAT_Q7 / (float) FLOAT_TO_SCORE_Q7)

// Two bits of the fractional part saved up for comparison shenanigans and potential future use.

// 16-bit fixed-point datatype to save up on space
typedef signed short int score_q7_t;

// Conversions between fixed-point and floating-point
float score_q7_to_float(score_q7_t amount);
score_q7_t float_to_score_q7(float amount);

// Chinese-like score with bonus for taking the button and saving up ko-threats
score_q7_t score_q7(const state *s);
float score(const state *s);

// Big score for capturing the target. Stone score not included to reduce weird play. Button and ko threat bonuses are included
score_q7_t target_lost_score_q7(const state *s);
float target_lost_score(const state *s);

// Big score for losing the target. Button and ko threat bonuses are included
score_q7_t take_target_score_q7(const state *s);
float take_target_score(const state *s);

// Incentivize delaying if the target stones cannot be saved
score_q7_t delay_capture_q7(score_q7_t my_score);
float delay_capture(float my_score);

// TODO: Consider renaming target scoring functions or move results
