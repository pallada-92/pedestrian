#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void assert(int cond, char* str) {
  if (!cond) {
    fprintf(stderr, "%s\n", str);
  }
}

typedef uint64_t dist_t;

#include "tree.c"

#define PRICE_MAX 4294967295

#define debug(ARGS...) ;

typedef uint8_t map_t;
typedef uint8_t status_t;

dist_t *dists; // length = wh
map_t *map; // length = wh
status_t* statuses; // length = wh
size_t* parents;

size_t width, height, wh;
size_t sx, sy, six, six, gx, gy, gix;
char diagonal, heuristic;

void print_cells() {
  printf("NEXT_CELLS: ");
  for (size_t i=0; i<wh; i++) {
    if (next_cell[i]) {
      printf("%d->%d ", i, next_cell[i]);
    }
  }
  printf("\n");
}

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

dist_t calc_heuristic(size_t ix1, size_t ix2) {
  int x1, y1, x2, y2;
  x1 = ix1 % width;
  y1 = ix1 / width;
  x2 = ix2 % width;
  y2 = ix2 / width;
  size_t dx, dy;
  if (x1 < x2) {
    dx = x2 - x1;
  } else {
    dx = x1 - x2;
  }
  if (y1 < y2) {
    dy = y2 - y1;
  } else {
    dy = y1 - y2;
  }
  size_t dmin, dmax;
  if (dy < dx) {
    dmin = dy;
    dmax = dx;
  } else {
    dmin = dx;
    dmax = dy;
  }
  switch (heuristic) {
  case 'm': // manhattan
    return 10 * (dmin + dmax);
  case 'o': // octile
    return 14 * dmin + 10 * (dmax - dmin);
  case 'c': // chebyshev
    return 10 * dmax;
  case 'e': // euclid
    return (size_t) (10.0 * sqrt(dmin * dmin + dmax * dmax) / sqrt(2) * 1.4);
  }
  return 0;
}

dist_t calc_cost(size_t ix, int dx, int dy) {
  dist_t price1 = price(map[ix]);
  dist_t price2 = price(map[ix + dx + dy * width]);
  if (dx == 0 || dy == 0) {
    return price1 * 5 + price2 * 5;
  } else { // diagonal
    size_t ix3 = ix + dx;
    size_t ix4 = ix + dy * width;
    dist_t price3 = price(map[ix + dx]);
    dist_t price4 = price(map[ix + dy * width]);
    dist_t max, min;
    if (price3 > price4) {
      max = price3;
      min = price4;
    } else {
      max = price4;
      min = price3;
    }
    if (diagonal == 'd') {
      return price1 * 3 + max * 8 + price2 * 3;
    } else if (diagonal == 'c') {
      return price1 * 3 + min * 8 + price2 * 3;
    } else {
      return price1 * 7 + price2 * 7;
    }
  }
}

void check_neighbor(size_t parent, dist_t new_dist, size_t ix) {
  if (statuses[ix] == UNSET) {
    debug("%lld + %lld = %lld\n", new_dist, calc_heuristic(ix, gix), new_dist + calc_heuristic(ix, gix));
    statuses[ix] = OPEN;
    dists[ix] = new_dist;
    parents[ix] = parent;
    add_to_tree(ix, new_dist + calc_heuristic(ix, gix));
    return;
  }
  if (statuses[ix] == CLOSED) {
    debug("CLOSED\n");
    return;
  }
  debug("%lld + %lld = %lld; ", new_dist, calc_heuristic(ix, gix), new_dist + calc_heuristic(ix, gix));
  if (new_dist < dists[ix]) {
    debug("%lld < %lld\n", new_dist, dists[ix]);
    dists[ix] = new_dist;
    parents[ix] = parent;
  } else {
    debug("%lld >= %lld\n", new_dist, dists[ix]);
  }
}

