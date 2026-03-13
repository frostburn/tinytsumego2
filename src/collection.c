#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
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
  root.ko_threats = 2;

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

  state two_libs = root;
  two_libs.ko_threats = 1;
  float two_libs_score = 30 + BUTTON_BONUS + KO_THREAT_BONUS;

  tsumego tsumegos[] = {
    {"no-liberties", "No Outside Liberties", no_libs, false, {no_libs_score, no_libs_score}},
    {"one-liberty", "One Outside Liberty", one_lib, false, {one_lib_score, one_lib_score}},
    {"one-liberty-defense", "One Outside Liberty (Defense)", one_lib_def, true, {one_lib_def_score, one_lib_def_score}},
    {"two-liberties", "Two Outside Liberties", two_libs, true, {two_libs_score, two_libs_score}},
  };

  tsumego *ts = malloc(sizeof(tsumegos));
  memcpy(ts, tsumegos, sizeof(tsumegos));

  return (collection) {
    "rectangle-six",
    "Rectangular Six in the Corner",
    root,
    true,
    sizeof(tsumegos) / sizeof(tsumego),
    ts,
    COMPRESSED_KEYSPACE,
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
  root.ko_threats = 2;

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

  state one_lib = root;
  one_lib.ko_threats = 1;
  float one_lib_score = 24 + BUTTON_BONUS + KO_THREAT_BONUS;

  tsumego tsumegos[] = {
    {"no-liberties", "No Outside Liberties", no_libs, false, {no_libs_score, no_libs_score}},
    {"no-liberties-defense", "No Outside Liberties (Defense)", no_libs_def, true, {no_libs_def_score, no_libs_def_score}},
    {"one-liberty", "One Outside Liberty", one_lib, true, {one_lib_score, one_lib_score}},
  };

  tsumego *ts = malloc(sizeof(tsumegos));
  memcpy(ts, tsumegos, sizeof(tsumegos));

  return (collection) {
    "rectangle-eight",
    "Rectangular Eight in the Corner",
    root,
    true,
    sizeof(tsumegos) / sizeof(tsumego),
    ts,
    COMPRESSED_KEYSPACE,
  };
}

collection l_j_groups() {
  state root = parse_state("\
          . . . . . . . B , \
          . . . . . B B B , \
          . w w w B , , , , \
          . B + B , B , , , \
          . B B , , , , , , \
          B B , , , , , , , \
          , , , , , , , , , \
  ");
  root.ko_threats = 2;

  state l_group = parse_state(" \
              . . . . . . B , , \
              . . . w B B B , , \
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

  state straight_j_group_att = parse_state("\
                          . . . . 0 . . B , \
                          . . . . 0 B B B , \
                          . w w w B , , , , \
                          . B B B , B , , , \
                          . B , , , , , , , \
                          B B , , , , , , , \
  ");
  float straight_j_group_att_score = TARGET_CAPTURED_SCORE - BUTTON_BONUS;

  state straight_j_group_sac = straight_j_group_att;
  straight_j_group_sac.ko_threats = -1;
  float straight_j_group_sac_score = 43 - BUTTON_BONUS - KO_THREAT_BONUS;

  state straight_j_group_def = straight_j_group_att;
  swap_players(&straight_j_group_def);
  straight_j_group_def.ko_threats = -1;
  float straight_j_group_def_score = -35 - BUTTON_BONUS - KO_THREAT_BONUS;

  state straight_j_group_ko = parse_state("\
                          . . . . 0 . . B , \
                          . . . . 0 B B B , \
                          . w w w B , , , , \
                          . B + B , , , , , \
                          . B B , B , , , , \
                          B B , , , , , , , \
  ");
  straight_j_group_ko.ko_threats = -1;
  float straight_j_group_ko_score = 37 - BUTTON_BONUS;

  tsumego tsumegos[] = {
    {"l-group", "L Group", l_group, true, {l_group_score, l_group_score}},
    {"2nd-l-1-group-attack", "Second L+1 Group (Attack)", second_l_1_att, false, {second_l_1_att_score, second_l_1_att_score}},
    {"2nd-l-1-group-defense", "Second L+1 Group (Defense)", second_l_1_def, false, {second_l_1_def_score, second_l_1_def_score}},
    {"j-group-defense", "J Group (Defense)", j_group_def, false, {j_group_def_score, j_group_def_score}},
    {"j-group-attack", "J Group (Attack)", j_group_att, false, {j_group_att_score, j_group_att_score}},
    {"straight-j-group-defense", "Straight J Group (Defense)", straight_j_group_def, false, {straight_j_group_def_score, straight_j_group_def_score}},
    {"straight-j-group-attack", "Straight J Group (Attack)", straight_j_group_att, false, {straight_j_group_att_score, straight_j_group_att_score}},
    {"straight-j-group-sac", "Straight J Group (Sacrifice)", straight_j_group_sac, true, {straight_j_group_sac_score, straight_j_group_sac_score}},
    {"straight-j-group-ko", "Straight J Group (Ko)", straight_j_group_ko, true, {straight_j_group_ko_score, straight_j_group_ko_score}},
  };

  tsumego *ts = malloc(sizeof(tsumegos));
  memcpy(ts, tsumegos, sizeof(tsumegos));

  return (collection) {
    "l-j-groups",
    "L & J Groups",
    root,
    true,
    sizeof(tsumegos) / sizeof(tsumego),
    ts,
    COMPRESSED_KEYSPACE,
  };
}

collection notcher_122xy() {
  state root = notcher("122NN");
  mirror_v(&root);
  root.visual_area |= root.visual_area << 1;
  root.opponent |= single_16(9, 0) | single_16(9, 1);
  root.immortal = root.opponent;
  root.ko_threats = 2;

  state def_122NN = root;
  def_122NN.ko_threats = 0;
  float score_def_122NN = -18 + BUTTON_BONUS;

  state att_122NN = def_122NN;
  swap_players(&att_122NN);
  float score_att_122NN = TARGET_CAPTURED_SCORE - BUTTON_BONUS;

  // Unfortunately these are dead and there are no forcing moves
  state def_122WN = def_122NN;
  def_122WN.opponent |= single_16(3, 2);
  def_122WN.immortal = def_122WN.opponent;
  def_122WN.logical_area &= ~def_122WN.immortal;
  // float score_def_122WN = -TARGET_CAPTURED_SCORE + BUTTON_BONUS;

  state att_122WN = def_122WN;
  swap_players(&att_122WN);
  // float score_att_122WN = TARGET_CAPTURED_SCORE + BUTTON_BONUS;

  state def_122NS = def_122NN;
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
    true,
    sizeof(tsumegos) / sizeof(tsumego),
    ts,
    COMPRESSED_KEYSPACE,
  };
}

