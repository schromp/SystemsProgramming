/*
 * author: Lennart Koziollek
 *
 */

#include "cachelab.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct cache_line {
  int valid;
  int tag;
  int *data;
} cache_line;

int main(int argc, char **argv) {

  int opt, S, E, B;
  int verbose = 0;
  int trace = 0;

  while (-1 != (opt = getopt(argc, argv, ":s:E:b:t:"))) {
    switch (opt) {
    case 'h':
      // prints usage info
      printf("usage info");
      return 0;
      break;
    case 'v':
      // verbose: displays trace info
      verbose = 1;
      break;
    case 's':
      // number of set index bits
      S = 2 ^ atoi(optarg);
      break;
    case 'E':
      // associativity: number of lines per set
      E = atoi(optarg);
      break;
    case 'b':
      // number of block bits
      B = 2 ^ atoi(optarg);
      break;
    case 't':
      // tracefile: name of valgrind trace to replay
      trace = 1;
      break;
    default:
      printf("wrong argument");
      break;
    }
  }

  // todo: size out of args
  // [x][y] x is the set index and y is the line index
  cache_line cache[3][3];

  printSummary(0, 0, 0);
  return 0;
}