void make_closed(size_t pos) {
  if (statuses[pos] == CLOSED) return;
  statuses[pos] = CLOSED;
  if (pos < width + 1 || pos + width + 1 > wh) return;
  dist_t val = dists[pos];
  debug("\npos = %lld, cost = %lld + %lld = %lld, x = %d, y = %d\n", pos, val, calc_heuristic(pos, gix), val + calc_heuristic(pos, gix), pos % width, pos / width);
  debug("top [%lld] = ", pos - width);
  check_neighbor(pos, val + calc_cost(pos, 0, -1), pos - width);
  debug("bottom = ");
  check_neighbor(pos, val + calc_cost(pos, 0, +1), pos + width);
  debug("left = ");
  check_neighbor(pos, val + calc_cost(pos, -1, 0), pos - 1);
  debug("right = ");
  check_neighbor(pos, val + calc_cost(pos, +1, 0), pos + 1);
  if (diagonal != 'n') {
    debug("top left = ");
    check_neighbor(pos, val + calc_cost(pos, -1, -1), pos - width - 1);
    debug("top right = ");
    check_neighbor(pos, val + calc_cost(pos, +1, -1), pos - width + 1);
    debug("bottom left = ");
    check_neighbor(pos, val + calc_cost(pos, -1, +1), pos + width - 1);
    debug("bottom right = ");
    check_neighbor(pos, val + calc_cost(pos, +1, +1), pos + width + 1);
  }
}

int clamp_color(int color) {
  if (color < 0) return 0;
  if (color > 255) return 255;
  return color;
}

void main(int argc, char **argv) {
  FILE *map_file = fopen("map.pgm", "rb");
  if (argc != 9) {
    printf("Exactly 8 arguments required\n");
    return;
  }
  width = atoi(argv[1]);
  height = atoi(argv[2]);
  wh = width * height;
  sx = atoi(argv[3]);
  sy = atoi(argv[4]);
  six = sy * width + sx;
  gx = atoi(argv[5]);
  gy = atoi(argv[6]);
  gix = gy * width + gx;
  diagonal = argv[7][0];
  heuristic = argv[8][0];
  if (heuristic == 'm' && diagonal != 'n') {
    printf("Can only use manhattan, when diagonal moves are prohibited\n");
    return;
  }
  map = malloc(wh);
  fread(map, 1, wh, map_file);
  fclose(map_file);
  tree = calloc(TREE_MAX_SIZE, sizeof(tree_t));
  debug("Memory allocated for tree = %dM\n", TREE_MAX_SIZE * sizeof(tree_t) / 1024 / 1024);
  next_cell = calloc(wh, sizeof(tree_t));
  dists = malloc(wh * sizeof(dist_t));
  statuses = malloc(wh * sizeof(status_t));
  parents = malloc(wh * sizeof(status_t));
  for (size_t i=0; i<wh; i++) {
    statuses[i] = UNSET;
  }
  tree_size = 1;
  init_tree(six);
  statuses[six] = OPEN;
  dists[six] = 0;
  unsigned long step = 0;
  debug("Starting...\n");
  tree_pos_t tree_pos;
  for (size_t i=0; i<TREE_H; i++) {
    tree_pos.digits[i] = 0;
    tree_pos.ptrs[i] = TREE_D * (TREE_H - 1 - i) + 1;
  }
  tree_pos.cell_ptr = six + 1;
  tree_pos.finished = 0;
  map_t *res = calloc(wh, 3);
  while (1) {
    step++;
    tree_t cur_pos = tree_pos.cell_ptr;
    assert(cur_pos != 0, "cur_pos == 0");
    debug("%d %d\n", cur_pos - 1, gix);
    if (cur_pos - 1 == gix) {
      printf("PATH FOUND!\n");
      break;
    }
    make_closed(cur_pos - 1);
    // print_cells();
    // print_pos(&tree_pos);
    next_pos(&tree_pos);
    if (tree_pos.finished) break;
    if (step % 10000 == 0) {
      printf("Step %10d %10dK\n",
            step, tree_size * sizeof(tree_t) / 1024);
    }
  }
  printf("Memory used by tree = %dM\n", tree_size * sizeof(tree_t) / 1024 / 1024);
  for (size_t i=0; i<wh; i++) {
    res[3 * i] = map[i];
    res[3 * i + 1] = map[i];
    res[3 * i + 2] = map[i];
    if (statuses[i] == OPEN) {
      res[3 * i + 2] = 255;
    } else if (statuses[i] == CLOSED) {
      res[3 * i + 1] = 255;
    }
  }
  size_t cur_pos = gix;
  while (cur_pos != six) {
    cur_pos = parents[cur_pos];
    res[3 * cur_pos + 0] = 255;
    res[3 * cur_pos + 1] = 0;
    res[3 * cur_pos + 2] = 0;
  }
  res[3 * six + 0] = 255;
  res[3 * six + 1] = 255;
  res[3 * gix + 0] = 255;
  res[3 * gix + 2] = 255;
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
  free(res);
  free(parents);
  printf("DONE\n");
}
