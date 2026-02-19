#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "tinytsumego2/collection.h"
#include "tinytsumego2/state.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/shape.h"

collection rectangle_six() {
  state root = parse_state("\
          . . . w B , , , , \
          . . . w B , , , , \
          w w w w B , , , , \
          + + B B , B , , , \
          B B , , , , , , , \
          , , B , , , , , , \
  ");
  root.ko_threats = 1;
  float root_score = 30 + BUTTON_BONUS + KO_THREAT_BONUS;

  state no_libs = parse_state(" \
              . . . w B , , , , \
              . . . w B , , , , \
              w w w w B , , , , \
              B B B B , , , , , \
              , , , , B , , , , \
  ");
  no_libs.ko_threats = -1;
  float no_libs_score = TARGET_CAPTURED_SCORE - BUTTON_BONUS - KO_THREAT_BONUS;

  state one_lib = parse_state(" \
              . . . w B , , , , \
              . . . w B , , , , \
              w w w w B , , , , \
              + B B B B , , , , \
              B , , , , , , , , \
              , B , , , , , , , \
  ");
  float one_lib_score = TARGET_CAPTURED_SCORE - BUTTON_BONUS;

  state one_lib_def = one_lib;
  one_lib_def.ko_threats = -1;
  float one_lib_def_score = 30 + BUTTON_BONUS - KO_THREAT_BONUS;

  tsumego tsumegos[] = {
    {"no-liberties", "No Outside Liberties", no_libs, false, {no_libs_score, no_libs_score}},
    {"one-liberty", "One Outside Liberty", one_lib, false, {one_lib_score, one_lib_score}},
    {"one-liberty-defense", "One Outside Liberty (Defense)", one_lib_def, true, {one_lib_def_score, one_lib_def_score}},
    {"two-liberties", "Two Outside Liberties", root, true, {root_score, root_score}},
  };

  tsumego *ts = malloc(sizeof(tsumegos));
  memcpy(ts, tsumegos, sizeof(tsumegos));

  return (collection) {
    "rectangle-six",
    "Rectangular Six in the Corner",
    root,
    sizeof(tsumegos) / sizeof(tsumego),
    ts,
  };
}

collection rectangle_eight() {
  state root = parse_state("\
          . . . . w B , , , \
          . . . . w B , , , \
          w w w w w B , , , \
          + B B B B , B , , \
          B , , , , , , , , \
          , B , , , , , , , \
  ");
  root.ko_threats = 1;
  float root_score = 24 + BUTTON_BONUS + KO_THREAT_BONUS;

  state no_libs = parse_state(" \
              . . . . w B , , , \
              . . . . w B , , , \
              w w w w w B , , , \
              B B B B B , B , , \
  ");
  no_libs.ko_threats = 0;
  float no_libs_score = 32 + BUTTON_BONUS;

  state no_libs_def = no_libs;
  no_libs_def.ko_threats = -1;
  float no_libs_def_score = 24 + BUTTON_BONUS;

  tsumego tsumegos[] = {
    {"no-liberties", "No Outside Liberties", no_libs, false, {no_libs_score, no_libs_score}},
    {"no-liberties-defense", "No Outside Liberties (Defense)", no_libs_def, true, {no_libs_def_score, no_libs_def_score}},
    {"one-liberty", "One Outside Liberty", root, true, {root_score, root_score}},
  };

  tsumego *ts = malloc(sizeof(tsumegos));
  memcpy(ts, tsumegos, sizeof(tsumegos));

  return (collection) {
    "rectangle-eight",
    "Rectangular Eight in the Corner",
    root,
    sizeof(tsumegos) / sizeof(tsumego),
    ts,
  };
}

