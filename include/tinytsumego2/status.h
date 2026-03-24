#pragma once

#include "tinytsumego2/state.h"

/**
 * @file status.h
 * @brief Human-oriented life-and-death status summaries.
 */

/**
 * @brief Coarse life status of the player's target stones.
 */
typedef enum life_status {
  DEAD,              /**< No line of play saves the target stones. */
  DEAD_UNLESS_KO_2,  /**< Dead unless the player has two external ko threats. */
  DEAD_UNLESS_KO_1,  /**< Dead unless the player has one external ko threat. */
  SEKI,              /**< Alive without points. */
  SUPER_KO,          /**< Outcome depends on ruleset-specific superko handling. */
  ALIVE_UNLESS_KO_1, /**< Alive unless the opponent has one external ko threat. */
  ALIVE_UNLESS_KO_2, /**< Alive unless the opponent has two external ko threats. */
  ALIVE,             /**< Alive with points and no effective attack. */
} life_status;

/**
 * @brief Tempo summary for the relevant side's best line.
 */
typedef enum initiative_status {
  GOTE,    /**< Correct play loses tempo. */
  SENTE,   /**< Correct play keeps tempo. */
  UNKNOWN, /**< The ruleset is too vague to decide tempo cleanly. */
} initiative_status;

/** @brief Status summary for one side to move first. */
typedef struct tsumego_sub_status {
  life_status life;
  initiative_status initiative;
} tsumego_sub_status;

/** @brief Combined status summary for both move orders. */
typedef struct tsumego_status {
  tsumego_sub_status player_first;
  tsumego_sub_status opponent_first;
} tsumego_status;

/** @brief Compress a tsumego_status into a stable two-character code string. */
const char *tsumego_status_string(tsumego_status ts);

/** @brief Evaluate the life-and-death status of the player's target stones. */
tsumego_status get_tsumego_status(const state *s);
