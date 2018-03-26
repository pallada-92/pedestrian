#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// reset && gcc calc_paths.c && time (echo "2700 2100 1107 1021" | ./a.out 2>&1 | less)

#define UNSET 0
#define OPEN 1
#define CLOSED 2
#define PRICE_MAX 4294967295
#define TREE_H 32
#define TREE_X 2
#define TREE_D 4
#define TREE_M 3
#define TREE_MAX_SIZE 1024 * 1024 * 100

/* #define debug(ARGS...) printf(ARGS); */
#define debug(ARGS...) ;
#define info(ARGS...) fprintf(stdout, ARGS);
/* #define assert(ARGS...) ; */

typedef uint64_t dist_t;
typedef uint8_t map_t;
typedef uint32_t count_t;
typedef uint8_t status_t;
typedef uint32_t tree_t;

#ifndef assert
void assert(int cond, char* str) {
  if (!cond) {
    fprintf(stderr, "%s\n", str);
  }
}
#endif

dist_t price(map_t cell_code) {
  if (cell_code >= 128) { // buildings
    return PRICE_MAX / 16;
  }
  switch (cell_code) {
  case 0: // barriers
    return PRICE_MAX;
  case 20: // gates
    return PRICE_MAX / 2;
  case 40: // entrances
    return PRICE_MAX / 4;
  case 60: // spec_landuse
    return PRICE_MAX / 8;
  case 80: // grass
    return 40;
  case 100: // default
    return 10;
  case 120: // footways
    return 1;
  }
}

int clamp_color(int color) {
  if (color < 0) return 0;
  if (color > 255) return 255;
  return color;
}

size_t width, height, wh;
dist_t *dists; // by cells
map_t *map; // by cells
status_t* statuses; // by cells
tree_t *tree;
tree_t *next_cell;
tree_t tree_size;

void add_to_tree(tree_t ix, dist_t val) {
  debug("a_t_t(%d, %lld)\n", ix, val);
  tree_t node = 1;
  tree_t prev_node = 0;
  size_t h = TREE_H * TREE_X;
  int digit = 0;
  while (h > 0) {
    h -= TREE_X;
    digit = (val >> h) & TREE_M;
    if (digit != 0) break;
    node += TREE_D;
    debug("PHASE 1: h=%d, digit=%d, node=%d\n", h, digit, node);
  }
  h += TREE_X;
  while (h > 0) {
    h -= TREE_X;
    prev_node = node;
    digit = (val >> h) & TREE_M;
    node = tree[prev_node + digit];
    debug("PHASE 2: h=%d, digit=%d, node=%d\n", h, digit, node);
    if (!node) break;
  }
  while (h > 0) {
    node = tree_size;
    tree[prev_node + digit] = node;
    debug("PHASE 3: tree[%d + %d] = %d\n", prev_node, digit, node);
    tree_size += TREE_D;
    assert(tree_size < TREE_MAX_SIZE, "Tree overflow");
    /* info("tree_size = %d\n", tree_size); */
    h -= TREE_X;
    digit = (val >> h) & TREE_M;
    prev_node = node;
  }
  tree_t prev_val = tree[prev_node + digit];
  tree[prev_node + digit] = ix + 1;
  debug("PHASE 4: tree[%d + %d] = %d, prev_val=%d\n",
        prev_node, digit, ix + 1, prev_val);
  next_cell[ix] = prev_val;
}

void init_tree(tree_t ix) {
  size_t i = 1;
  for (; i<TREE_H * TREE_D; i+=TREE_D) {
    tree[i] = i + TREE_D;
  }
  tree[i - TREE_D] = ix + 1;
  tree_size = i;
}

typedef struct {
  size_t digits[TREE_H];
  size_t ptrs[TREE_H];
  size_t cell_ptr;
  int finished;
} tree_pos_t;