collection l_j_groups() {
  state root = parse_state("\
          . . . . . . . B , \
          . . . . . B B B , \
          . w w w B , , , , \
          . B B B , B , , , \
          . B , , , , , , , \
          B B , , , , , , , \
          , , , , , , , , , \
  ");
  root.ko_threats = 1;

  state l_group = parse_state(" \
              . . . . . . . B , \
              . . . w B B B B , \
              . w w w B , , , , \
              . B B B , B , , , \
              . B , , , , , , , \
              B B , , , , , , , \
  ");
  swap_players(&l_group);
  l_group.ko_threats = 1;
  float l_group_score = -TARGET_CAPTURED_SCORE + BUTTON_BONUS + KO_THREAT_BONUS;

  state second_l_1_att = parse_state("\
                    . . . . . . . B , \
                    . . . w w B B B , \
                    . w w w B , , , , \
                    . B B B , B , , , \
                    . B , , , , , , , \
                    B B , , , , , , , \
  ");
  float second_l_1_att_score = TARGET_CAPTURED_SCORE - BUTTON_BONUS;

  state second_l_1_def = second_l_1_att;
  swap_players(&second_l_1_def);
  float second_l_1_def_score = -37 + BUTTON_BONUS;

  state j_group_att = parse_state(" \
                  . . . 0 . . . B , \
                  . . . . 0 B B B , \
                  . w w w B , , , , \
                  . B B B , B , , , \
                  . B , , , , , , , \
                  B B , , , , , , , \
  ");
  float j_group_att_score = TARGET_CAPTURED_SCORE - BUTTON_BONUS;

  state j_group_def = j_group_att;
  swap_players(&j_group_def);
  float j_group_def_score = -35 - BUTTON_BONUS;

  tsumego tsumegos[] = {
    {"l-group", "L Group", l_group, true, {l_group_score, l_group_score}},
    {"2nd-l-1-group-attack", "Second L+1 Group (Attack)", second_l_1_att, false, {second_l_1_att_score, second_l_1_att_score}},
    {"2nd-l-1-group-defense", "Second L+1 Group (Defense)", second_l_1_def, false, {second_l_1_def_score, second_l_1_def_score}},
    {"j-group-defense", "J Group (Defense)", j_group_def, false, {j_group_def_score, j_group_def_score}},
    {"j-group-attack", "J Group (Attack)", j_group_att, false, {j_group_att_score, j_group_att_score}},
  };

  tsumego *ts = malloc(sizeof(tsumegos));
  memcpy(ts, tsumegos, sizeof(tsumegos));

  return (collection) {
    "l-j-groups",
    "L & J Groups",
    root,
    sizeof(tsumegos) / sizeof(tsumego),
    ts,
  };
}

collection notcher_122xy() {
  state root = notcher("122NN");
  mirror_v(&root);
  root.visual_area |= root.visual_area << 1;
  root.opponent |= single_16(9, 0) | single_16(9, 1);
  root.immortal = root.opponent;

  state def_122NN = root;
  float score_def_122NN = -18 + BUTTON_BONUS;

  state att_122NN = root;
  swap_players(&att_122NN);
  float score_att_122NN = TARGET_CAPTURED_SCORE - BUTTON_BONUS;

  // Unfortunately these are dead and there are no forcing moves
  state def_122WN = root;
  def_122WN.opponent |= single_16(3, 2);
  def_122WN.immortal = def_122WN.opponent;
  def_122WN.logical_area &= ~def_122WN.immortal;
  // float score_def_122WN = -TARGET_CAPTURED_SCORE + BUTTON_BONUS;

  state att_122WN = def_122WN;
  swap_players(&att_122WN);
  // float score_att_122WN = TARGET_CAPTURED_SCORE + BUTTON_BONUS;

  state def_122NS = root;
  def_122NS.player |= single_16(5, 2);
  def_122NS.target = def_122NS.player;
  def_122NS.logical_area &= ~def_122NS.target;
  float score_def_122NS = -16 - BUTTON_BONUS;

  state att_122NS = def_122NS;
  swap_players(&att_122NS);
  float score_att_122NS = TARGET_CAPTURED_SCORE - BUTTON_BONUS;

  state def_122SS = def_122NS;
  def_122SS.player |= single_16(3, 2);
  def_122SS.target = def_122SS.player;
  def_122SS.logical_area &= ~def_122SS.target;
  float score_def_122SS = -16 + BUTTON_BONUS;

  state att_122SS = def_122SS;
  swap_players(&att_122SS);
  float score_att_122SS = TARGET_CAPTURED_SCORE - BUTTON_BONUS;

  tsumego tsumegos[] = {
    {"122NN-defense", "122NN Defense", def_122NN, false, {score_def_122NN, score_def_122NN}},
    {"122NN-attack", "122NN Attack", att_122NN, false, {score_att_122NN, score_att_122NN}},
    // {"122WN-defense", "122WN Defense", def_122WN, true, {score_def_122WN, score_def_122WN}},
    // {"122WN-attack", "122WN Attack", att_122WN, true, {score_att_122WN, score_att_122WN}},
    {"122NS-defense", "122NS Defense", def_122NS, false, {score_def_122NS, score_def_122NS}},
    {"122NS-attack", "122NS Attack", att_122NS, false, {score_att_122NS, score_att_122NS}},
    {"122SS-defense", "122SS Defense", def_122SS, false, {score_def_122SS, score_def_122SS}},
    {"122SS-attack", "122SS Attack", att_122SS, false, {score_att_122SS, score_att_122SS}},
  };

  tsumego *ts = malloc(sizeof(tsumegos));
  memcpy(ts, tsumegos, sizeof(tsumegos));

  return (collection) {
    "122xy",
    "Notchers 122xy",
    root,
    sizeof(tsumegos) / sizeof(tsumego),
    ts,
  };
}

collection* get_collections(size_t *num_collections) {
  *num_collections = 4;
  collection *result = malloc(*num_collections * sizeof(collection));
  result[0] = rectangle_six();
  result[1] = rectangle_eight();
  result[2] = notcher_122xy();
  result[3] = l_j_groups();
  return result;
}
