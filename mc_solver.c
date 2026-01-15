#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "jkiss/jkiss.h"
#include "jkiss/util.h"
#include "tinytsumego2/scoring.h"

#include "tsumego.c"

#define NUM_PLAYOUTS (100)

#define EPSILON (1e-4f)

#define MAX_TAIL_SIZE (8192)

// TODO: Count visits for exploration vs. exploitation?
typedef struct node {
  state state;

  // Odds of winning
  double odds;

  // Distance from root node
  int depth;

  // Last time this node was touched
  int generation;
} node;

int solve(tsumego t, bool low_komi, bool verbose) {

  state root = t.state;
  float komi = t.low;

  if (low_komi) {
    komi -= EPSILON;
  } else {
    komi += EPSILON;
  }

  int num_moves = popcount(root.logical_area) + 1;
  stones_t* moves = malloc(num_moves * sizeof(stones_t));

  int m = 0;
  for (int i = 0; i < 64; ++i) {
    const stones_t p = 1ULL << i;
    if (root.logical_area & p) {
      moves[m++] = p;
    }
  }
  moves[m] = pass();

  size_t num_nodes = 1;
  size_t nodes_capacity = 1;
  size_t num_sorted = 1;
  node *nodes = malloc(sizeof(node) * nodes_capacity);

  node* find_node(state *s) {
    // Heavy abuse of struct state vs. struct node
    void *existing = bsearch((void *)s, (void*)nodes, num_sorted, sizeof(node), compare);
    if (existing) {
      return (node*)existing;
    }
    for (size_t i = num_sorted; i < num_nodes; ++i) {
      if (compare((void *)s, (void *)(nodes + i)) == 0) {
        return nodes + i;
      }
    }
    return NULL;
  }

  unsigned int entropy_buffer = num_moves * num_moves;
  entropy_buffer *= entropy_buffer;

  // Play random moves until game over and return the score.
  float playout(state s) {
    unsigned int i = 1234567;
    unsigned int entropy = 0;
    float sign = -1;
    for (;;) {
      if (entropy < entropy_buffer) {
        i = jrand();
        entropy = UINT_MAX;
      }
      state child = s;
      move_result r = make_move(&child, moves[i % num_moves]);
      i /= num_moves;
      entropy /= num_moves;
      if (r == SECOND_PASS) {
        return sign * score(&child);
      } else if (r == TAKE_TARGET) {
        return sign * target_lost_score(&child);
      } else if (r != ILLEGAL) {
        s = child;
        sign = -sign;
      }
    }
  }

  int generation = 0;

  double improve_odds(node *n) {
    n->generation = generation;
    state s = n->state;
    float my_komi = s.white_to_play == root.white_to_play ? komi : -komi;
    double loss_odds = 1;
    int num_legal = 0;

    node *most_promising = NULL;
    node *runner_up = NULL;
    state mp_state = s;
    state ru_state = s;
    mp_state.visual_area = 0;
    ru_state.visual_area = 0;

    for (int i = 0; i < num_moves; ++i) {
      state child = s;
      move_result r = make_move(&child, moves[i]);
      move_result br = apply_benson(&child);
      if (r == SECOND_PASS) {
        if (-score(&child) > my_komi) {
          n->odds = 1;
          return 1;
        }
      } else if (r == TAKE_TARGET || br == TAKE_TARGET) {
        if (-target_lost_score(&child) > my_komi) {
          n->odds = 1;
          return 1;
        }
      } else if (br == TARGET_LOST) {
        if (target_lost_score(&child) > my_komi) {
          n->odds = 1;
          return 1;
        }
      } else if (r != ILLEGAL) {
        num_legal++;
        node *child_node = find_node(&child);
        if (child_node) {
          if (child_node->depth > n->depth) {
            child_node->depth = n->depth + 1;
          } else if (child_node->generation == n->generation) {
            // We're at a loop node that has been explored this generation
            loss_odds *= child_node->odds;
            // Bail out
            continue;
          }
          // Choose two nodes to explore further
          if (!most_promising) {
            most_promising = child_node;
          } else {
            if (child_node->odds < most_promising->odds) {
              if (runner_up) {
                loss_odds *= runner_up->odds;
              }
              runner_up = most_promising;
              most_promising = child_node;
            } else {
              if (runner_up) {
                if (child_node->odds < runner_up->odds) {
                  loss_odds *= runner_up->odds;
                  runner_up = child_node;
                } else {
                  loss_odds *= child_node->odds;
                }
              } else {
                runner_up = child_node;
              }
            }
          }
        } else {
          int num_wins = 1;
          for (int i = 0; i < NUM_PLAYOUTS; ++i) {
            if (playout(child) > -my_komi) {
              num_wins++;
            }
          }
          double odds = num_wins / (double) (NUM_PLAYOUTS + 2);
          if (num_nodes >= nodes_capacity) {
            if (most_promising) {
              mp_state = most_promising->state;
            }
            if (runner_up) {
              ru_state = runner_up->state;
            }
            num_sorted = nodes_capacity;
            qsort((void*) nodes, num_sorted, sizeof(node), compare);
            nodes_capacity <<= 1;
            nodes = realloc(nodes, nodes_capacity * sizeof(node));
            #ifdef DEBUG
              printf("Capacity increased to %zu\n", nodes_capacity);
            #endif
            n = find_node(&s);
            if (most_promising) {
              most_promising = find_node(&mp_state);
            }
            if (runner_up) {
              runner_up = find_node(&ru_state);
            }
          } else if (num_nodes > num_sorted + MAX_TAIL_SIZE) {
            if (most_promising) {
              mp_state = most_promising->state;
            }
            if (runner_up) {
              ru_state = runner_up->state;
            }
            num_sorted = num_nodes;
            qsort((void*) nodes, num_sorted, sizeof(node), compare);
            n = find_node(&s);
            if (most_promising) {
              most_promising = find_node(&mp_state);
            }
            if (runner_up) {
              runner_up = find_node(&ru_state);
            }
          }
          nodes[num_nodes++] = (node){child, odds, n->depth + 1, generation};
          loss_odds *= odds;
        }
      }
    }

    size_t my_sorted = num_sorted;
    if (runner_up) {
      ru_state = runner_up->state;
    }
    if (most_promising) {
      loss_odds *= improve_odds(most_promising);
    }
    if (runner_up) {
      if (my_sorted != num_sorted) {
        runner_up = find_node(&ru_state);
      }
      loss_odds *= improve_odds(runner_up);
    }

    // Ad hoc curve to model playout correlation
    loss_odds = pow(loss_odds, 1 / (fmin(num_legal, 3) + 0.5));

    if (my_sorted != num_sorted) {
      n = find_node(&s);
    }
    n->odds = 1 - loss_odds;

    return n->odds;
  }

  int num_wins = 1;
  for (int i = 0; i < NUM_PLAYOUTS; ++i) {
    if (playout(root) > komi) {
      num_wins++;
    }
  }

  double odds = num_wins / (double) (NUM_PLAYOUTS + 2);

  nodes[0] = (node){root, odds, 0, generation};

  print_state(&root);
  printf("Starting win rate: %g %%\n", odds * 100);

  double last_odds = NAN;

  while(generation < 10000) {
    double odds = improve_odds(find_node(&root));
    generation++;
    if (verbose) {
      printf("Generation %i win rate: %g %%\n", generation, odds * 100);
    }
    if (odds == 0 || odds == 1 || (!low_komi && last_odds == odds)) {
      last_odds = odds;
      break;
    }
    last_odds = odds;
  }

  printf("Final win rate: %g %%\n", last_odds * 100);
  printf("%zu nodes explored\n", num_nodes);

  free(moves);
  free(nodes);

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  jkiss_init();
  if (argc <= 1) {
    for (size_t i = 0; i < NUM_TSUMEGO; ++i) {
      printf("%s\n", TSUMEGO_NAMES[i]);
      solve(get_tsumego(TSUMEGO_NAMES[i]), true, false);
      solve(get_tsumego(TSUMEGO_NAMES[i]), false, false);
    }
    return EXIT_SUCCESS;
  } else {
    return solve(get_tsumego(argv[1]), argc >= 3, true);
  }
}