void next_pos(tree_pos_t *tp) {
  debug("n_p()\n");
  if (tp->cell_ptr) {
    debug("tp->cell_ptr = %d\n", tp->cell_ptr);
    tp->cell_ptr = next_cell[tp->cell_ptr - 1];
    debug("tp->cell_ptr = %d\n", tp->cell_ptr);
    if (tp->cell_ptr) return;
  }
  size_t h = 0;
  tree_t node;
  while ((node = tp->ptrs[h]) == 0) {
    h++;
  }
  size_t digit = tp->digits[h] + 1;
  debug("PHASE 1: h=%d, node=%d, digit=%d\n", h, node, digit);
  while (digit > TREE_M || tree[node + digit] == 0) {
    /* debug("digit = %d\n", digit); */
    if (digit++ >= TREE_M) {
      node = tp->ptrs[++h];
      digit = tp->digits[h] + 1;
      debug("PHASE 1: h=%d, node=%d, digit=%d\n", h, node, digit);
      if (h == TREE_H) {
        tp->finished = 1;
        debug("FINISHED\n");
        return;
      }
    }
  }
  tp->digits[h] = digit;
  tp->ptrs[h] = node;
  debug("PHASE 2: h=%d, node=%d, digit=%d\n", h, node, digit);
  while (h > 0) {
    node = tree[node + digit];
    tp->ptrs[--h] = node;
    debug("PHASE 2: tp->ptrs[%d] = %d\n", h, node);
    digit = 0;
    while (tree[node + digit] == 0) {
      digit++;
    }
    debug("PHASE 2: digit = %d\n", digit);
    assert(digit < TREE_D, "digit >= TREE_D");
    tp->digits[h] = digit;
  }
  assert(tree[node + digit] > 0, "tree[node + digit] == 0");
  tp->cell_ptr = tree[node + digit];
  debug("PHASE 3: h=%d, node=%d, digit=%d\n", h, node, digit);
}

//#define debug(ARGS...) printf(ARGS)
void print_tree() {
  debug("TREE[%d]: ", tree_size);
  for (size_t i=1; i<tree_size; i+=TREE_D) {
    debug("%d:[", i);
    for (size_t j=0; j<TREE_D; j++) {
      if (j > 0) {
        debug(" ");
      }
      debug("%d", tree[i + j]);
    }
    debug("] ");
  }
  debug("\n");
}

void print_cells() {
  debug("NEXT_CELLS: ");
  for (size_t i=0; i<wh; i++) {
    if (next_cell[i]) {
      debug("%d->%d ", i, next_cell[i]);
    }
  }
  debug("\n");
}

void print_pos(tree_pos_t* tree_pos) {
  debug("POS[%d, %d]: ", tree_pos->cell_ptr, tree_pos->finished);
  debug("digits[");
  for (size_t i=0; i<TREE_H; i++) {
    if (i > 0) {
      debug(" ");
    }
    debug("%d", tree_pos->digits[i]);
  }
  debug("] ptrs[");
  for (size_t i=0; i<TREE_H; i++) {
    if (i > 0) {
      debug(" ");
    }
    debug("%d", tree_pos->ptrs[i]);
  }
  debug("]\n");
}
//#define debug(ARGS...) ;

void check_neighbor(dist_t base_val, size_t ix) {
  debug("check ix = %d\n", ix);
  status_t status = statuses[ix];
  #ifdef assert
  if (status != UNSET) return;
  #endif
  dist_t new_val = base_val + price(map[ix]);
  // debug("base_val = %d, price = %d\n", base_val, price(map[ix]));
  if (status == UNSET) {
    statuses[ix] = OPEN;
    dists[ix] = new_val;
    add_to_tree(ix, new_val);
    /* debug("UNSET\n"); */
    return;
  }
  /* debug("new_val = %llu, dists[ix] = %llu\n", new_val, dists[ix]); */
  if (new_val < dists[ix]) {
    assert(status == OPEN, "c_n: status != OPEN");
    assert(0, "status != UNSET");
    debug("WRITE_OPEN");
    //debug("status = %d, dists[ix] = %llu, base_val = %llu, price = %llu, new_val = %llu\n",
    //status, dists[ix], base_val, price(map[ix]), new_val);
    // debug("OPEN\n");
    dists[ix] = new_val;
  }
}

