#define UNSET 0
#define OPEN 1
#define CLOSED 2
#define TREE_H 64
#define TREE_X 1
#define TREE_D 2
#define TREE_M 1
#define TREE_MAX_SIZE 1024 * 1024 * 100

typedef uint32_t tree_t;

tree_t *tree;
tree_t *next_cell;
tree_t tree_size;

void add_to_tree(tree_t ix, dist_t val) {
  tree_t node = 1;
  tree_t prev_node = 0;
  size_t h = TREE_H * TREE_X;
  int digit = 0;
  while (h > 0) {
    h -= TREE_X;
    digit = (val >> h) & TREE_M;
    if (digit != 0) break;
    node += TREE_D;
  }
  h += TREE_X;
  while (h > 0) {
    h -= TREE_X;
    prev_node = node;
    digit = (val >> h) & TREE_M;
    node = tree[prev_node + digit];
    if (!node) break;
  }
  while (h > 0) {
    node = tree_size;
    tree[prev_node + digit] = node;
    tree_size += TREE_D;
    assert(tree_size < TREE_MAX_SIZE, "Tree overflow");
    h -= TREE_X;
    digit = (val >> h) & TREE_M;
    prev_node = node;
  }
  if (tree[prev_node + digit] == 0) {
    tree[prev_node + digit] = ix + 1;
    next_cell[ix] = 0;
  } else {
    tree_t prev_cell = 0;
    tree_t cur_cell = tree[prev_node + digit];
    while (cur_cell > 0) {
      prev_cell = cur_cell;
      cur_cell = next_cell[cur_cell - 1];
    }
    next_cell[prev_cell - 1] = ix + 1;
    next_cell[ix] = 0;
  }
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
  if (tp->cell_ptr) {
    tp->cell_ptr = next_cell[tp->cell_ptr - 1];
    if (tp->cell_ptr) return;
  }
  size_t h = 0;
  tree_t node;
  while ((node = tp->ptrs[h]) == 0) {
    h++;
  }
  size_t digit = tp->digits[h] + 1;
  while (digit > TREE_M || tree[node + digit] == 0) {
    if (digit++ >= TREE_M) {
      node = tp->ptrs[++h];
      digit = tp->digits[h] + 1;
      if (h == TREE_H) {
        tp->finished = 1;
        return;
      }
    }
  }
  tp->digits[h] = digit;
  tp->ptrs[h] = node;
  while (h > 0) {
    node = tree[node + digit];
    tp->ptrs[--h] = node;
    digit = 0;
    while (tree[node + digit] == 0) {
      digit++;
    }
    assert(digit < TREE_D, "digit >= TREE_D");
    tp->digits[h] = digit;
  }
  assert(tree[node + digit] > 0, "tree[node + digit] == 0");
  tp->cell_ptr = tree[node + digit];
}

void print_tree() {
  printf("TREE[%d]: ", tree_size);
  for (size_t i=1; i<tree_size; i+=TREE_D) {
    printf("%d:[", i);
    for (size_t j=0; j<TREE_D; j++) {
      if (j > 0) {
        printf(" ");
      }
      printf("%d", tree[i + j]);
    }
    printf("] ");
  }
  printf("\n");
}

void print_pos(tree_pos_t* tree_pos) {
  printf("POS[%d, %d]: ", tree_pos->cell_ptr, tree_pos->finished);
  printf("digits[");
  for (size_t i=0; i<TREE_H; i++) {
    if (i > 0) {
      printf(" ");
    }
    printf("%d", tree_pos->digits[i]);
  }
  printf("] ptrs[");
  for (size_t i=0; i<TREE_H; i++) {
    if (i > 0) {
      printf(" ");
    }
    printf("%d", tree_pos->ptrs[i]);
  }
  printf("]\n");
}
