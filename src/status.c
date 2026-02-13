#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "tinytsumego2/dual_solver.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/status.h"

static const char* UNKNOWN_STATUS = "? ";

static const char* DEAD_DEAD = "D ";
static const char* KO_DEAD = "KD";
static const char* SEKI_DEAD = "SD";
static const char* ALIVE_DEAD = "LD";

static const char* SEKI_SEKI = "S ";
static const char* KO_KO = "K ";
static const char* SEKI_KO = "SK";

static const char* ALIVE_KO = "LK";
static const char* ALIVE_SEKI = "LS";
static const char* ALIVE_ALIVE = "L ";

const char* tsumego_status_string(tsumego_status ts) {
  if (ts.player_first.life == SUPER_KO || ts.opponent_first.life == SUPER_KO) {
    return UNKNOWN_STATUS;
  }
  if (ts.opponent_first.life == DEAD) {
    switch (ts.player_first.life) {
      case DEAD:
        return DEAD_DEAD;
      case DEAD_UNLESS_KO_2:
      case DEAD_UNLESS_KO_1:
      case ALIVE_UNLESS_KO_1:
      case ALIVE_UNLESS_KO_2:
        return KO_DEAD;
      case SEKI:
        return SEKI_DEAD;
      case ALIVE:
        return ALIVE_DEAD;
      default:
    }
  }
  if (ts.player_first.life == ALIVE) {
    switch (ts.opponent_first.life) {
      case DEAD_UNLESS_KO_2:
      case DEAD_UNLESS_KO_1:
      case ALIVE_UNLESS_KO_1:
      case ALIVE_UNLESS_KO_2:
        return ALIVE_KO;
      case SEKI:
        return ALIVE_SEKI;
      case ALIVE:
        return ALIVE_ALIVE;
      default:
    }
  }
  if (ts.player_first.life == SEKI) {
    if (ts.opponent_first.life == SEKI) {
      return SEKI_SEKI;
    }
    return SEKI_KO;
  }
  return KO_KO;
}

// TODO: Promote to an actual dual_graph method
state navigate_principal_path(dual_graph *dg, state s) {
  bool low_color = s.white_to_play;
  value v = get_dual_graph_value(dg, &s, NONE);
  while (true) {
    bool found = false;
    for (int i = 0; i < dg->num_moves; ++i) {
      state child = s;
      const move_result r = make_move(&child, dg->moves[i]);
      if (r > TAKE_TARGET) {
        value child_v = get_dual_graph_value(dg, &child, NONE);
        if (s.white_to_play == low_color) {
          if (v.low == -child_v.high) {
            found = true;
          }
        } else if (v.high == -child_v.low) {
          found = true;
        }
        if (found) {
          s = child;
          v = child_v;
          break;
        }
      }
    }
    if (!found) {
      break;
    }
  }
  return s;
}

