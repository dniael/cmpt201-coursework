#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct header {
  uint64_t size;
  struct header *next;
  int id;
};

void initialize_block(struct header *block, uint64_t size, struct header *next,
                      int id) {
  block->size = size;
  block->next = next;
  block->id = id;
}

int find_first_fit(struct header *free_list_ptr, uint64_t size) {
  // TODO: Implement first fit

  struct header *current = free_list_ptr;
  while (current != NULL) {
    if (current->size >= size) {
      return current->id;
    }

    current = current->next;
  }

  return -1;
}

uint64_t max(uint64_t a, uint64_t b) { return a > b ? a : b; }

uint64_t min(uint64_t a, uint64_t b) { return a < b ? a : b; }

int find_best_fit(struct header *free_list_ptr, uint64_t size) {
  int best_fit_id = -1;
  // TODO: Implement best fit

  struct header *current = free_list_ptr;
  uint64_t delta = UINT64_MAX;
  while (current != NULL) {
    if (current->size >= size && (current->size - size) < delta) {
      delta = current->size - size;
      best_fit_id = current->id;
    }

    current = current->next;
  }

  return best_fit_id;
}

int find_worst_fit(struct header *free_list_ptr, uint64_t size) {
  int worst_fit_id = -1;
  // TODO: Implement worst fit

  struct header *current = free_list_ptr;
  uint64_t delta = 0;
  while (current != NULL) {
    if (current->size >= size && (current->size - size) > delta) {
      delta = current->size - size;
      worst_fit_id = current->id;
    }

    current = current->next;
  }

  return worst_fit_id;
}

int main(void) {

  struct header *free_block1 = (struct header *)malloc(sizeof(struct header));
  struct header *free_block2 = (struct header *)malloc(sizeof(struct header));
  struct header *free_block3 = (struct header *)malloc(sizeof(struct header));
  struct header *free_block4 = (struct header *)malloc(sizeof(struct header));
  struct header *free_block5 = (struct header *)malloc(sizeof(struct header));

  initialize_block(free_block1, 6, free_block2, 1);
  initialize_block(free_block2, 12, free_block3, 2);
  initialize_block(free_block3, 24, free_block4, 3);
  initialize_block(free_block4, 8, free_block5, 4);
  initialize_block(free_block5, 4, NULL, 5);

  struct header *free_list_ptr = free_block1;

  int first_fit_id = find_first_fit(free_list_ptr, 7);
  int best_fit_id = find_best_fit(free_list_ptr, 7);
  int worst_fit_id = find_worst_fit(free_list_ptr, 7);

  // TODO: Print out the IDs

  printf("first fit id: %d\n", first_fit_id);
  printf("best_fit_id: %d\n", best_fit_id);
  printf("worst_fit_id: %d\n", worst_fit_id);

  free(free_block1);
  free(free_block2);
  free(free_block3);
  free(free_block4);
  free(free_block5);
  return 0;
}

// coalescing algorithm pseudocode
// struct header *coalesce_around(struct header *block) {
//  set block->next to next free block (if there are any)
//  struct header* front = NULL;
//  struct header* back = NULL;
//  void *block_addr = (void *)block;
//  loop through linked list with block as header {
//    if (current (ptr address) == block_addr + block->size) front = current
//    if (current (ptr address) == block_addr - current->size) back = current
//  }
//
//
//  if (front && back) {
//    new block address = back
//    new block size = back size + block size + front size
//  } else if (front) {
//    keep original block address
//    new block size = block size + front size
//  } else if (back) {
//    new block address = back
//    new block size = back size + block size
//  } else {
//    new block address = null
//  }
//
//  free(block)
//
//  return new block address
