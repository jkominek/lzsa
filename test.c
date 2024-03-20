#include <stddef.h>
#include <lib.h>
#include <stdio.h>
void main(void) {
  long long orig = 0;
  long long compressed = 0;
  int commandCount = 0;
  int safeDist = 0;
  lzsa_stats stats;
  int ret =
    lzsa_compress_file("Makefile", "Makefile.lzsa", NULL, 0, 0, 1, NULL,
                       &orig, &compressed, &commandCount, &safeDist, &stats);
  printf("ret: %i\n", ret);
  printf("orig: %ld\n", orig);
  printf("compressed: %ld\n", compressed);
}
