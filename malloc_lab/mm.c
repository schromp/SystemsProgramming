/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "cool",
    /* First member's full name */
    "Lennart Koziollek",
    /* First member's email address */
    "lennart.koziollek@uni-due.de",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* get chunk pointer from payload */
// TODO:

/* tagbit operations */
#define GET_FREEBIT(header) (((unsigned)header) & 0b1)
#define GET_SIZEBIT(header) (((unsigned)header) >> 0b1)

#define SET_ISFREE(pointer) (*(unsigned *)pointer) &= ~0b1
#define SET_NOTFREE(pointer) (*(unsigned *)pointer) |= 0b1

// set the size in blocks
#define SET_SIZEBIT(pointer, size)                                             \
  (*(unsigned *)pointer |= ((unsigned)size) << 0b1)

/* set and get internal pointers */
#define GET_NEXTBLOCK(pointer) *((void **)((unsigned *)pointer) + 1)
#define GET_PREVBLOCK(pointer) *((void **)((unsigned *)pointer) + 3)

#define SET_NEXTBLOCK(pointer, pointer_next_block)                             \
  GET_NEXTBLOCK(pointer) = pointer_next_block
#define SET_PREVBLOCK(pointer, pointer_prev_block)                             \
  GET_PREVBLOCK(pointer) = pointer_prev_block

#define GROW_HEAP(pointer, size)                                               \
  SET_ISFREE(pointer);                                                         \
  SET_SIZEBIT(pointer, size)

#define ALLOCATE(pointer, size)                                                \
  SET_NOTFREE(pointer);                                                        \
  SET_SIZEBIT(pointer, size)

/* Struct definition for easy access to chunk meta data */
typedef struct Chunk Chunk;
struct Chunk {
  unsigned header; // size and flagbits
  Chunk *next_chunk;
  Chunk *prev_chunk;
  void *payload; // TODO: this is probably wrong because of alignment
};

static Chunk *start;

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  // init to 1 word block by default  TODO: is this the best action to take?

  void *heap = mem_sbrk(2 * ALIGNMENT);
  if (heap == (void *)-1) {
    return -1;
  }

  // creates one free block of memory
  ALLOCATE(heap, 1);
  SET_ISFREE(heap);
  SET_NEXTBLOCK(heap, NULL);
  SET_PREVBLOCK(heap, NULL);

  start = (Chunk *)heap;

  return 0;
}

Chunk *first_fit(size_t size) {

  Chunk *current = start;

  while (current->next_chunk != NULL) {
    unsigned current_size = (GET_SIZEBIT(current->header));
    if (current_size <= size) {
      return current;
    }
    current = current->next_chunk;
  }

  return NULL;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {

  /*
   * try to insert if there is space - first fit
   * if no space grow heap
   */

  Chunk *fit = first_fit(size);

  if (fit == NULL) {
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);

    if (p == (void *)-1)
      return NULL;
    else {
      *(size_t *)p = size;
      return (void *)((char *)p + SIZE_T_SIZE);
    }
  } else {
    SET_NOTFREE(&fit->header);
    return fit->payload;
  }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
  // TODO: need a get function for the header from the view of the payload pointer
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
  /*
   * ATTENTION: You do not need to implement realloc for this assignment
   */
  void *oldptr = ptr;
  void *newptr;
  size_t copySize;

  newptr = mm_malloc(size);
  if (newptr == NULL)
    return NULL;
  copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
  if (size < copySize)
    copySize = size;
  memcpy(newptr, oldptr, copySize);
  mm_free(oldptr);
  return newptr;
}
