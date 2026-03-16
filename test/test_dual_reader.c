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

state rectangle_six() {
  state s = {0};
  s.visual_area = rectangle(4, 5);
  s.external = rectangle(2, 1) << (3 * V_SHIFT);
  s.logical_area = rectangle(3, 2) | s.external;
  s.opponent = rectangle(4, 3) ^ rectangle(3, 2);
  s.target = s.opponent;
  s.player = rectangle(4, 2) << (3 * V_SHIFT);
  s.immortal = s.player ^ s.external;
  s.ko_threats = 0;
  return s;
}

void test_bulky_five() {
  const state root = bulky_five();
  print_state(&root);
  dual_graph dg = create_dual_graph(&root, COMPRESSED_KEYSPACE);
  while(iterate_dual_graph(&dg, false));
  while(area_iterate_dual_graph(&dg, true));

  size_t value_map_size = 0;
  dual_table_value *value_map = create_value_map(&dg, &value_map_size);
  printf("%zu unique value quads in the graph\n", value_map_size);
  assert(value_map_size == 16);

  char *buffer = malloc(MEM_FILE_SIZE);
  FILE *stream = fmemopen(buffer, MEM_FILE_SIZE, "wb");
  write_dual_graph(&dg, value_map, value_map_size, stream);
  fclose(stream);
  free(value_map);
  free_dual_graph(&dg);

  dual_graph_reader dgr = {0};
  dgr.fd = -1;
  dgr.buffer = buffer;
  unbuffer_dual_graph_reader(&dgr);

  for (size_t i = 0; i < dgr.value_map_size; ++i) {
    dual_value v = dgr.value_map[i];
    printf("#%zu: (%f, %f), (%f, %f)\n", i, v.plain.low, v.plain.high, v.forcing.low, v.forcing.high);
  }

  dual_value v = get_dual_graph_reader_value(&dgr, &root);

  printf("(%f, %f), (%f, %f)\n", v.plain.low, v.plain.high, v.forcing.low, v.forcing.high);

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

void test_external_liberties() {
  const state root = rectangle_six();
  print_state(&root);
  dual_graph dg = create_dual_graph(&root, COMPRESSED_KEYSPACE);
  while(iterate_dual_graph(&dg, false));
  while(area_iterate_dual_graph(&dg, true));

  size_t value_map_size = 0;
  dual_table_value *value_map = create_value_map(&dg, &value_map_size);
  printf("%zu unique value quads in the graph\n", value_map_size);
  assert(value_map_size == 31);

  char *buffer = malloc(MEM_FILE_SIZE);
  FILE *stream = fmemopen(buffer, MEM_FILE_SIZE, "wb");
  write_dual_graph(&dg, value_map, value_map_size, stream);
  fclose(stream);
  free(value_map);
  free_dual_graph(&dg);

  dual_graph_reader dgr = {0};
  dgr.fd = -1;
  dgr.buffer = buffer;
  unbuffer_dual_graph_reader(&dgr);

  for (size_t i = 0; i < dgr.value_map_size; ++i) {
    dual_value v = dgr.value_map[i];
    printf("#%zu: (%f, %f), (%f, %f)\n", i, v.plain.low, v.plain.high, v.forcing.low, v.forcing.high);
  }

  dual_value v = get_dual_graph_reader_value(&dgr, &root);

  printf("(%f, %f), (%f, %f)\n", v.plain.low, v.plain.high, v.forcing.low, v.forcing.high);

  assert(v.plain.low == -4 + BUTTON_BONUS);
  assert(v.plain.high == -4 + BUTTON_BONUS);
  assert(v.forcing.low == -4 + BUTTON_BONUS);
  assert(v.forcing.high == -1.8125);

  int num_infos = 0;
  move_info *mis = dual_graph_reader_move_infos(&dgr, &root, &num_infos);
  bool left_found = false;
  bool right_found = false;
  for (int i = 0; i < num_infos; ++i) {
    assert(mis[i].low_gain <= 0);
    assert(mis[i].high_gain <= 0);
    if (mis[i].coords.x == 0 && mis[i].coords.y == 3) {
      left_found = true;
    }
    if (mis[i].coords.x == 1 && mis[i].coords.y == 3) {
      right_found = true;
    }
  }
  assert(left_found);
  assert(right_found);
  free(mis);

  state terminal = dual_graph_reader_low_terminal(&dgr, &root, FORCING);
  print_state(&terminal);
  assert(terminal.white_to_play);
  assert(terminal.player);

  terminal = dual_graph_reader_high_terminal(&dgr, &root, FORCING);
  print_state(&terminal);
  assert(terminal.white_to_play);
  assert(terminal.player);

  state s = root;
  make_move(&s, single(0, 3));
  make_move(&s, pass());
  print_state(&s);

  mis = dual_graph_reader_move_infos(&dgr, &s, &num_infos);
  left_found = false;
  right_found = false;
  for (int i = 0; i < num_infos; ++i) {
    assert(mis[i].low_gain <= 0);
    assert(mis[i].high_gain <= 0);
    if (mis[i].coords.x == 0 && mis[i].coords.y == 3) {
      left_found = true;
    }
    if (mis[i].coords.x == 1 && mis[i].coords.y == 3) {
      right_found = true;
    }
  }
  assert(!left_found);
  assert(right_found);

  s = root;
  make_move(&s, single(1, 3));
  make_move(&s, pass());
  print_state(&s);

  mis = dual_graph_reader_move_infos(&dgr, &s, &num_infos);
  left_found = false;
  right_found = false;
  for (int i = 0; i < num_infos; ++i) {
    assert(mis[i].low_gain <= 0);
    assert(mis[i].high_gain <= 0);
    if (mis[i].coords.x == 0 && mis[i].coords.y == 3) {
      left_found = true;
    }
    if (mis[i].coords.x == 1 && mis[i].coords.y == 3) {
      right_found = true;
    }
  }
  #ifndef NORMALIZE_EXTERNAL_LIBERTIES
    assert(left_found);
    assert(!right_found);
  #endif

  unload_dual_graph_reader(&dgr);

  free(buffer);
}

int main() {
  test_bulky_five();
  test_external_liberties();
  return 0;
}
