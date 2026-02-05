#pragma once
#include <limits.h>
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

// Playing forcing moves is incentivized but not so much as to override taking the button
// The idea is to play as many forcing moves as possible and still get to tenuki
#define FORCE_Q7 ((FLOAT_TO_SCORE_Q7 / 64) * 31)
#define FORCE_BONUS (31.0f / 64)

// Limits and special values
#define SCORE_Q7_MAX (SHRT_MAX)
#define SCORE_Q7_MIN (-SHRT_MAX)
#define SCORE_Q7_NAN (-32768)

// One bit of the fractional part saved up for comparison shenanigans

// 16-bit fixed-point datatype to save up on space
typedef signed short int score_q7_t;

// Extra incentives for actions outside of the final score
typedef enum tactics {
  // No special rewards
  NONE,

  // Delaying the capture of your target stones is rewarded
  DELAY,

  // Playing non-passing moves while the button is on the table awards points. Simulates forcing moves during a ko-fight
  FORCING,
} tactics;

// Score range for a given game state. States with loops may not converge to a single score.
typedef struct value {
  float low;
  float high;
} value;

// Use 16-bit values to save space
typedef struct table_value {
  score_q7_t low;
  score_q7_t high;
} table_value;

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

// Incentivize playing forcing moves
score_q7_t reward_force_q7(score_q7_t my_score);
float reward_force(float my_score);

// Give score based on the move result and switch to parent's perspective
table_value score_terminal_q7(const move_result r, const state *child);
value score_terminal(const move_result r, const state *child);

// Apply tactics to the value of a child node and swap to parent's perspective
table_value apply_tactics_q7(const tactics ts, const move_result r, const state *child, const table_value child_value);
value apply_tactics(const tactics ts, const move_result r, const state *child, const value child_value);

value table_value_to_value(table_value v);

static const table_value MAX_RANGE_Q7 = (table_value) {SCORE_Q7_MIN, SCORE_Q7_MAX};
static const table_value NAN_RANGE_Q7 = (table_value) {SCORE_Q7_NAN, SCORE_Q7_NAN};
