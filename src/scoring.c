#include <math.h>
#include <stdio.h>
#include "tinytsumego2/scoring.h"

float score_q7_to_float(score_q7_t amount) {
  return ((float) amount) / ((float) (FLOAT_TO_SCORE_Q7));
}

score_q7_t float_to_score_q7(float amount) {
  return (score_q7_t)(amount * FLOAT_TO_SCORE_Q7);
}

score_q7_t score_q7(const state *s) {
  return (
    simple_area_score(s) * FLOAT_TO_SCORE_Q7 +
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

score_q7_t take_target_score_q7(const state *s) {
  return (
    TARGET_CAPTURED_SCORE_Q7 +
    s->button * BUTTON_Q7 +
    s->ko_threats * KO_THREAT_Q7
  );
}
float take_target_score(const state *s) {
  return score_q7_to_float(take_target_score_q7(s));
}

score_q7_t delay_capture_q7(score_q7_t my_score) {
  if (my_score == SCORE_Q7_NAN) {
    return SCORE_Q7_NAN;
  }
  if (my_score == SCORE_Q7_MIN) {
    return SCORE_Q7_MIN;
  }
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

score_q7_t reward_force_q7(score_q7_t my_score) {
  if (my_score == SCORE_Q7_NAN) {
    return SCORE_Q7_NAN;
  }
  if (my_score == SCORE_Q7_MIN) {
    return SCORE_Q7_MIN;
  }
  if (my_score < BIG_SCORE_Q7) {
    return my_score + FORCE_Q7;
  }
  return my_score;
}

float reward_force(float my_score) {
  if (my_score < BIG_SCORE) {
    return my_score + FORCE_BONUS;
  }
  return my_score;
}

value table_value_to_value(table_value v) {
  if (v.low == SCORE_Q7_NAN) {
    return (value){NAN, NAN};
  }
  value result;
  if (v.low == SCORE_Q7_MIN) {
    result.low = -INFINITY;
  } else {
    result.low = score_q7_to_float(v.low);
  }
  if (v.high == SCORE_Q7_MAX) {
    result.high = INFINITY;
  } else {
    result.high = score_q7_to_float(v.high);
  }
  return result;
}

table_value score_terminal_q7(const move_result r, const state *child) {
  score_q7_t child_score;
  switch (r) {
    case ILLEGAL:
      return NAN_RANGE_Q7;
    case TARGET_LOST:
      // The logic is that the opponent took the lost target
       child_score = -take_target_score_q7(child);
       break;
    case SECOND_PASS:
      child_score = -score_q7(child);
      break;
    case TAKE_TARGET:
      // The player just took so the opponent lost
      child_score = -target_lost_score_q7(child);
      break;
    default:
      fprintf(stderr, "Reference value needed for non-ending moves\n");
      exit(EXIT_FAILURE);
  }
  return (table_value){child_score, child_score};
}

value score_terminal(const move_result r, const state *child) {
  float child_score;
  switch (r) {
    case ILLEGAL:
      return (value){NAN, NAN};
    case TARGET_LOST:
       child_score = -take_target_score(child);
       break;
    case SECOND_PASS:
      child_score = -score(child);
      break;
    case TAKE_TARGET:
      child_score = -target_lost_score(child);
      break;
    default:
      fprintf(stderr, "Reference value needed for non-ending moves\n");
      exit(EXIT_FAILURE);
  }
  return (value){child_score, child_score};
}

table_value apply_tactics_q7(const tactics ts, const move_result r, const state *child, const table_value child_value) {
  switch (ts) {
    case NONE:
      return (table_value){-child_value.low, -child_value.high};
    case DELAY:
      return (table_value) {
        -delay_capture_q7(child_value.low),
        -delay_capture_q7(child_value.high)
      };
    case FORCING:
      if (r > PASS && !child->button) {
        return (table_value) {
          reward_force_q7(-child_value.low),
          -child_value.high
        };
      } else {
        return (table_value){-child_value.low, -child_value.high};
      }
  }
  fprintf(stderr, "Unreachable\n");
  exit(EXIT_FAILURE);
}

value apply_tactics(const tactics ts, const move_result r, const state *child, const value child_value) {
  switch (ts) {
    case NONE:
      return (value){-child_value.low, -child_value.high};
    case DELAY:
      return (value) {
        -delay_capture(child_value.low),
        -delay_capture(child_value.high)
      };
    case FORCING:
      if (r > PASS && !child->button) {
        return (value) {
          reward_force(-child_value.low),
          -child_value.high
        };
      } else {
        return (value){-child_value.low, -child_value.high};
      }
  }
  fprintf(stderr, "Unreachable\n");
  exit(EXIT_FAILURE);
}