collection rectangular_goban(int width, int height, bool wide, keyspace_type type) {
  state root = {0};
  root.wide = wide;
  root.visual_area = wide ? rectangle_16(width, height) : rectangle(width, height);
  root.logical_area = root.visual_area;
  root.ko_threats = 2;

  state empty = root;
  empty.ko_threats = 0;

  tsumego tsumegos[] = {
    {"empty", "Empty Board", empty, false, {NAN, NAN}},
  };

  tsumego *ts = malloc(sizeof(tsumegos));
  memcpy(ts, tsumegos, sizeof(tsumegos));

  char *slug = malloc(128 * sizeof(char));
  char *title = malloc(128 * sizeof(char));

  sprintf(slug, "%dx%d", width, height);
  sprintf(title, "%dx%d Goban", width, height);

  return (collection) {
    slug,
    title,
    root,
    false,
    sizeof(tsumegos) / sizeof(tsumego),
    ts,
    type,
  };
}

collection carpenters_square() {
  state root = parse_state("\
          . . . . . . B , , \
          . . . w B B B , , \
          . . . w B , , , , \
          . w w w + B , , , \
          . B B B B , , , , \
          . B , , , , , , , \
          B B , , , , , , , \
  ");
  root.ko_threats = 2;

  state no_libs = parse_state(" \
              . . . . . . B , , \
              . . . w B B , B , \
              . . . w B , , , , \
              . w w w B , , , , \
              . B B B , B , , , \
              . B , , , , , , , \
              B B , , , , , , , \
  ");
  float no_libs_score = TARGET_CAPTURED_SCORE - BUTTON_BONUS;

  state seki = no_libs;
  seki.ko_threats = -1;
  float seki_score = 41 - BUTTON_BONUS - KO_THREAT_BONUS;

  state one_lib_att = root;
  one_lib_att.ko_threats = 0;
  float one_lib_att_score = TARGET_CAPTURED_SCORE - BUTTON_BONUS;

  state one_lib_def = root;
  one_lib_def.ko_threats = 0;
  make_move(&one_lib_def, single(2, 1));
  float one_lib_def_score = -37 + BUTTON_BONUS;

  tsumego tsumegos[] = {
    {"no-liberties", "No Outside Liberties", no_libs, false, {no_libs_score, no_libs_score}},
    {"seki", "No Outside Liberties (Seki)", seki, false, {seki_score, seki_score}},
    {"one-liberty-attack", "One Outside Liberty (Attack)", one_lib_att, false, {one_lib_att_score, one_lib_att_score}},
    {"one-liberty-defense", "One Outside Liberty (Defense)", one_lib_def, false, {one_lib_def_score, one_lib_def_score}},
  };

  tsumego *ts = malloc(sizeof(tsumegos));
  memcpy(ts, tsumegos, sizeof(tsumegos));

  return (collection) {
    "carpenters-square",
    "Carpenter's Square",
    root,
    true,
    sizeof(tsumegos) / sizeof(tsumego),
    ts,
    COMPRESSED_KEYSPACE,
  };
}

