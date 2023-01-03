/*
 * author: Lennart Koziollek
 *
 */

#include "cachelab.h"
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// GLOBAL VARIABLE
int verbose = 0;

/*
 * --- LINE STRUCT AND FUNCTIONS
 */
typedef struct cache_line {
  // for easy access while inserting line into cache
  unsigned long set_bits;
  unsigned long block_bits;

  int valid;
  unsigned long tag;
  struct cache_line *next;
} cache_line;

/**
 * @brief gets the bits out of the given address
 *
 * @param address
 * @param leftmost_bit starting with 1
 * @param rightmost_bit starting with 1
 *
 * @return the part out of the address
 */
unsigned get_bits(unsigned long address, unsigned long leftmost_bit,
                  unsigned long rightmost_bit) {
  unsigned long converted = (1 << leftmost_bit) - 1;
  unsigned long bits_until_index = address & converted;
  unsigned long bits_between = bits_until_index >> rightmost_bit;

  return bits_between;
}

/**
 * @brief convert input into cacheline
 *
 * @param address
 * @param size
 *
 * @return cacheline
 */
cache_line into_cache_line(unsigned long address, int num_block_bits,
                           int num_set_bits) {
  int rest_bits;
  unsigned long block_bits;
  unsigned long set_bits;
  unsigned long tag_bits;

  rest_bits = num_block_bits + num_set_bits;
  block_bits = get_bits(address, num_block_bits, 0);
  set_bits = get_bits(address, rest_bits, num_block_bits);
  tag_bits = get_bits(address, (sizeof(unsigned long) * CHAR_BIT) - 1,
                      rest_bits - 1); 

  // printf("ad %lx, tb %lx, sb %lx, bb %lx\n", address, tag_bits, set_bits, block_bits);

  cache_line line;
  line.valid = 0;
  line.block_bits = block_bits;
  line.set_bits = set_bits;
  line.tag = tag_bits;
  line.next = NULL;
  return line;
}

/*
 * --- SET STRUCT AND FUNCTIONS ---
 */

typedef struct cache_set {
  int line_amount;
  cache_line *first;
  cache_line *last;
} cache_set;

/**
 * @brief does the set contain the line?
 *
 * @param set
 * @param tag
 *
 * @return 0 for no, 1 for yes
 */
int set_contains_line(cache_set *set, unsigned long tag) {
  cache_line *current = set->first;

  for (int i = 0; i < set->line_amount; i++) {
    if (current->tag == tag) {
      return 1;
    }
    current = current->next;
  }

  return 0;
}

void set_enqueue(cache_set *set, cache_line new_line) {
  cache_line *line = calloc(1, sizeof(cache_line));
  *line = new_line;
  if (set->first == NULL) {
    set->first = line;
    set->last = line;
    set->line_amount++;
  } else {
    set->last->next = line;
    set->last = line;
    set->line_amount++;
  }
}

void set_dequeue(cache_set *set) {
  cache_line *current = set->first;

  if (current == NULL) {
    return; // set is already empty
  }

  set->first = current->next;
  free(current);
  set->line_amount--;
}

void set_insert_line(cache_set *set, cache_line line, int max_lines) {
  set_enqueue(set, line);
  set_dequeue(set);
}

void set_line_to_first(cache_set *set, int tag) {
  cache_line *current = set->first;
  cache_line *before = NULL;

  while (current != set->last && current->tag != tag) {
    before = current;
    current = current->next;
  }

  if (before == NULL && current != NULL) {
    return; // current is the first in queue
  } else if (before == NULL && current == NULL) {
    return; // queue is empty - this should get checked before hand
  } else {
    if (current->next != NULL) {
      before->next = current->next;
      set->last->next = current;
      set->last = current;
      current->next = NULL;
    }
  }
}

/*
 * --- CACHE SIMULATOR IMPLEMENTATION ---
 */

typedef enum return_t {
  MISS,
  HIT,
  HITHIT,
  MISSHIT,
  MISSEVICTION,
  MISSEVICTIONHIT,
} return_t;

char *status_to_string(return_t status) {
  switch (status) {
  case MISS:
    return "miss \n";
  case HIT:
    return "hit \n";
  case HITHIT:
    return "hit hit \n";
  case MISSHIT:
    return "miss hit \n";
  case MISSEVICTION:
    return "miss eviction \n";
  case MISSEVICTIONHIT:
    return "miss eviction hit \n";
  }
  return "ERROR";
}

