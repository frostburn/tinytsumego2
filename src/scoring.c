#include "tinytsumego2/scoring.h"

float score_q7_to_float(score_q7_t amount) {
  return ((float) amount) / ((float) (FLOAT_TO_SCORE_Q7));
}

score_q7_t float_to_score_q7(float amount) {
  return (score_q7_t)(amount * FLOAT_TO_SCORE_Q7);
}

score_q7_t score_q7(const state *s) {
  return (
    chinese_liberty_score(s) * FLOAT_TO_SCORE_Q7 +
    s->button * BUTTON_Q7 +
    s->ko_threats * KO_THREAT_Q7
  );
}
float score(const state *s) {
  return score_q7_to_float(score_q7(s));
}

score_q7_t target_lost_score_q7(const state *s) {
  return (
    -TARGET_CAPTURED_SCORE_Q7 +
    s->button * BUTTON_Q7 +
    s->ko_threats * KO_THREAT_Q7
  );
}
float target_lost_score(const state *s) {
  return score_q7_to_float(target_lost_score_q7(s));
}

score_q7_t delay_capture_q7(score_q7_t my_score) {
  if (my_score < -BIG_SCORE_Q7) {
    return my_score + DELAY_Q7;
  }
  return my_score;
}

// Explicit formula to deal with inf and nan
float delay_capture(float my_score) {
  if (my_score < -BIG_SCORE) {
    return my_score + DELAY_BONUS;
  }
  return my_score;
}
