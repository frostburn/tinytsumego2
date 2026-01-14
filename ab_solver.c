#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "jkiss/jkiss.h"
#include "jkiss/util.h"
#include "tinytsumego2/scoring.h"

#include "tsumego.c"

// #define DEBUG
// #define DEBUG_SEARCH

#define MAX_TAIL_SIZE (8192)

struct child {
  state state;
  move_result move_result;
  int heuristic_penalty;
};

// Node in the game graph
typedef struct node {
  state state;
  int depth;
  float low;
  float high;
  bool low_fixed;
  bool high_fixed;
  int generation;
} node;

int compare_children(const void *a_, const void *b_) {
  struct child *a = (struct child*) a_;
  struct child *b = (struct child*) b_;
  return a->heuristic_penalty - b->heuristic_penalty;
}

// TODO: Fix "Rectangle Eight in the Corner"
// TODO: Fix "Rectangle Eight in the Corner (defender has threats)"
int solve(tsumego t, bool verbose) {
  state root = t.state;
  if (verbose) {
    print_state(&root);
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

  int generation = 0;

  size_t num_nodes = 1;
  size_t nodes_capacity = 1;
  size_t num_sorted = 1;
  node *nodes = malloc(sizeof(node));
  nodes[0] = (node) {root, 0, -INFINITY, INFINITY, false, false, generation};

  bool graph_updated = false;
  bool fix_loops = false;

  // TODO: Just use node pointers and return NULL on failure
  size_t find_index(state *s) {
    // Heavy abuse of struct state vs. struct node
    void *existing = bsearch((void *)s, (void*)nodes, num_sorted, sizeof(node), compare);
    if (existing) {
      return ((node*)existing) - nodes;
    }
    for (size_t i = num_sorted; i < num_nodes; ++i) {
      if (compare((void *)s, (void *)(nodes + i)) == 0) {
        return i;
      }
    }
    return SIZE_MAX;
  }

  void improve_bound(size_t index, bool lower) {
    float old_low = nodes[index].low;
    float old_high = nodes[index].high;
    #ifdef DEBUG_SEARCH
      printf(
        "Improving %s bound %f%s for %zu\n",
        lower ? "lower" : "upper",
        lower ? old_low : old_high, (lower ? nodes[index].low_fixed : nodes[index].high_fixed) ? "*" : "",
        index
    );
    #endif
    float low = -INFINITY;
    float high = -INFINITY;

    bool low_fixed = true;
    bool evaluate_more = true;

    struct child *children = malloc(num_moves * sizeof(struct child));
    state parent = nodes[index].state;
    int num_player_immortal = popcount(parent.player & parent.immortal);
    int num_opponent_immortal = popcount(parent.opponent & parent.immortal);
    stones_t empty = parent.visual_area & ~(parent.player | parent.opponent);
    for (int i = 0; i < num_moves; ++i) {
      children[i].state = nodes[index].state;
      children[i].move_result = make_move((state*)(children + i), moves[i]);
      children[i].heuristic_penalty = 0;
      if (children[i].move_result == ILLEGAL) {
        children[i].heuristic_penalty += 100000;
      }
      else if (children[i].move_result == SECOND_PASS) {
        children[i].heuristic_penalty -= 10000;
      } else if (children[i].move_result == TAKE_TARGET) {
        children[i].heuristic_penalty -= 100000;
      } else {
        move_result benson_result = apply_benson((state*)(children + i));
        if (benson_result == TAKE_TARGET) {
          children[i].move_result = benson_result;
          children[i].heuristic_penalty -= 100000;
        } else if (benson_result == TARGET_LOST) {
          children[i].move_result = benson_result;
          children[i].heuristic_penalty += 100000;
        }
        children[i].heuristic_penalty = jrand() % 1000 - popcount(cross(moves[i]) & empty) * 4000;
        if (popcount(children[i].state.opponent & children[i].state.immortal) > num_player_immortal) {
          children[i].heuristic_penalty -= 20000;
        }
        if (popcount(children[i].state.player & children[i].state.immortal) > num_opponent_immortal) {
          children[i].heuristic_penalty += 30000;
        }
      }
    }
    qsort((void*) children, num_moves, sizeof(struct child), compare_children);

    size_t my_sorted = num_sorted;
    nodes[index].generation = generation;

    for (int i = 0; i < num_moves; ++i) {
      state child = children[i].state;
      move_result r = children[i].move_result;
      if (r == SECOND_PASS) {
        float child_score = score(&child);
        if (nodes[index].high == -child_score && !nodes[index].high_fixed) {
          nodes[index].high_fixed = true;
          graph_updated = true;
          if (!lower) {
            evaluate_more = false;
          }
          #ifdef DEBUG_SEARCH
            printf("Upper bound %f fixed for %zu due to second pass\n", nodes[index].high, index);
          #endif
        }
        low = fmax(low, -child_score);
        high = fmax(high, -child_score);
      }
      else if (r == TAKE_TARGET || r == TARGET_LOST) {
        float child_score = target_lost_score(&child);
        if (r == TARGET_LOST) {
          child_score = -child_score;
        }
        if (nodes[index].high == -child_score && !nodes[index].high_fixed) {
          nodes[index].high_fixed = true;
          graph_updated = true;
          if (!lower) {
            evaluate_more = false;
          }
          #ifdef DEBUG_SEARCH
            printf("Upper bound %f fixed for %zu due to target capture\n", nodes[index].high, index);
          #endif
        }
        low = fmax(low, -child_score);
        high = fmax(high, -child_score);
      } else if (r != ILLEGAL) {
        size_t j = find_index(&child);
        if (j < SIZE_MAX) {
          if (nodes[j].depth < nodes[index].depth && nodes[j].generation == generation) {
            // Loop detected
            if (fix_loops) {
              if (nodes[index].low == -nodes[j].high && !nodes[j].high_fixed) {
                nodes[j].high_fixed = true;
                graph_updated = true;
                #ifdef DEBUG_SEARCH
                  printf("Tentatively fixing upper bound %f for loop node %zu by %zu\n", nodes[j].high, j, index);
                #endif
              }
              if (nodes[index].high == -nodes[j].low && !nodes[j].low_fixed) {
                nodes[j].low_fixed = true;
                graph_updated = true;
                #ifdef DEBUG_SEARCH
                  printf("Tentatively fixing lower bound %f for loop node %zu by %zu\n", nodes[j].low, j, index);
                #endif
              }
            }

            if (!nodes[j].high_fixed && -nodes[j].low > nodes[index].low) {
              #ifdef DEBUG_SEARCH
                printf("Not fixing low for %zu due to loop node %zu\n", index, j);
              #endif
              low_fixed = false;
            }
            if (nodes[j].low_fixed && nodes[index].high == -nodes[j].low && !nodes[index].high_fixed) {
              nodes[index].high_fixed = true;
              graph_updated = true;
              if (!lower) {
                evaluate_more = false;
              }
              #ifdef DEBUG_SEARCH
                printf("Upper bound %f fixed for %zu due to super-ko\n", nodes[index].high, index);
              #endif
            }
            // Break out of graph loops
            low = fmax(low, -nodes[j].high);
            high = fmax(high, -nodes[j].low);
            continue;
          }
          if (nodes[j].depth > nodes[index].depth) {
            nodes[j].depth = nodes[index].depth + 1;
          }
          if (lower) {
            // Any improvement raises lower bound. Evaluate only if there's potential and no improvement has been found yet.
            if (!nodes[j].high_fixed && evaluate_more) {
              improve_bound(j, false);
              if (my_sorted != num_sorted) {
                index = find_index(&parent);
                j = find_index(&child);
                my_sorted = num_sorted;
              }
            }
          } else if (nodes[index].high == -nodes[j].low && evaluate_more) {
            // Must reduce the most offensive node(s). Discriminate early.
            improve_bound(j, true);
            if (my_sorted != num_sorted) {
              index = find_index(&parent);
              j = find_index(&child);
              my_sorted = num_sorted;
            }
            if (nodes[j].low_fixed && nodes[index].high == -nodes[j].low) {
              if (!nodes[index].high_fixed) {
                graph_updated = true;
              }
              nodes[index].high_fixed = true;
              evaluate_more = false;
              #ifdef DEBUG_SEARCH
                printf("Upper bound %f fixed for %zu\n", nodes[index].high, index);
              #endif
            }
          }
          if (!nodes[j].high_fixed && -nodes[j].low > nodes[index].low) {
            #ifdef DEBUG_SEARCH
              printf("Not fixing low for %zu due to %zu\n", index, j);
            #endif
            low_fixed = false;
          }
          low = fmax(low, -nodes[j].high);
          high = fmax(high, -nodes[j].low);
        } else {
          if (evaluate_more) {
            if (num_nodes >= nodes_capacity) {
              num_sorted = nodes_capacity;
              qsort((void*) nodes, num_sorted, sizeof(node), compare);
              nodes_capacity <<= 1;
              nodes = realloc(nodes, nodes_capacity * sizeof(node));
              #ifdef DEBUG
                printf("Capacity increased to %zu\n", nodes_capacity);
              #endif
              index = find_index(&parent);
              my_sorted = num_sorted;
            } else if (num_nodes > num_sorted + MAX_TAIL_SIZE) {
              num_sorted = num_nodes;
              qsort((void*) nodes, num_sorted, sizeof(node), compare);
              index = find_index(&parent);
              my_sorted = num_sorted;
            }
            size_t j = num_nodes++;
            nodes[j] = (node) {child, nodes[index].depth + 1, -INFINITY, INFINITY, false, false, generation};
            graph_updated = true;
            improve_bound(j, !lower);
            if (my_sorted != num_sorted) {
              index = find_index(&parent);
              j = find_index(&child);
              my_sorted = num_sorted;
            }
            if (!nodes[j].high_fixed && -nodes[j].low > nodes[index].low) {
              #ifdef DEBUG_SEARCH
                printf("Not fixing low for %zu due to new node %zu\n", index, j);
              #endif
              low_fixed = false;
            }
            low = fmax(low, -nodes[j].high);
            high = fmax(high, -nodes[j].low);
          } else {
            #ifdef DEBUG_SEARCH
              printf("Not fixing low for %zu due to an unevaluated node\n", index);
            #endif
            low_fixed = false;
            high = INFINITY;
          }
        }
      }
      if (lower && (low > old_low)) {
        #ifdef DEBUG_SEARCH
          if (evaluate_more) {
            printf("Canceling further exploration for %zu because %f > %f\n", index, low, old_low);
          }
        #endif
        evaluate_more = false;
      }
    }
    nodes[index].low = low;
    nodes[index].high = high;
    if (low == high) {
      if (!nodes[index].low_fixed || !nodes[index].high_fixed) {
        graph_updated = true;
      }
      nodes[index].low_fixed = true;
      nodes[index].high_fixed = true;
      #ifdef DEBUG_SEARCH
        printf("Score %f fixed for %zu\n", low, index);
      #endif
    }

    if (!nodes[index].low_fixed && low_fixed) {
      nodes[index].low_fixed = true;
      graph_updated = true;
      #ifdef DEBUG_SEARCH
        printf("Lower bound %f fixed for %zu\n", low, index);
      #endif
    }

    #ifdef DEBUG_SEARCH
      if (old_low != nodes[index].low || old_high != nodes[index].high) {
        printf("Improved bounds to %f, %f from %f, %f for %zu\n", nodes[index].low, nodes[index].high, old_low, old_high, index);
      } else {
        printf("Failed to improve bounds %f, %f for %zu\n", nodes[index].low, nodes[index].high, index);
      }
    #endif

    if (old_low != low || old_high != high) {
      graph_updated = true;
    }
    free(children);
  }

  size_t root_index = 0;
  while (!nodes[root_index].low_fixed || !nodes[root_index].high_fixed) {
    graph_updated = false;
    #ifdef DEBUG_SEARCH
      printf("\nImproving root lower bound\n");
    #endif
    generation++;
    improve_bound(root_index, true);
    root_index = find_index(&root);
    #ifdef DEBUG_SEARCH
      printf("\nImproving root upper bound\n");
    #endif
    generation++;
    improve_bound(root_index, false);
    root_index = find_index(&root);
    if (!graph_updated) {
      if (fix_loops) {
        break;
      }
      #ifdef DEBUG_SEARCH
        printf("Failed to update graph. Fixing loops\n");
      #endif
      fix_loops = true;
    }
  }

  if (verbose) {
    printf("%zu nodes expanded\n", num_nodes);

    printf("%f%s, %f%s\n", nodes[root_index].low, nodes[root_index].low_fixed ? "*" : "", nodes[root_index].high, nodes[root_index].high_fixed ? "*" : "");
  }

  #ifdef DEBUG_SEARCH
  for (int j = 1; j < 10; ++j) {
    printf("\nDepth %d\n", j);
    for (size_t i = 0; i < num_nodes; ++i) {
      if (nodes[i].depth == j) {
        printf(" %zu: %f%s, %f%s -> ", i, nodes[i].low, nodes[i].low_fixed ? "*" : "", nodes[i].high, nodes[i].high_fixed ? "*" : "");
        for (int k = 0; k < num_moves; ++k) {
          state child = nodes[i].state;
          move_result r = make_move(&child, moves[k]);
          // TODO: Apply Benson's
          if (r == SECOND_PASS) {
            printf("sp = %f, ", score(&child));
          } else if (r == TAKE_TARGET) {
            printf("tt = %f, ", target_lost_score(&child));
          } else if (r != ILLEGAL) {
            for (size_t l = 0; l < num_nodes; ++l) {
              if (equals(&child, &(nodes[l].state))) {
                printf("%zu, ", l);
              }
            }
          }
        }
        printf("\n");
      }
    }
  }
  #endif

  float low = nodes[root_index].low;
  float high = nodes[root_index].high;

  free(moves);
  free(nodes);

  assert(low == t.low);
  assert(high == t.high);

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  jkiss_init();
  if (argc <= 1) {
    for (size_t i = 0; i < NUM_TSUMEGO; ++i) {
      printf("%s\n", TSUMEGO_NAMES[i]);
      solve(get_tsumego(TSUMEGO_NAMES[i]), true);
    }
    return EXIT_SUCCESS;
  } else {
    return solve(get_tsumego(argv[1]), true);
  }
}
