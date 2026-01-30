#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "tinytsumego2/full_reader.h"

size_t write_full_graph(const state *restrict root, const full_graph *restrict fg, FILE *restrict stream) {
  size_t total = fwrite(root, sizeof(state), 1, stream);
  total += fwrite(&(fg->num_moves), sizeof(int), 1, stream);
  total += fwrite(fg->moves, sizeof(stones_t), fg->num_moves, stream);
  total += fwrite(&(fg->num_nodes), sizeof(size_t), 1, stream);

  // TODO: Add the option to sort on-disk

  light_node *nodes = malloc(fg->num_nodes * sizeof(light_node));
  for (size_t i = 0; i < fg->num_nodes; ++i) {
    nodes[i].key = to_key(root, fg->states + i);
    nodes[i].low = float_to_score_q7(fg->values[i].low);
    nodes[i].high = float_to_score_q7(fg->values[i].high);
  }
  // Abuses light_node ~ size_t
  qsort((void*) nodes, fg->num_nodes, sizeof(light_node), compare_keys);
  total += fwrite(nodes, sizeof(light_node), fg->num_nodes, stream);
  free(nodes);
  return total;
}

char* file_to_mmap(const char *filename, struct stat *sb, int *fd) {
  stat(filename, sb);
  *fd = open(filename, O_RDONLY);
  assert(*fd != -1);
  char *map;
  map = (char*) mmap(NULL, sb->st_size, PROT_READ, MAP_SHARED, *fd, 0);
  madvise(map, sb->st_size, MADV_RANDOM);
  return map;
}

full_graph_reader load_full_graph_reader(const char *filename) {
  full_graph_reader result;
  char *map = file_to_mmap(filename, &(result.sb), &(result.fd));

  result.buffer = map;

  result.root = ((state*) map)[0];
  map += sizeof(state);

  result.num_moves = ((int *) map)[0];
  map += sizeof(int);

  result.moves = malloc(result.num_moves * sizeof(stones_t));
  for (int i = 0; i < result.num_moves; ++i) {
    result.moves[i] = ((stones_t *) map)[i];
  }
  map += result.num_moves * sizeof(stones_t);

  result.num_nodes = ((size_t *) map)[0];
  map += sizeof(size_t);

  result.nodes = (light_node *) map;
  map += result.num_nodes * sizeof(light_node);

  return result;
}

void unload_full_graph_reader(full_graph_reader *fgr) {
  free(fgr->moves);
  fgr->num_moves = 0;
  fgr->moves = NULL;

  munmap(fgr->buffer, fgr->sb.st_size);
  close(fgr->fd);

  fgr->buffer = NULL;
  fgr->fd = -1;

  fgr->num_nodes = 0;
  fgr->nodes = NULL;
}

value get_full_graph_reader_value(const full_graph_reader *fgr, const state *s) {
  if (s->button < 0) {
    state c = *s;
    c.button = -c.button;
    size_t key = to_key(&(fgr->root), &c);
    light_node *ln = (light_node*) bsearch((void*) &key, (void*) fgr->nodes, fgr->num_nodes, sizeof(light_node), compare_keys);
    if (!ln) {
      return (value){NAN, NAN};
    }
    return (value){
      score_q7_to_float(ln->low) - 2 * BUTTON_BONUS,
      score_q7_to_float(ln->high) - 2 * BUTTON_BONUS
    };
  }

  size_t key = to_key(&(fgr->root), s);
  light_node *ln = (light_node*) bsearch((void*) &key, (void*) fgr->nodes, fgr->num_nodes, sizeof(light_node), compare_keys);
  if (!ln) {
    return (value){NAN, NAN};
  }
  return (value){score_q7_to_float(ln->low), score_q7_to_float(ln->high)};
}