void make_closed(size_t pos) {
  debug("close = %d\n", pos);
  assert(statuses[pos] == OPEN, "m_c: status != OPEN");
  if (statuses[pos] != OPEN) {
    info("statuses[%d] = %d\n", pos, statuses[pos]);
  }
  statuses[pos] = CLOSED;
  dist_t val = dists[pos];
  if (pos >= width) {
    // debug("pos - width\n");
    check_neighbor(val, pos - width);
  }
  if (pos + width < wh) {
    // debug("pos + width\n");
    check_neighbor(val, pos + width);
  }
  if (pos >= 1) {
    // debug("pos - 1\n");
    check_neighbor(val, pos - 1);
  }
  if (pos + 1 < wh) {
    // debug("pos + 1\n");
    check_neighbor(val, pos + 1);
  }
}

void main() {
  FILE *map_file = fopen("map.pgm", "rb");
  size_t cx, cy;
  scanf("%d%d", &width, &height);
  wh = width * height;
  scanf("%d%d", &cx, &cy);
  map = malloc(wh);
  fread(map, 1, wh, map_file);
  fclose(map_file);
  tree = calloc(TREE_MAX_SIZE, sizeof(tree_t));
  info("TREE_SIZE = %dM\n", TREE_MAX_SIZE * sizeof(tree_t) / 1024 / 1024);
  next_cell = calloc(wh, sizeof(tree_t));
  dists = malloc(wh * sizeof(dist_t));
  statuses = malloc(wh * sizeof(status_t));
  for (size_t i=0; i<wh; i++) {
    statuses[i] = UNSET;
  }
  tree_t ix = cy * width + cx;
  tree_size = 1;
  init_tree(ix);
  print_tree();
  // debug("ix = %d\n", ix);
  statuses[ix] = OPEN;
  dists[ix] = 0;
  unsigned long step = 0;
  debug("Starting...\n");
  tree_pos_t tree_pos;
  for (size_t i=0; i<TREE_H; i++) {
    tree_pos.digits[i] = 0;
    tree_pos.ptrs[i] = TREE_D * (TREE_H - 1 - i) + 1;
  }
  tree_pos.cell_ptr = ix + 1;
  tree_pos.finished = 0;
  /*
  print_pos(&tree_pos);
  add_to_tree(222, 2);
  add_to_tree(555, 5);
  add_to_tree(444, 4);
  add_to_tree(333, 3);
  print_tree();
  */
  map_t *res = calloc(wh, 3);
  while (1) {
    step++;
    tree_t cur_pos = tree_pos.cell_ptr;
    assert(cur_pos != 0, "cur_pos == 0");
    int color = step % 1000 < 100 ? 0 : 255;
    res[cur_pos * 3] = clamp_color(color);
    res[cur_pos * 3 + 1] = clamp_color(color);
    res[cur_pos * 3 + 2] = clamp_color(color);
    make_closed(cur_pos - 1);
    next_pos(&tree_pos);
    if (tree_pos.finished) break;
    if (step % 100000 == 0) {
      /* break; */
      info("Step %10d %10dK\n",
            step, tree_size * sizeof(tree_t) / 1024);
    }
  }
  count_t *counts = calloc(wh, sizeof(count_t));
  /*
  for (size_t i=0; i<wh; i++) {
    res[3 * i] = map[i];
    res[3 * i + 1] = map[i];
    res[3 * i + 2] = map[i];
    if (statuses[i] == OPEN) {
      res[3 * i] = 255;
    } else if (statuses[i] == CLOSED) {
      res[3 * i + 1] = 255;
    }
  }
  */
  FILE *res_file = fopen("res_map.ppm", "wb");
  char prefix[30];
  sprintf(prefix, "P6 %d %d 255 ", width, height);
  fwrite(prefix, 1, strlen(prefix), res_file);
  fwrite(res, 1, wh * 3, res_file);
  fclose(res_file);
  free(map);
  free(dists);
  free(statuses);
  free(tree);
  free(counts);
  free(res);
  info("DONE\n");
}
