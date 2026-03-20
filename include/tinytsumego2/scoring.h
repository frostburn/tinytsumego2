#pragma once
#include <limits.h>
#include "tinytsumego2/stones.h"
#include "tinytsumego2/state.h"

/**
 * @file scoring.h
 * @brief Fixed-point scoring utilities for solved tinytsumego positions.
 */

/** @brief Conversion factor from floating-point scores to Q7 fixed-point. */
#define FLOAT_TO_SCORE_Q7 (128)
/** @brief Large win value used when the target is captured. */
#define TARGET_CAPTURED_SCORE (200)
/** @brief Q7 representation of TARGET_CAPTURED_SCORE. */
#define TARGET_CAPTURED_SCORE_Q7 (TARGET_CAPTURED_SCORE * FLOAT_TO_SCORE_Q7)
/** @brief Maximum regular score plus headroom for tactical bonuses. */
#define BIG_SCORE (WIDTH * HEIGHT + 10)
/** @brief Q7 representation of BIG_SCORE. */
#define BIG_SCORE_Q7 (BIG_SCORE * FLOAT_TO_SCORE_Q7)
/** @brief Q7 bonus for delaying inevitable capture. */
#define DELAY_Q7 (FLOAT_TO_SCORE_Q7 / 2)
/** @brief Floating-point bonus for delaying inevitable capture. */
#define DELAY_BONUS (0.5f)
/** @brief Q7 bonus for taking the button. */
#define BUTTON_Q7 (FLOAT_TO_SCORE_Q7 / 4)
/** @brief Floating-point bonus for taking the button. */
#define BUTTON_BONUS (BUTTON_Q7 / (float) FLOAT_TO_SCORE_Q7)
/** @brief Q7 bonus for saving an external ko threat. */
#define KO_THREAT_Q7 (FLOAT_TO_SCORE_Q7 / 32)
/** @brief Floating-point bonus for saving an external ko threat. */
#define KO_THREAT_BONUS (KO_THREAT_Q7 / (float) FLOAT_TO_SCORE_Q7)
/** @brief Q7 bonus for playing forcing moves. */
#define FORCE_Q7 ((FLOAT_TO_SCORE_Q7 / 64) * 31)
/** @brief Floating-point bonus for playing forcing moves. */
#define FORCE_BONUS (31.0f / 64)
/** @brief Maximum representable Q7 score. */
#define SCORE_Q7_MAX (SHRT_MAX)
/** @brief Minimum representable Q7 score. */
#define SCORE_Q7_MIN (-SHRT_MAX)
/** @brief Sentinel used to represent an undefined Q7 score. */
#define SCORE_Q7_NAN (-32768)

/** @brief Signed 16-bit Q7 fixed-point score type. */
typedef signed short int score_q7_t;

/**
 * @brief Tactical scoring adjustments used during search.
 */
typedef enum tactics {
  /** @brief Use plain game-theoretic scoring without extra rewards. */
  NONE,
  /** @brief Reward delaying inevitable capture of the target. */
  DELAY,
  /** @brief Reward forcing moves while the button remains available. */
  FORCING,
} tactics;

/** @brief Lower and upper floating-point bounds for a game state's score. */
typedef struct value {
  float low;
  float high;
} value;

/** @brief Lower and upper Q7 bounds for a game state's score. */
typedef struct table_value {
  score_q7_t low;
  score_q7_t high;
} table_value;

/** @brief Convert a Q7 fixed-point score to floating point. */
float score_q7_to_float(score_q7_t amount);

/** @brief Convert a floating-point score to Q7 fixed point. */
score_q7_t float_to_score_q7(float amount);

/** @brief Compute Chinese-like score with button and ko-threat bonuses in Q7 form. */
score_q7_t score_q7(const state *s);

/** @brief Compute Chinese-like score with button and ko-threat bonuses. */
float score(const state *s);

/** @brief Score a state where the target has been lost, returned in Q7 form. */
score_q7_t target_lost_score_q7(const state *s);

/** @brief Score a state where the target has been lost. */
float target_lost_score(const state *s);

/** @brief Score a state where the target has been captured, returned in Q7 form. */
score_q7_t take_target_score_q7(const state *s);

/** @brief Score a state where the target has been captured. */
float take_target_score(const state *s);

/** @brief Apply the delay bonus to a Q7 score. */
score_q7_t delay_capture_q7(score_q7_t my_score);

/** @brief Apply the delay bonus to a floating-point score. */
float delay_capture(float my_score);

/** @brief Apply the forcing-move bonus to a Q7 score. */
score_q7_t reward_force_q7(score_q7_t my_score);

/** @brief Apply the forcing-move bonus to a floating-point score. */
float reward_force(float my_score);

/** @brief Score a terminal move result in Q7 form and convert to the parent's perspective. */
table_value score_terminal_q7(const move_result r, const state *child);

/** @brief Score a terminal move result and convert to the parent's perspective. */
value score_terminal(const move_result r, const state *child);

/** @brief Apply tactical bonuses to a child value in Q7 form and swap perspective. */
table_value apply_tactics_q7(const tactics ts, const move_result r, const state *child, const table_value child_value);

/** @brief Apply tactical bonuses to a child value and swap perspective. */
value apply_tactics(const tactics ts, const move_result r, const state *child, const value child_value);

/** @brief Convert a Q7 range to floating point. */
value table_value_to_value(table_value v);

/** @brief Full representable Q7 range. */
static const table_value MAX_RANGE_Q7 = (table_value) {SCORE_Q7_MIN, SCORE_Q7_MAX};
/** @brief Undefined Q7 range. */
static const table_value NAN_RANGE_Q7 = (table_value) {SCORE_Q7_NAN, SCORE_Q7_NAN};
