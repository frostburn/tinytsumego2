#pragma once
#include <stdint.h>
#include <stdio.h>
#include "tinytsumego2/state.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/full_solver.h"

// TODO: Make this (4) once the basic idea is validated
#define TABLE_WIDTH (3)
#define TABLE_HEIGHT (3)

// 3**(TABLE_WIDTH * TABLE_HEIGHT)
// #define TABLEBASE_SIZE (531441UL) // 3**12
#define TABLEBASE_SIZE (19683UL)  // 3**9

#define INVALID_KEY (SIZE_MAX)

#define INVALID_SCORE_Q7 (-32768)

typedef enum table_type {
  CORNER,
  EDGE,
  CENTER,
} table_type;

// Use 16-bit values to save space
typedef struct table_value {
  score_q7_t low;
  score_q7_t high;
} table_value;

// Table containing solutions to every tsumego with a tablebase key
typedef struct tsumego_table {
  table_type type;
  int button;
  int ko_threats;
  int num_external;
  // bool wide; // TODO
  bool opponent_targetted;

  // Always has `TABLEBASE_SIZE` entries but not all valid
  table_value *values;
} tsumego_table;

// Collection of tables with various number of ko-threats and external liberties
typedef struct tablebase {
  size_t num_tables;
  tsumego_table *tables;
} tablebase;

// Convert a TABLE_WIDTH x TABLE_HEIGHT corner tsumego into an enumerated index
size_t to_corner_tablebase_key(const state *s);

// Build a game state from an enumeration of a corner tsumego
state from_corner_tablebase_key(size_t key);

// TODO: (Wide only)
state from_edge_tablebase_key(size_t key);

// TODO
state from_center_tablebase_key(size_t key);

// Get a value for a state according to the tablebase. Returns {NAN, NAN} on failure
value get_tablebase_value(const tablebase *tb, const state *s);

// Write a single tsumego table to a file
size_t write_tsumego_table(const tsumego_table *restrict tt, FILE *restrict stream);

// Write a full tablebase to a file
size_t write_tablebase(const tablebase *restrict tb, FILE *restrict stream);

// Read a single tsumego table from a file
tsumego_table read_tsumego_table(FILE *restrict stream);

// Read a full tablebase from a file
tablebase read_tablebase(FILE *restrict stream);

// Release resources associated with a tablebase
void free_tablebase(tablebase *tb);