/*
 * Modes:
 * L: Data load -> load data into cache
 * S: Data store -> load data back to memory
 * M: Data modify -> load followed by store
 */

return_t insert_line(cache_set *set, cache_line line, char mode,
                     int max_lines) {

  return_t ret;

  if (mode == 'L' || mode == 'S') {

    if (set_contains_line(set, line.tag) == 1) { 
      ret = HIT;
      set_line_to_first(set, line.tag);

    } else {

      if (set->line_amount < max_lines) {
        set_enqueue(set, line);
        ret = MISS;

      } else {
        set_insert_line(set, line, max_lines);
        ret = MISSEVICTION;
      }
    }

  } else if (mode == 'M') {
    if (set_contains_line(set, line.tag) == 1) {
      ret = HITHIT;
      set_line_to_first(set, line.tag);
    } else {
      if (set->line_amount < max_lines) {
        set_enqueue(set, line);
        ret = MISSHIT;
      } else {
        set_insert_line(set, line, max_lines);
        ret = MISSEVICTIONHIT;
      }
    }
  }

  return ret;
}

/**
 * @brief debug function
 *
 * @param cache
 */
void print_cache(cache_set *cache, int sets, int lines) {

  for (int i = 0; i < sets; i++) {
    printf("Set %d:", i);
    if (cache[i].first == NULL) {
      printf("Empty.\n");
      continue;
    } else {
      cache_line *current = cache[i].first;
      for (int j = 0; j < cache[i].line_amount; j++) {
        printf("Line(%lx, %lx %lx, %p), ", current->tag, current->set_bits,
               current->block_bits, current->next);
        // printf("%p", current->next);
        current = current->next;
      }
    }
    printf("\n");
  }
  printf("\n");
}

/*
 * --- MAIN ---
 */

int main(int argc, char **argv) {

  int opt;

  int S; // number of sets
  int E; // number of lines per set
  char *trace = "";

  int block_bits;
  int set_bits;

  while (-1 != (opt = getopt(argc, argv, "hvs:E:b:t:"))) {
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
      set_bits = atoi(optarg);
      S = 1 << set_bits;
      break;
    case 'E':
      // associativity: number of lines per set
      E = atoi(optarg);
      break;
    case 'b':
      // number of block bits
      block_bits = atoi(optarg);
      break;
    case 't':
      // tracefile: name of valgrind trace to replay
      trace = optarg;
      break;
    default:
      printf("wrong argument");
      break;
    }
  }

  // --- defining the cache ---
  cache_set *cache = calloc(S, sizeof(cache_set));

  // --- reading file into memory ---
  FILE *pFile;
  char identifier;
  unsigned long address;
  int size;

  // --- counting hits, misses and evictions
  int hits = 0;
  int misses = 0;
  int evictions = 0;

  pFile = fopen(trace, "r");
  if (pFile == NULL) {
    printf("Error opening file: %s\n", trace);
    exit(EXIT_FAILURE);
  }

  while (fscanf(pFile, " %c %lx,%d", &identifier, &address, &size) > 0) {

    if (identifier == 'I') {
      continue;
    }
    // load input into cache
    cache_line cache_line = into_cache_line(address, block_bits, set_bits);
    // printf("Insert setbits: %lu\n", cache_line.set_bits);
    return_t status =
        insert_line(&cache[cache_line.set_bits], cache_line, identifier, E);

    if (verbose == 1) {
      printf("%c %lx,%d %s", identifier, address, size,
             status_to_string(status));
    }

    // printf("%d ", S);
    // printf("%u\n", cache[3].line_amount);
    print_cache(cache, S, E); //600a60

    switch (status) {
    case MISS:
      misses++;
      break;
    case HIT:
      hits++;
      break;
    case HITHIT:
      hits += 2;
      break;
    case MISSHIT:
      hits++;
      misses++;
      break;
    case MISSEVICTION:
      misses++;
      evictions++;
      break;
    case MISSEVICTIONHIT:
      hits++;
      misses++;
      evictions++;
      break;
    }
  }

  // --- closing up the program ---
  free(cache);

  fclose(pFile);

  printSummary(hits, misses, evictions);

  return 0;
}
