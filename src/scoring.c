#include "tinytsumego2/scoring.h"

// Chinese-like score with bonus for taking the button and saving up ko-threats
float score(state *s) {
  return (
    chinese_liberty_score(s) +
    s->button * 0.5 +
    s->ko_threats * 0.0625
  );
}

// Big score for capturing the target. Stone score not included to reduce weird play. Button and ko threat bonuses are included
float target_lost_score(state *s) {
  return -TARGET_CAPTURED_SCORE + s->button * 0.5 + s->ko_threats * 0.0625;
}

// Incentivize delaying if the target stones cannot be saved
float delay_capture(float my_score) {
  #ifdef NO_CAPTURE_DELAY
    return my_score;
  #endif
  if (my_score < -BIG_SCORE) {
    return my_score + 0.25;
  }
  return my_score;
}
