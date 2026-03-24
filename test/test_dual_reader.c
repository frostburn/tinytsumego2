#include "tinytsumego2/dual_reader.h"
#include "tinytsumego2/dual_solver.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/state.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

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
  while (iterate_dual_graph(&dg, false))
    ;
  while (area_iterate_dual_graph(&dg, true))
    ;

  size_t value_map_size = 0;
  frozen_hash_table fht = prepare_frozen_hash(&dg, &value_map_size);
  printf("%zu unique value quads in the graph\n", value_map_size);
  assert(value_map_size == 16);

  char *buffer = malloc(MEM_FILE_SIZE);
  FILE *stream = fmemopen(buffer, MEM_FILE_SIZE, "wb");
  write_dual_graph(&dg, &fht, stream);
  fclose(stream);
  free(fht.bulk_map);
  free_dual_graph(&dg);

  dual_graph_reader dgr = {0};
  dgr.fd = -1;
  dgr.buffer = buffer;
  unbuffer_dual_graph_reader(&dgr);

  for (size_t i = 0; i < dgr.value_table.bulk_map_size; ++i) {
    dual_table_value tv = dgr.value_table.bulk_map[i];
    dual_value v = (dual_value){
        {score_q7_to_float(tv.plain.low), score_q7_to_float(tv.plain.high)},
        {score_q7_to_float(tv.forcing.low), score_q7_to_float(tv.forcing.high)},
    };
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
  while (iterate_dual_graph(&dg, false))
    ;
  while (area_iterate_dual_graph(&dg, true))
    ;

  size_t value_map_size = 0;
  frozen_hash_table fht = prepare_frozen_hash(&dg, &value_map_size);
  printf("%zu unique value quads in the graph\n", value_map_size);
  assert(value_map_size == 31);

  char *buffer = malloc(MEM_FILE_SIZE);
  FILE *stream = fmemopen(buffer, MEM_FILE_SIZE, "wb");
  write_dual_graph(&dg, &fht, stream);
  fclose(stream);
  free(fht.bulk_map);
  free_dual_graph(&dg);

  dual_graph_reader dgr = {0};
  dgr.fd = -1;
  dgr.buffer = buffer;
  unbuffer_dual_graph_reader(&dgr);

  for (size_t i = 0; i < dgr.value_table.bulk_map_size; ++i) {
    dual_table_value tv = dgr.value_table.bulk_map[i];
    dual_value v = (dual_value){
        {score_q7_to_float(tv.plain.low), score_q7_to_float(tv.plain.high)},
        {score_q7_to_float(tv.forcing.low), score_q7_to_float(tv.forcing.high)},
    };
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

void test_frozen_hash_table() {
  // There's no natural way to construct frozen hash tables so we mock the game graph and disk round-trip
  dual_graph dg = {0};
  dg.type = MOCK_KEYSPACE;

  const size_t num_common = 20000;
  const size_t num_uncommon = 40000;
  const size_t num_rare = 20000;

  assert(num_common + num_uncommon + num_rare > VALUE_MAP_SIZE);

  dg.keyspace._.size = 3 * num_common + 2 * num_uncommon + num_rare;

  dg.plain_values = malloc(dg.keyspace._.size * sizeof(table_value));
  dg.forcing_values = malloc(dg.keyspace._.size * sizeof(table_value));

  for (size_t i = 0; i < num_rare; ++i) {
    dg.plain_values[i].low = (score_q7_t)(i + 1);
    dg.plain_values[i].high = 0;
    dg.forcing_values[i].low = 0;
    dg.forcing_values[i].high = 0;
  }

  for (size_t i = 0; i < num_uncommon; ++i) {
    size_t i0 = num_rare + i;
    size_t i1 = num_rare + num_uncommon + i;
    dg.plain_values[i0].low = dg.plain_values[i1].low = 0;
    dg.plain_values[i0].high = dg.plain_values[i1].high = (score_q7_t)(i + 1); // Intentional overflow to negative
    dg.forcing_values[i0].low = dg.forcing_values[i1].low = 0;
    dg.forcing_values[i0].high = dg.forcing_values[i1].high = 0;
  }

  for (size_t i = 0; i < num_common; ++i) {
    size_t j = num_rare + 2 * num_uncommon + 3 * i;
    dg.plain_values[j + 0].low = dg.plain_values[j + 1].low = dg.plain_values[j + 2].low = 0;
    dg.plain_values[j + 0].high = dg.plain_values[j + 1].high = dg.plain_values[j + 2].high = 0;
    dg.forcing_values[j + 0].low = dg.forcing_values[j + 1].low = dg.forcing_values[j + 2].low = (score_q7_t)(i + 1);
    dg.forcing_values[j + 0].high = dg.forcing_values[j + 1].high = dg.forcing_values[j + 2].high = 0;
  }

  size_t num_unique = 0;
  frozen_hash_table fht = prepare_frozen_hash(&dg, &num_unique);

  assert(num_unique == num_common + num_uncommon + num_rare);

  assert(fht.tail_size == num_unique - (VALUE_MAP_SIZE - 1));

  // Mock bulk map
  fht.bulk_ids = malloc(dg.keyspace._.size * sizeof(value_id_t));
  for (size_t i = 0; i < dg.keyspace._.size; ++i) {
    dual_table_value v = (dual_table_value){
        dg.plain_values[i],
        dg.forcing_values[i],
    };
    dual_table_value *tv = bsearch(&v, fht.bulk_map, fht.bulk_map_size, sizeof(dual_table_value), compare_dual_table_values);
    fht.bulk_ids[i] = tv ? (value_id_t)(tv - fht.bulk_map) : VALUE_ID_SENTINEL;
  }

  // Make sure it works
  for (size_t i = 0; i < dg.keyspace._.size; ++i) {
    dual_table_value v = get_frozen_hash_value(&fht, i);
    assert(v.plain.low == dg.plain_values[i].low);
    assert(v.plain.high == dg.plain_values[i].high);
    assert(v.forcing.low == dg.forcing_values[i].low);
    assert(v.forcing.high == dg.forcing_values[i].high);
  }

  // Clear specific mocks
  free(fht.bulk_ids);
  fht.bulk_ids = NULL;

  const size_t mem_file_size = (MEM_FILE_SIZE + dg.keyspace._.size * sizeof(value_id_t) + fht.bulk_map_size * sizeof(dual_table_value) +
                                fht.tail_size * sizeof(dual_table_value) + (fht.tail_size / 2) * sizeof(size_t));

  char *buffer = malloc(mem_file_size);
  for (size_t i = 0; i < mem_file_size; ++i) {
    buffer[i] = 1;
  }
  FILE *stream = fmemopen(buffer, mem_file_size, "wb");
  write_dual_graph(&dg, &fht, stream);
  fclose(stream);

  dual_graph_reader dgr = {0};
  dgr.fd = -1;
  dgr.buffer = buffer;
  unbuffer_dual_graph_reader(&dgr);

  assert(dgr.value_table.bulk_map_size == fht.bulk_map_size);
  assert(dgr.value_table.tail_size == fht.tail_size);

  // Free generic mocks
  free(fht.bulk_map);
  free(fht.tail_keys);
  free(fht.tail_values);

  // Make sure it works after the mock round-trip
  for (size_t i = 0; i < dg.keyspace._.size; ++i) {
    dual_table_value v = get_frozen_hash_value(&(dgr.value_table), i);
    assert(v.plain.low == dg.plain_values[i].low);
    assert(v.plain.high == dg.plain_values[i].high);
    assert(v.forcing.low == dg.forcing_values[i].low);
    assert(v.forcing.high == dg.forcing_values[i].high);
  }

  // Free generic mocks
  free(dg.plain_values);
  free(dg.forcing_values);
  unload_dual_graph_reader(&dgr);

  // Free memory file
  free(buffer);
}

int main() {
  test_bulky_five();
  test_external_liberties();
  test_frozen_hash_table();
  return 0;
}
