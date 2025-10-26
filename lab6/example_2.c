#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ASSERT(expr)                                                           \
  {                                                                            \
    if (!(expr)) {                                                             \
      fprintf(stderr, "Assertion failed in file %s line %d: %s\n", __FILE__,   \
              __LINE__, #expr);                                                \
      exit(1);                                                                 \
    }                                                                          \
  }

#define TEST(expr)                                                             \
  {                                                                            \
    if (!(expr)) {                                                             \
      fprintf(stderr, "Test failed: %s\n", #expr);                             \
      exit(1);                                                                 \
    }                                                                          \
  }

typedef struct node {
  uint64_t data;
  struct node *next;
} node_t;

typedef struct info {
  uint64_t sum;
} info_t;

node_t *head = NULL;
info_t info = {0};

int sum_list(node_t *head) {

  node_t *curr = head;
  int sum = 0;
  while (curr != NULL) {
    sum += curr->data;
    curr = curr->next;
  }

  return sum;
}
void insert_sorted(uint64_t data) {
  node_t *new_node = malloc(sizeof(node_t));
  new_node->data = data;
  new_node->next = NULL;

  if (head == NULL) {
    head = new_node;
  } else if (data < head->data) {
    new_node->next = head;
    head = new_node;
  } else {
    node_t *curr = head;
    node_t *prev = NULL;

    while (curr != NULL) {
      if (data < curr->data) {

        prev->next = new_node;
        new_node->next = curr;
        break;
      } else if (curr->next == NULL) {
        curr->next = new_node;
        break;
      }

      prev = curr;
      curr = curr->next;
    }
  }

  info.sum += data;
}

int index_of(uint64_t data) {
  node_t *curr = head;
  int index = 0;

  while (curr != NULL) {
    if (curr->data == data) {
      return index;
    }

    curr = curr->next;
    index++;
  }

  return -1;
}

int main() {
  insert_sorted(1);

  ASSERT(info.sum == sum_list(head));
  insert_sorted(3);

  ASSERT(info.sum == sum_list(head));
  insert_sorted(5);

  ASSERT(info.sum == sum_list(head));
  insert_sorted(2);

  ASSERT(info.sum == sum_list(head));
  TEST(info.sum == 1 + 3 + 5 + 2);
  TEST(index_of(2) == 1);
  ASSERT(info.sum == sum_list(head));

  return 0;
}