collection long_l_group() {
  state root = parse_state("\
          . . . . . . . B , \
          . . . . w B B B , \
          . w w w w B , , B \
          . . B + B B , , , \
          . B , B , , , , , \
          . B , , , , , , , \
          B B , , , , , , , \
  ");
  root.ko_threats = 2;

  state no_libs_mannen_ko = parse_state(" \
                        . . . . . . . B , \
                        . . . . w B B , , \
                        . w w w w B , B , \
                        . B B B B , , , , \
                        . B , , , B , , , \
                        B , B , , , , , , \
  ");
  float no_libs_mannen_ko_score = 45 - BUTTON_BONUS;

  state no_libs_ko = no_libs_mannen_ko;
  no_libs_ko.ko_threats = 1;
  float no_libs_ko_score = TARGET_CAPTURED_SCORE - BUTTON_BONUS + KO_THREAT_BONUS;

  state no_libs_hane_seki = no_libs_mannen_ko;
  no_libs_hane_seki.opponent |= single(0, 3);
  float no_libs_hane_seki_score = 39 - BUTTON_BONUS;

  state no_libs_hane_ko = no_libs_hane_seki;
  no_libs_hane_ko.ko_threats = 2;
  float no_libs_hane_ko_score = TARGET_CAPTURED_SCORE - BUTTON_BONUS;

  state one_lib = parse_state(" \
              . . . . . . . B , \
              . . . . w B B B , \
              . w w w w B , , B \
              . B B + B B , , , \
              . B , B , , , , , \
              B B , , , , , , , \
  ");
  one_lib.ko_threats = 1;
  float one_lib_score = 41 + BUTTON_BONUS + KO_THREAT_BONUS;

  state one_lib_life = one_lib;
  one_lib_life.ko_threats = -1;
  make_move(&one_lib_life, single(1, 1));
  float one_lib_life_score = -26 - BUTTON_BONUS + KO_THREAT_BONUS;

  state hane_outside = parse_state("\
                  . . . . . . . B , \
                  . . . . w B B B , \
                  . w w w w B , , B \
                  @ w B B B , B , , \
                  . B , , , , , , , \
                  . B , , , , , , , \
                  B B , , , , , , , \
  ");
  float hane_outside_score = 43 - BUTTON_BONUS;

  state descent_1 = parse_state("\
              . . . . . . . B , \
              . . . . w B B B , \
              . w w w w B , , , \
              B B B B B B , , , \
  ");
  descent_1.ko_threats = -2;
  float descent_1_score = TARGET_CAPTURED_SCORE - BUTTON_BONUS - 2 * KO_THREAT_BONUS;

  state descent_2 = parse_state(" \
                . . . . . B , , , \
                . . . . w B , , , \
                . w w w w B , , , \
                . B B B B B , , , \
                . B , , , , , , , \
                B , B , , , , , , \
  ");
  descent_2.ko_threats = -2;
  float descent_2_score = TARGET_CAPTURED_SCORE - BUTTON_BONUS - 2 * KO_THREAT_BONUS;

  tsumego tsumegos[] = {
    {"no-liberties-mannen-ko", "No Outside Liberties (Mannen-Ko)", no_libs_mannen_ko, false, {no_libs_mannen_ko_score, no_libs_mannen_ko_score}},
    {"no-liberties-ko", "No Outside Liberties (Ko)", no_libs_ko, false, {no_libs_ko_score, no_libs_ko_score}},
    {"no-liberties-hane-seki", "No Outside Liberties w/ Hane (Seki)", no_libs_hane_seki, false, {no_libs_hane_seki_score, no_libs_hane_seki_score}},
    {"no-liberties-hane-ko", "No Outside Liberties w/ Hane (Ko)", no_libs_hane_ko, false, {no_libs_hane_ko_score, no_libs_hane_ko_score}},
    {"one-liberty-seki", "One Outside Liberty (Seki)", one_lib, true, {one_lib_score, one_lib_score}},
    {"one-liberty-life", "One Outside Liberty (Life)", one_lib_life, false, {one_lib_life_score, one_lib_life_score}},
    {"hane-outside", "Hane Outside", hane_outside, true, {hane_outside_score, hane_outside_score}},
    {"1st-descent", "First Descent", descent_1, false, {descent_1_score, descent_1_score}},
    {"2nd-descent", "Second Descent", descent_2, false, {descent_2_score, descent_2_score}},
  };

  tsumego *ts = malloc(sizeof(tsumegos));
  memcpy(ts, tsumegos, sizeof(tsumegos));

  return (collection) {
    "long-l-group",
    "Long L Group",
    root,
    true,
    sizeof(tsumegos) / sizeof(tsumego),
    ts,
    COMPRESSED_KEYSPACE,
  };
}

