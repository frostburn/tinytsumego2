#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tinytsumego2/status.h"

void test_straight_two_wide() {
  state s = parse_state("  \
    xxxx xxxx xxxx b b b b \
    xxxx xxxx xxxx b . . b \
    xxxx xxxx xxxx b b b b \
  ");
  s.wide = true;
  print_state(&s);
  s.wide = true;
  tsumego_status ts = get_tsumego_status(&s);
  assert(strcmp("D ", tsumego_status_string(ts)) == 0);
  assert(ts.player_first.life == DEAD);
  assert(ts.player_first.initiative == SENTE);
  assert(ts.opponent_first.life == DEAD);
  assert(ts.opponent_first.initiative == SENTE);
}

void test_straight_three() {
  state s = parse_state(" \
        b . . . b W , x x \
        b b b b b W , x x \
        W W W W W , W x x \
        B B B B B B B x x \
  ");
  print_state(&s);
  tsumego_status ts = get_tsumego_status(&s);
  assert(strcmp("LD", tsumego_status_string(ts)) == 0);
  assert(ts.player_first.life == ALIVE);
  assert(ts.player_first.initiative == GOTE);
  assert(ts.opponent_first.life == DEAD);
  assert(ts.opponent_first.initiative == SENTE);
}

void test_seki() {
  state s = parse_state(" \
        b . 0 B x x x x x \
        b . 0 B x x x x x \
        b . 0 B x x x x x \
        W W B B x x x x x \
  ");
  print_state(&s);
  tsumego_status ts = get_tsumego_status(&s);
  assert(strcmp("SD", tsumego_status_string(ts)) == 0);
  assert(ts.player_first.life == SEKI);
  assert(ts.player_first.initiative == GOTE);
  assert(ts.opponent_first.life == DEAD);
  assert(ts.opponent_first.initiative == SENTE);
}

void test_ko() {
  state s = parse_state(" \
        . b b b x x x x x \
        . . . b x x x x x \
  ");
  print_state(&s);
  tsumego_status ts = get_tsumego_status(&s);
  assert(strcmp("LK", tsumego_status_string(ts)) == 0);
  assert(ts.player_first.life == ALIVE);
  assert(ts.player_first.initiative == GOTE);
  assert(ts.opponent_first.life == ALIVE_UNLESS_KO_1);
  assert(ts.opponent_first.initiative == SENTE);
}

int main() {
  test_straight_two_wide();
  test_straight_three();
  test_seki();
  test_ko();
}
