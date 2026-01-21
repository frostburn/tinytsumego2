#include <limits.h>
#include <math.h>
#include <stdio.h>
#include "jkiss/jkiss.h"
#include "jkiss/util.h"
#include "tinytsumego2/scoring.h"
#include "tinytsumego2/bloom.h"
#include "tinytsumego2/partial_solver.h"

// #define DEBUG_SEARCH

int compare_children(const void *a_, const void *b_) {
  struct child *a = (struct child*) a_;
  struct child *b = (struct child*) b_;
  return a->heuristic_penalty - b->heuristic_penalty;
}

game_graph create_game_graph(const state *root) {
  game_graph gg = {0};

  gg.root = *root;

  gg.bloom = calloc(BLOOM_SIZE, sizeof(unsigned char));

  gg.nodes_capacity = MIN_CAPACITY;
  gg.nodes = malloc(gg.nodes_capacity * sizeof(node));

  gg.num_moves = popcount(root->logical_area) + 1;
  gg.moves = malloc(gg.num_moves * sizeof(stones_t));

  int j = 0;
  for (int i = 0; i < 64; ++i) {
    const stones_t p = 1ULL << i;
    if (root->logical_area & p) {
      gg.moves[j++] = p;
    }
  }
  gg.moves[j] = pass();

  node_proxy np = get_game_graph_node(&gg, root);
  np.depth = 0;
  update_game_graph_node(&gg, &np);

  return gg;
}

void print_game_graph(game_graph *gg) {
  for (size_t i = 0; i < gg->num_nodes; ++i) {
    node n = gg->nodes[i];
    printf(
      "#%zu @ %d: %f%s, %f%s\n",
      i,
      n.depth,
      n.low,
      n.low_fixed ? "*" : "",
      n.high,
      n.high_fixed ? "*" : ""
    );
  }
}

node_proxy get_game_graph_node(game_graph *gg, const state *s) {
  const stones_t a = hash_a(s);
  const stones_t b = hash_b(s);
  // Pre-filter using bloom
  const bool maybe_seen = bloom_test(gg->bloom, a, b);
  if (maybe_seen) {
    // Binary search the sorted head (abuses node ~ state)
    node *existing = (node*) bsearch((void*) s, (void*) gg->nodes, gg->num_sorted, sizeof(node), compare);
    if (existing) {
      return (node_proxy) {
        existing->state,
        existing->depth,
        existing->low,
        existing->high,
        existing->low_fixed,
        existing->high_fixed,
        existing->generation,
        existing->visits,
        existing->num_children,
        existing->children,
        existing - gg->nodes,
        gg->num_sorted,
      };
    }
    // Search tail linearly
    for (size_t i = gg->num_sorted; i < gg->num_nodes; ++i) {
      if (compare((void*) (gg->nodes + i), (void*) s) == 0) {
        return (node_proxy) {
          gg->nodes[i].state,
          gg->nodes[i].depth,
          gg->nodes[i].low,
          gg->nodes[i].high,
          gg->nodes[i].low_fixed,
          gg->nodes[i].high_fixed,
          gg->nodes[i].generation,
          gg->nodes[i].visits,
          gg->nodes[i].num_children,
          gg->nodes[i].children,
          i,
          gg->num_sorted,
        };
      }
    }
    // False positive hit on bloom. Carry on
  }
  if (gg->num_nodes >= gg->nodes_capacity) {
    gg->num_sorted = gg->nodes_capacity;
    qsort((void*) gg->nodes, gg->num_sorted, sizeof(node), compare);
    gg->nodes_capacity <<= 1;
    gg->nodes = realloc(gg->nodes, gg->nodes_capacity * sizeof(node));
  } else if (gg->num_nodes > gg->num_sorted + MAX_TAIL_SIZE) {
    gg->num_sorted = gg->num_nodes;
    qsort((void*) gg->nodes, gg->num_sorted, sizeof(node), compare);
  }
  gg->nodes[gg->num_nodes++] = (node) {
    *s,
    INT_MAX,
    -INFINITY,
    INFINITY,
    false,
    false,
    -1,
    0,
    0,
    NULL,
  };
  bloom_insert(gg->bloom, a, b);

  return (node_proxy) {
    *s,
    INT_MAX,
    -INFINITY,
    INFINITY,
    false,
    false,
    -1,
    0,
    0,
    NULL,
    gg->num_nodes - 1,
    gg->num_sorted,
  };
}

