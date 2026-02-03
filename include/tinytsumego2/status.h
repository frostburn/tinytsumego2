#pragma once

#include "tinytsumego2/state.h"

typedef enum life_status {
  DEAD,  // No way to defend player's target stones
  DEAD_UNLESS_KO_2, // Dead unless the player has two external ko threats
  DEAD_UNLESS_KO_1, // Dead unless the player has an external ko threat
  SEKI,  // Alive without points
  SUPER_KO,  // Status depends on finer details of the ruleset
  ALIVE_UNLESS_KO_1, // Alive unless the opponent has an external ko threat
  ALIVE_UNLESS_KO_2, // Alive unless the opponent has two external ko threats
  ALIVE,  // Makes points and no way to attack
} life_status;

typedef enum initiative_status {
  GOTE,  // Lose tempo
  SENTE,  // Maintain tempo
  UNKNOWN,  // The ruleset is too vague to judge
} initiative_status;

typedef struct tsumego_sub_status {
  life_status life;
  initiative_status initiative;
} tsumego_sub_status;

typedef struct tsumego_status {
  tsumego_sub_status player_first;
  tsumego_sub_status opponent_first;
} tsumego_status;

// Compress tsumego_status to a simple two character string
const char* tsumego_status_string(tsumego_status ts);

// Get the status of the player's target stones
tsumego_status get_tsumego_status(const state *s);
