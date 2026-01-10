#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "tinytsumego2/scoring.h"

#include "tsumego.c"

#define DEBUG_SEARCH

// Node in the game graph
typedef struct node {
  state state;
  int depth;
  float low;
  float high;
  bool low_fixed;
  bool high_fixed;
} node;

int main() {
  state root = get_tsumego("Rectangle Eight (defender has threats)");

  print_state(&root);

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
  node *nodes = malloc(sizeof(node));
  nodes[0] = (node) {root, 0, -INFINITY, INFINITY, false, false};

  bool graph_updated = false;

  void improve_bound(size_t index, bool lower) {
    float old_low = nodes[index].low;
    float old_high = nodes[index].high;
    #ifdef DEBUG_SEARCH
      printf("Improving %s bound %f for %zu\n", lower ? "lower" : "upper", lower ? old_low : old_high, index);
    #endif
    float low = -INFINITY;
    float high = -INFINITY;

    bool low_fixed = true;
    bool evaluate_more = true;

    for (int i = 0; i < num_moves; ++i) {
      state child = nodes[index].state;
      move_result r = make_move(&child, moves[i]);
      if (r == SECOND_PASS) {
        float child_score = score(&child);
        low = fmax(low, -child_score);
        high = fmax(high, -child_score);
      }
      else if (r == TAKE_TARGET) {
        float child_score = target_lost_score(&child);
        low = fmax(low, -child_score);
        high = fmax(high, -child_score);
      } else if (r != ILLEGAL) {
        bool found = false;
        for (size_t j = 0; j < num_nodes; ++j) {
          if (equals(&child, &(nodes[j].state))) {
            found = true;
            if (nodes[j].depth < nodes[index].depth) {
              if (!nodes[j].high_fixed) {
                low_fixed = false;
              }
              low = fmax(low, -nodes[j].high);
              high = fmax(high, -nodes[j].low);
              // Break out of graph loops
              break;
            }
            if (nodes[j].depth > nodes[index].depth) {
              nodes[j].depth = nodes[index].depth + 1;
            }
            if (lower) {
              // Any improvement raises lower bound. Evaluate only if there's potential and no improvement has been found yet.
              if (!nodes[j].high_fixed && evaluate_more) {
                improve_bound(j, false);
              }
            } else if (nodes[index].high == -nodes[j].low && evaluate_more) {
              // Must reduce the most offensive node(s). Discriminate early.
              improve_bound(j, true);
              if (nodes[j].low_fixed) {
                nodes[index].high_fixed = true;
                evaluate_more = false;
                #ifdef DEBUG_SEARCH
                  printf("Upper bound %f fixed for %zu\n", nodes[index].high, index);
                #endif
              }
            }
            if (!nodes[j].high_fixed) {
              low_fixed = false;
            }
            low = fmax(low, -nodes[j].high);
            high = fmax(high, -nodes[j].low);
            break;
          }
        }
        if (!found) {
          if (num_nodes >= nodes_capacity) {
            nodes_capacity <<= 1;
            nodes = realloc(nodes, nodes_capacity * sizeof(node));
            #ifdef DEBUG
              printf("Capacity increased to %zu\n", nodes_capacity);
            #endif
          }
          size_t j = num_nodes++;
          nodes[j] = (node) {child, nodes[index].depth + 1, -INFINITY, INFINITY, false, false};
          if (evaluate_more) {
            improve_bound(j, !lower);
          }
          if (!nodes[j].high_fixed) {
            low_fixed = false;
          }
          low = fmax(low, -nodes[j].high);
          high = fmax(high, -nodes[j].low);
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
      nodes[index].low_fixed = true;
      nodes[index].high_fixed = true;
      #ifdef DEBUG_SEARCH
        printf("Score %f fixed for %zu\n", low, index);
      #endif
    }

    if (!nodes[index].low_fixed && low_fixed) {
      nodes[index].low_fixed = true;
      #ifdef DEBUG_SEARCH
        printf("Lower bound %f fixed for %zu\n", low, index);
      #endif
    }

    #ifdef DEBUG_SEARCH
      printf("Improved bounds to %f, %f from %f, %f for %zu\n", nodes[index].low, nodes[index].high, old_low, old_high, index);
    #endif

    if (old_low != low || old_high != high) {
      graph_updated = true;
    }
  }

  while (!nodes[0].low_fixed || !nodes[0].high_fixed) {
    graph_updated = false;
    #ifdef DEBUG_SEARCH
      printf("\nImproving root lower bound\n");
    #endif
    improve_bound(0, true);
    #ifdef DEBUG_SEARCH
      printf("\nImproving root upper bound\n");
    #endif
    improve_bound(0, false);
    // TODO: Actually figure out how to fix loops
    if (!graph_updated) {
      break;
    }
  }

  printf("%zu nodes expanded\n", num_nodes);

  printf("%f, %f\n", nodes[0].low, nodes[0].high);

  #ifdef DEBUG_SEARCH
    printf("Depth 1\n");
    for (size_t i = 0; i < num_nodes; ++i) {
      if (nodes[i].depth == 1) {
        printf(" %zu: %f, %f\n", i, nodes[i].low, nodes[i].high);
      }
    }

    printf("\nDepth 2\n");
    for (size_t i = 0; i < num_nodes; ++i) {
      if (nodes[i].depth == 2) {
        printf(" %zu: %f, %f\n", i, nodes[i].low, nodes[i].high);
      }
    }
  #endif

  free(moves);
  free(nodes);
  return EXIT_SUCCESS;
}

/*
  // Array of offsets for backtracking (cannot use pointers due to reallocs)
  size_t history_size = 1;
  size_t history[MAX_DEPTH];
  history[0] = 0;

  bool almost_done = false;

  // First obtain a lower bound for the score
  bool search_low = true;

  float old_low, old_high;
  float alpha, beta;
  search_start:
  // Keep track of the global objective. Any improvement breaks us out.
  alpha = nodes[0].low;
  beta = nodes[0].high;

  // Non-flipping bounds
  old_low = nodes[0].low;
  old_high = nodes[0].high;

  // for (int n = 0; n < 10000; n++) {
  for (;;) {
    node *current = nodes + history[history_size - 1];
    // print_state(&(current->state));
    // printf("%d: %f, %f\n", current->depth, current->low, current->high);

    #ifdef DEBUG_SEARCH
      printf("Graph:\n");
      for (size_t i = 0; i < num_nodes; ++i) {
        printf(" %zu: %f, %f (%d%s)\n", i, nodes[i].low, nodes[i].high, nodes[i].depth, nodes[i].terminal ? "*" : "");
      }
    #endif

    node *next = NULL;
    if (current->low == current->high) {
      goto navigation;
    }
    // Perform negamax
    float low = -INFINITY;
    float high = -INFINITY;
    for (int j = 0; j < num_moves; ++j) {
      state child = current->state;
      const move_result r = make_move(&child, moves[j]);
      if (r == SECOND_PASS) {
        float child_score = score(&child);
        low = fmax(low, -child_score);
        high = fmax(high, -child_score);
      }
      else if (r == TAKE_TARGET) {
        float child_score = target_lost_score(&child);
        low = fmax(low, -child_score);
        high = fmax(high, -child_score);
      } else if (r != ILLEGAL) {
        bool found = false;
        for (size_t i = 0; i < num_nodes; ++i) {
          if (equals(&child, &(nodes[i].state))) {
            if (nodes[i].depth > current->depth) {
              nodes[i].depth = current->depth + 1;
            }
            found = true;

            // TODO: Figure out capture delay with alpha/beta
            // float my_child_low = -delay_capture(nodes[i].high);
            // float my_child_high = -delay_capture(nodes[i].low);
            float my_child_low = -nodes[i].high;
            float my_child_high = -nodes[i].low;
            low = fmax(low, my_child_low);
            high = fmax(high, my_child_high);

            if (!next && !nodes[i].terminal && (nodes[i].depth > current->depth))  {
              if (search_low) {
                // To improve the lower bound search anything with potential
                if (current->low < my_child_high) {
                  next = nodes + i;
                  #ifdef DEBUG_SEARCH
                    printf("Search low: %zu\n", i);
                  #endif
                }
              } else {
                // To improve the upper bound search a node that causes the upper bound to be so high
                if (current->high == my_child_high) {
                  next = nodes + i;
                  #ifdef DEBUG_SEARCH
                    printf("Search high: %zu\n", i);
                  #endif
                }
              }
            }
            break;
          }
        }
        if (!found) {
          if (num_nodes >= nodes_capacity) {
            nodes_capacity <<= 1;
            nodes = realloc(nodes, nodes_capacity * sizeof(node));
            #ifdef DEBUG_SEARCH
              printf("Capacity increased to %zu\n", nodes_capacity);
            #endif
            // Re-locate current node in the reallocated nodes
            current = nodes + history[history_size - 1];
          }
          nodes[num_nodes++] = (node) {child, current->depth + 1, -INFINITY, INFINITY, false};
          // Low negamax skipped: Nothing to do
          high = INFINITY;  // An unexplored node has infinite potential
          if (!next) {
            next = nodes + (num_nodes - 1);
            #ifdef DEBUG_SEARCH
              printf("Heading to new node %zu\n", num_nodes - 1);
            #endif
          }
        }
      }
    }
    current->low = low;
    current->high = high;

    navigation:
    size_t old = history_size;

    if (!next) {
      history_size--;
      if (!history_size) {
        break;
      }
      current->terminal = true;
      #ifdef DEBUG_SEARCH
        printf("Backtracking to %zu due to terminal node %zu\n", history[history_size - 1], current - nodes);
      #endif
    } else {
      if (search_low) {
        if (current->low > alpha) {
          history_size--;
          if (!history_size) {
            break;
          }
          #ifdef DEBUG_SEARCH
            printf("Backtracking to %zu due to alpha break %f > %f\n", history[history_size - 1], current->low, alpha);
          #endif
        }
      } else {
        if (current->high < beta) {
          history_size--;
          if (!history_size) {
            break;
          }
          printf("Backtracking to %zu due to beta break %f < %f\n", history[history_size - 1], current->high, beta);
        }
      }
    }

    if (old == history_size) {
      history[history_size++] = next - nodes;
      if (history_size >= MAX_DEPTH) {
        fprintf(stderr, "Maximum search depth exceeded\n");
        return EXIT_FAILURE;
      }
    }
    search_low = !search_low;

    float temp = alpha;
    alpha = -beta;
    beta = -temp;

    #ifdef DEBUG_SEARCH
      printf("History size = %zu\n", history_size);
      for (size_t i = 0; i < history_size; ++i) {
        printf("%zu, ", history[i]);
      }
      printf("\n");
    #endif
  }

  if (nodes[0].low == old_low && nodes[0].high == old_high) {
    if (almost_done) {
      goto cleanup;
    } else {
      printf("Bounds stayed at: %f, %f\n", nodes[0].low, nodes[0].high);
      almost_done = true;
    }
  } else {
    printf("Bounds improved to: %f, %f\n", nodes[0].low, nodes[0].high);
  }
  for (size_t i = 0; i < num_nodes; ++i) {
    nodes[i].terminal = false;
  }

  search_low = !search_low;
  history_size = 1;
  history[0] = 0;
  goto search_start;

  cleanup:

  printf("Done: %f, %f\n", nodes[0].low, nodes[0].high);

  free(moves);
  free(nodes);

  return EXIT_SUCCESS;
}

*/