void update_game_graph_node(game_graph *gg, node_proxy *np) {
  if (np->tag != gg->num_sorted) {
    node *existing = (node*) bsearch((void*) &(np->state), (void*) gg->nodes, gg->num_sorted, sizeof(node), compare);
    if (!existing) {
      fprintf(stderr, "Invalid proxy detected\n");
      exit(EXIT_FAILURE);
    }
    np->index = existing - gg->nodes;
    np->tag = gg->num_sorted;
  }
  gg->nodes[np->index] = (node) {
    np->state,
    np->depth,
    np->low,
    np->high,
    np->low_fixed,
    np->high_fixed,
    np->generation,
    np->visits,
    np->num_children,
    np->children,
  };
}

bool expand_children(game_graph *gg, node_proxy *np) {
  if (np->children != NULL) {
    return false;
  }
  struct child *children = malloc(gg->num_moves * sizeof(struct child));
  state parent = np->state;
  int num_player_immortal = popcount(parent.player & parent.immortal);
  int num_opponent_immortal = popcount(parent.opponent & parent.immortal);
  stones_t empty = parent.visual_area & ~(parent.player | parent.opponent);
  np->num_children = gg->num_moves;
  for (int i = 0; i < gg->num_moves; ++i) {
    children[i].state = parent;
    children[i].move_result = make_move((state*)(children + i), gg->moves[i]);
    children[i].heuristic_penalty = 0;
    if (children[i].move_result == ILLEGAL) {
      np->num_children--;
      children[i].heuristic_penalty += 1000000;
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
      children[i].heuristic_penalty = jrand() % 1000 - popcount(cross(gg->moves[i]) & empty) * 4000;
      if (popcount(children[i].state.opponent & children[i].state.immortal) > num_player_immortal) {
        children[i].heuristic_penalty -= 20000;
      }
      if (popcount(children[i].state.player & children[i].state.immortal) > num_opponent_immortal) {
        children[i].heuristic_penalty += 30000;
      }
    }
  }
  qsort((void*) children, gg->num_moves, sizeof(struct child), compare_children);
  np->children = realloc(children, np->num_children * sizeof(struct child));
  return true;
}