tsumego_status get_tsumego_status(const state *s) {
  tsumego_status result;
  dual_graph dg = {0};
  value v;
  state base;
  state c;

  assert(s->target & s->player);
  assert(!(s->target & s->opponent));

  // Normalize and solve base case
  base = *s;
  base.ko_threats = 2;
  base.passes = 0;
  base.button = 0;

  dg = create_dual_graph(&base);
  while(iterate_dual_graph(&dg, false));

  // Player first
  c = base;

  // First try to live with maximum resources
  c.ko_threats = 2;
  v = get_dual_graph_value(&dg, &c, NONE);
  if (v.low != v.high) {
    // Our ruleset is not powerful enough to determine status
    result.player_first.life = SUPER_KO;
  } else if (v.low < -BIG_SCORE) {
    // Target lost even with everything favoring the player
    result.player_first.life = DEAD;
  } else {
    c.ko_threats = 1;
    v = get_dual_graph_value(&dg, &c, NONE);
    if (v.low != v.high) {
      result.player_first.life = SUPER_KO;
    } else if (v.low < -BIG_SCORE) {
      // One ko threat was not enough for life
      result.player_first.life = DEAD_UNLESS_KO_2;
    } else {
      c.ko_threats = 0;
      v = get_dual_graph_value(&dg, &c, NONE);
      if (v.low != v.high) {
        result.player_first.life = SUPER_KO;
      } else if (v.low < -BIG_SCORE) {
        // Needed that ko threat
        result.player_first.life = DEAD_UNLESS_KO_1;
      } else {
        // It seems that we're alive. Can the opponent kill if they have ko threats?
        c.ko_threats = -2;
        v = get_dual_graph_value(&dg, &c, NONE);
        if (v.low != v.high) {
          result.player_first.life = SUPER_KO;
        } else if (v.low < -BIG_SCORE) {
          c.ko_threats = -1;
          v = get_dual_graph_value(&dg, &c, NONE);
          if (v.low != v.high) {
            result.player_first.life = SUPER_KO;
          } else if (v.low < -BIG_SCORE) {
            result.player_first.life = ALIVE_UNLESS_KO_1;
          } else {
            result.player_first.life = ALIVE_UNLESS_KO_2;
          }
        } else {
          // Opponent's ko threats don't seem to affect anything

          c.ko_threats = 0;

          c = navigate_principal_path(&dg, c);

          stones_t empty = c.logical_area & ~(c.player | c.opponent);
          stones_t player_libs = c.wide ? liberties_16(c.player, empty) : liberties(c.player, empty);
          stones_t opponent_libs = c.wide ? liberties_16(c.opponent, empty) : liberties(c.opponent, empty);

          if (player_libs & opponent_libs) {
            result.player_first.life = SEKI;
          } else {
            result.player_first.life = ALIVE;
          }
        }
      }
    }
  }

  c = navigate_principal_path(&dg, c);

  if (result.player_first.life == SUPER_KO) {
    // Our ruleset is not powerful enough to determine initiative
    result.player_first.initiative = UNKNOWN;
  } else {
    // Interpret getting the button as sente
    if (c.white_to_play == base.white_to_play ? c.button > 0 : c.button < 0) {
      result.player_first.initiative = SENTE;
    } else {
      result.player_first.initiative = GOTE;
    }
  }

  if (result.player_first.life == DEAD) {
    // Short-circuit
    result.opponent_first.life = DEAD;
    result.opponent_first.initiative = SENTE;
    free_dual_graph(&dg);
    return result;
  }

  // Opponent first
  c = base;
  c.player = base.opponent;
  c.opponent = base.player;
  c.white_to_play = !base.white_to_play;
  c.ko_threats = -2;  // Resist with maximum resources
  v = get_dual_graph_value(&dg, &c, NONE);
  if (v.low != v.high) {
    result.opponent_first.life = SUPER_KO;
  } else if (v.low > BIG_SCORE) {
    // Target lost even with everything favoring the player
    result.opponent_first.life = DEAD;
  } else {
    c.ko_threats = -1;
    v = get_dual_graph_value(&dg, &c, NONE);
    if (v.low != v.high) {
      result.opponent_first.life = SUPER_KO;
    } else if (v.low > BIG_SCORE) {
      result.opponent_first.life = ALIVE_UNLESS_KO_2;
    } else {
      c.ko_threats = 0;
      v = get_dual_graph_value(&dg, &c, NONE);
      if (v.low != v.high) {
        result.opponent_first.life = SUPER_KO;
      } else if (v.low > BIG_SCORE) {
        result.opponent_first.life = ALIVE_UNLESS_KO_1;
      } else {
        // We're alive. Give the opponent some chances
        c.ko_threats = 2;
        v = get_dual_graph_value(&dg, &c, NONE);
        if (v.low != v.high) {
          result.opponent_first.life = SUPER_KO;
        } else if (v.low > BIG_SCORE) {
          c.ko_threats = 1;
          v = get_dual_graph_value(&dg, &c, NONE);
          if (v.low != v.high) {
            result.opponent_first.life = SUPER_KO;
          } else if (v.low > BIG_SCORE) {
            result.opponent_first.life = ALIVE_UNLESS_KO_1;
          } else {
            result.opponent_first.life = ALIVE_UNLESS_KO_2;
          }
        } else {
          // Opponent's ko threats don't seem to affect anything

          c.ko_threats = 0;
          c = navigate_principal_path(&dg, c);

          stones_t empty = c.logical_area & ~(c.player | c.opponent);
          stones_t player_libs = c.wide ? liberties_16(c.player, empty) : liberties(c.player, empty);
          stones_t opponent_libs = c.wide ? liberties_16(c.opponent, empty) : liberties(c.opponent, empty);

          if (player_libs & opponent_libs) {
            result.opponent_first.life = SEKI;
          } else {
            result.opponent_first.life = ALIVE;
          }
        }
      }
    }
  }

  c = navigate_principal_path(&dg, c);

  if (result.opponent_first.life == SUPER_KO) {
    // Our ruleset is not powerful enough to determine initiative
    result.opponent_first.initiative = UNKNOWN;
  } else {
    // Interpret getting the button as sente
    if (c.white_to_play != base.white_to_play ? c.button < 0 : c.button > 0) {
      result.opponent_first.initiative = SENTE;
    } else {
      result.opponent_first.initiative = GOTE;
    }
  }

  free_dual_graph(&dg);

  return result;
}
