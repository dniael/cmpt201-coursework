#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define BUF_SIZE 256

struct header {
  uint64_t size;
  struct header *next;
};

void print_p(char *format, void *data) {
  char buf[BUF_SIZE];
  ssize_t len = snprintf(buf, BUF_SIZE, format, data);
  if (len < 0) {
    perror("snprintf");
  }
  write(STDOUT_FILENO, buf, len);
}

void print_i(char *fmt, uint64_t data) {
  char buf[BUF_SIZE];
  ssize_t len = snprintf(buf, BUF_SIZE, fmt, data);
  if (len < 0)
    perror("snprintf");

  write(STDOUT_FILENO, buf, len);
}

int main() {
  void *addr1 = sbrk(256);

  if (addr1 == (void *)-1) {
    perror("sbrk");
    return 0;
  }

  struct header *block1 = (struct header *)addr1;
  block1->size = 128;
  block1->next = NULL;

  print_p("first block: %p\n", addr1);

  void *addr2 = addr1 + block1->size;
  struct header *block2 = (struct header *)addr2;
  block2->size = 128;
  block2->next = addr1;

  print_p("second block: %p\n", addr2);

  print_i("first block size: %d\n", block1->size);
  print_p("first block next: %p\n", block1->next);

  print_i("second block size: %d\n", block2->size);
  print_p("second block next: %p\n", block2->next);

  memset(addr1, 0, 112);
  memset(addr2, 1, 112);

  uint8_t *p = addr1;
  for (uint8_t *i = p; i < (p + 128); i++) {
    print_i("%lu\n", (uint64_t)*i);
  }

  uint8_t *p1 = addr2;
  for (uint8_t *i = p; i < (p + 128); i++) {
    print_i("%lu\n", (uint64_t)*i);
  }

  return 0;
}