void improve_bound(game_graph *gg, node_proxy *np, bool lower) {
  if (lower && np->low_fixed) {
    return;
  }
  if (!lower && np->high_fixed) {
    return;
  }

  #ifdef DEBUG_SEARCH
    printf(
      "Improving %s bound %f%s for %llx @ %d\n",
      lower ? "lower" : "upper",
      lower ? np->low : np->high, (lower ? np->low_fixed : np->high_fixed) ? "*" : "",
      hash_a(&(np->state)),
      np->depth
    );
  #endif
  float low = -INFINITY;
  float high = -INFINITY;

  bool low_fixed = true;
  bool evaluate_more = true;

  bool owns_children = expand_children(gg, np);
  np->visits++;
  np->generation = gg->generation;
  update_game_graph_node(gg, np);

  for (int i = 0; i < np->num_children; ++i) {
    const state child = np->children[i].state;
    const move_result r = np->children[i].move_result;
    if (r <= TAKE_TARGET) {
      float child_score;
      if (r == SECOND_PASS) {
        child_score = score(&child);
      } else if (r == TARGET_LOST) {
        child_score = -target_lost_score(&child);
      } else {
        child_score = target_lost_score(&child);
      }
      if (np->high == -child_score && !np->high_fixed) {
        np->high_fixed = true;
        gg->updated = true;
        if(!lower) {
          evaluate_more = false;
        }
      }
      low = fmax(low, -child_score);
      high = fmax(high, -child_score);
    } else {
      node_proxy cp = get_game_graph_node(gg, &child);
      if (cp.depth < np->depth && cp.generation == gg->generation) {
        // Loop detected
        if (gg->fix_loops) {
          if (np->low == -cp.high && !cp.high_fixed) {
            cp.high_fixed = true;
            gg->updated = true;
          }
          if (np->high == -cp.low && !cp.low_fixed) {
            cp.low_fixed = true;
            gg->updated = true;
          }
          update_game_graph_node(gg, &cp);
        }
        if (!cp.high_fixed && -cp.low > np->low) {
          low_fixed = false;
        }
        if (cp.low_fixed && np->high == -cp.low && !np->high_fixed) {
          np->high_fixed = true;
          gg->updated = true;
          if (!lower) {
            evaluate_more = false;
          }
        }
        low = fmax(low, -cp.high);
        high = fmax(high, -cp.low);
        // Break out of the graph loop
        continue;
      }
      if (cp.depth > np->depth + 1) {
        cp.depth = np->depth + 1;
        update_game_graph_node(gg, &cp);
      }
      if (evaluate_more) {
        if (lower) {
          // Any improvement increases the lower bound. Pick based on heuristics
          improve_bound(gg, &cp, false);
        } else if (np->high == -cp.low) {
          // All of the most offending child nodes need to be improved to decrease the upper bound.
          improve_bound(gg, &cp, true);
          if (cp.low_fixed && np->high == -cp.low) {
            np->high_fixed = true;
            gg->updated = true;
            evaluate_more = false;
          }
        }
      }
      if (!cp.high_fixed && -cp.low > np->low) {
        low_fixed = false;
      }
      low = fmax(low, -cp.high);
      high = fmax(high, -cp.low);
    }
    if (lower && (low > np->low)) {
      evaluate_more = false;
    }
  }

  // Child nodes can get stale when transpositions trigger re-evaluation
  for (int i = 0; i < np->num_children; ++i) {
    if (np->children[i].move_result <= TAKE_TARGET) {
      continue;
    }
    node_proxy cp = get_game_graph_node(gg, &(np->children[i].state));
    low = fmax(low, -cp.high);
    high = fmax(high, -cp.low);
  }

  if (owns_children && np->visits < MIN_VISITS) {
    np->num_children = 0;
    free(np->children);
    np->children = NULL;
  }
  if (low == high) {
    low_fixed = true;
    if (!np->high_fixed) {
      np->high_fixed = true;
      gg->updated = true;
    }
  }
  if (low_fixed != np->low_fixed || low != np->low || high != np->high) {
    gg->updated = true;
  }
  np->low = low;
  np->high = high;
  np->low_fixed = low_fixed;
  update_game_graph_node(gg, np);
}

void solve_game_graph(game_graph *gg, bool verbose) {
  node_proxy np = get_game_graph_node(gg, &(gg->root));
  while (!np.low_fixed || !np.high_fixed) {
    gg->generation++;
    gg->updated = false;
    improve_bound(gg, &np, true);
    if (verbose) {
      printf("Generation %d (low): %f%s, %f%s\n", gg->generation, np.low, np.low_fixed ? "*" : "", np.high, np.high_fixed ? "*" : "");
    }
    gg->generation++;
    gg->updated = false;
    improve_bound(gg, &np, false);
    if (verbose) {
      printf("Generation %d (high): %f%s, %f%s\n", gg->generation, np.low, np.low_fixed ? "*" : "", np.high, np.high_fixed ? "*" : "");
    }
    if (!gg->updated) {
      gg->fix_loops = true;
    }
  }
}

void free_game_graph(game_graph *gg) {
  free(gg->moves);
  gg->num_moves = 0;
  gg->moves = NULL;

  free(gg->bloom);
  gg->bloom = NULL;

  for (size_t i = 0; i < gg->num_nodes; ++i) {
    if (gg->nodes[i].children) {
      free(gg->nodes[i].children);
    }
  }

  free(gg->nodes);
  gg->num_nodes = 0;
  gg->nodes_capacity = 0;
  gg->num_sorted = 0;
}
