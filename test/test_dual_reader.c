#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "tinytsumego2/state.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/dual_solver.h"
#include "tinytsumego2/dual_reader.h"

#define MEM_FILE_SIZE (1000000)

state bulky_five() {
  state s = {0};
  s.visual_area = rectangle(5, 4);
  s.logical_area = (rectangle(2, 2) << (H_SHIFT + V_SHIFT)) | single(3, 1);
  s.target = s.visual_area ^ s.logical_area;
  s.opponent = s.target;
  return s;
}

void test_bulky_five() {
  const state root = bulky_five();
  print_state(&root);
  dual_graph dg = create_dual_graph(&root);
  while(iterate_dual_graph(&dg, false));
  while(area_iterate_dual_graph(&dg, false));

  char *buffer = malloc(MEM_FILE_SIZE);
  FILE *stream = fmemopen(buffer, MEM_FILE_SIZE, "wb");
  write_dual_graph(&dg, stream);
  fclose(stream);
  free_dual_graph(&dg);

  dual_graph_reader dgr = {0};
  dgr.fd = -1;
  dgr.buffer = buffer;
  unbuffer_dual_graph_reader(&dgr);

  dual_value v = get_dual_graph_reader_value(&dgr, &root);

  assert(v.plain.low == TARGET_CAPTURED_SCORE - BUTTON_BONUS);
  assert(v.plain.high == TARGET_CAPTURED_SCORE - BUTTON_BONUS);
  assert(v.forcing.low == TARGET_CAPTURED_SCORE - BUTTON_BONUS - FORCE_BONUS);
  assert(v.forcing.high == TARGET_CAPTURED_SCORE - BUTTON_BONUS);

  int num_infos = 0;
  move_info *mis = dual_graph_reader_move_infos(&dgr, &root, &num_infos);
  for (int i = 0; i < num_infos; ++i) {
    assert(mis[i].low_gain <= 0);
    assert(mis[i].high_gain <= 0);
  }
  free(mis);

  state terminal = dual_graph_reader_low_terminal(&dgr, &root, FORCING);
  print_state(&terminal);
  assert(terminal.white_to_play);
  assert(!terminal.player);

  terminal = dual_graph_reader_high_terminal(&dgr, &root, FORCING);
  print_state(&terminal);
  assert(terminal.white_to_play);
  assert(!terminal.player);

  unload_dual_graph_reader(&dgr);

  free(buffer);
}

int main() {
  test_bulky_five();
  return 0;
}