collection five_on_3rd() {
  state root = parse_state("\
          . . . . . . . . B \
          . . . . . . . . B \
          w w w w w B B B , \
          B B B B B , , , B \
  ");
  root.ko_threats = 2;
  float root_score = TARGET_CAPTURED_SCORE - BUTTON_BONUS;

  state endgame = root;
  endgame.ko_threats = 0;

  state lesser_endgame = root;
  lesser_endgame.ko_threats = -1;
  float lesser_endgame_score = 10 + BUTTON_BONUS - KO_THREAT_BONUS;

  state endgame_def = root;
  swap_players(&endgame_def);
  endgame_def.ko_threats = 0;
  float endgame_def_score = -2 - BUTTON_BONUS;

  state lesser_endgame_def = endgame_def;
  lesser_endgame_def.ko_threats = -1;
  float lesser_endgame_def_score = -4 - BUTTON_BONUS - KO_THREAT_BONUS;

  state eternal_life = parse_state("\
                  . @ . 0 . @ 0 . B \
                  w @ @ @ w w B B B \
                  w w w w w B , , , \
                  B B B B B B , , , \
  ");
  swap_players(&eternal_life);

  tsumego tsumegos[] = {
    {"ko", "Ko", root, false, {root_score, root_score}},
    {"endgame-attack", "Endgame (Attack)", endgame, false, {12 + BUTTON_BONUS, 14 - BUTTON_BONUS}},
    {"lesser-endgame-attack", "Lesser Endgame (Attack)", lesser_endgame, false, {lesser_endgame_score, lesser_endgame_score}},
    {"endgame-defense", "Endgame (Defense)", endgame_def, false, {endgame_def_score, endgame_def_score}},
    {"lesser-endgame-defense", "Lesser Endgame (Defense)", lesser_endgame_def, false, {lesser_endgame_def_score, lesser_endgame_def_score}},
    {"eternal-life", "Eternal Life", eternal_life, false, {BUTTON_BONUS - TARGET_CAPTURED_SCORE, -4 - BUTTON_BONUS}},
  };

  tsumego *ts = malloc(sizeof(tsumegos));
  memcpy(ts, tsumegos, sizeof(tsumegos));

  return (collection) {
    "5-on-3rd",
    "Five on the Third Line in the Corner",
    root,
    true,
    sizeof(tsumegos) / sizeof(tsumego),
    ts,
    COMPRESSED_KEYSPACE,
  };
}

collection* get_collections(size_t *num_collections) {
  *num_collections = 20;
  collection *result = malloc(*num_collections * sizeof(collection));
  result[0] = rectangle_six();
  result[1] = rectangle_eight();
  result[2] = notcher_122xy();
  result[3] = l_j_groups();
  result[4] = carpenters_square();
  result[5] = five_on_3rd();
  result[6] = long_l_group();

  size_t i = 7;
  result[i+0] = rectangular_goban(3, 2, false, COMPRESSED_KEYSPACE);
  result[i+1] = rectangular_goban(3, 3, false, SYMMETRIC_KEYSPACE);
  result[i+2] = rectangular_goban(4, 2, true, SYMMETRIC_KEYSPACE);
  result[i+3] = rectangular_goban(4, 3, false, SYMMETRIC_KEYSPACE);
  result[i+4] = rectangular_goban(4, 4, false, SYMMETRIC_KEYSPACE);
  result[i+5] = rectangular_goban(5, 2, true, SYMMETRIC_KEYSPACE);
  result[i+6] = rectangular_goban(5, 3, false, SYMMETRIC_KEYSPACE);
  result[i+7] = rectangular_goban(5, 4, true, SYMMETRIC_KEYSPACE);
  result[i+7].root.ko_threats = 1;
  result[i+8] = rectangular_goban(6, 2, true, SYMMETRIC_KEYSPACE);
  result[i+9] = rectangular_goban(6, 3, false, SYMMETRIC_KEYSPACE);
  result[i+10] = rectangular_goban(7, 2, true, SYMMETRIC_KEYSPACE);
  result[i+11] = rectangular_goban(8, 2, true, SYMMETRIC_KEYSPACE);
  result[i+12] = rectangular_goban(9, 2, true, SYMMETRIC_KEYSPACE);
  return result;
}
