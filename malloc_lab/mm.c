/*
 * The Malloc implementation has the following structure
 *
 * Non free block:
 * ---------------
 *  - Size | free - Header
 *  ---------------
 *  -   Payload   -
 *  ---------------
 *  - Size | free - Footer
 *  ---------------
 *
 *  Free block:
 *  ---------------
 *  - Size | free - Header
 *  ---------------
 *  -  next_free  -
 *  ---------------
 *  -  prev_free  -
 *  ---------------
 *  -   ununsed   -
 *  ---------------
 *  - Size | free - Footer
 *  ---------------
 *
 * Free blocks are a explicitly, doubly and circularly linked list
 * malloc uses first fit with splitting
 * free immediatly coalesces if possible
 *
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
    "5am",
    /* First member's full name */
    "Lennart Koziollek",
    /* First member's email address */
    "lennart.koziollek@uni-due.de",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/*
 * ---------------------------------
 * Struct definition for easy access to chunk meta data
 * Chunk Struct has to be pointed -1 sizeof(unsigned) before start of chunk
 * -> gives easy access to prev_size
 * ---------------------------------
 */
typedef struct Chunk Chunk;
struct Chunk {
  unsigned prev_size;
  unsigned header; // size and flagbits
  char payload;
};

typedef struct FreeChunk FreeChunk;
struct FreeChunk {
  unsigned prev_size;
  unsigned header; // size and flagbits
  FreeChunk *next_chunk;
  FreeChunk *prev_chunk;
  char payload;
};

/* GLOBAL VARIABLE
 * Easy Access and storing of Heapstart
 */
static Chunk *START;
static Chunk *END;
// static FreeChunk *FIRST_FREE;
// static FreeChunk *LAST_FREE;

// defined as global variable because this is being calculated often and stays
// the same
static unsigned MIN_CHUNKSIZE = (sizeof(unsigned) * 2) + (sizeof(char *) * 2);


/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

#define WORDSZ 4
#define DWORDSZ 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * Calculate the Size of a Chunk with just the payload
 * IMPORTANT: Check if bigger than CHUNKMINSIZE
 */
#define CALC_CHUNK_SIZE(payloadsize)                                           \
  (((unsigned)ALIGN(payloadsize)) + ((unsigned)(sizeof(unsigned) * 2)))

/* calculate the payloadsize from size including overhead */
#define PAYLOADSIZE_FROM_CHUNKSIZE(chunksize)                                  \
  (chunksize - (2 * sizeof(unsigned)))

/* get pointer to header of chunk from payload pointer */
#define PAYLOAD_TO_CHUNKPTR(payload_pointer)                                   \
  ((void *)((unsigned *)payload_pointer) - 1)


/* get pointer of type chunk from payload pointer */
#define PAYLOAD_TO_CHUNKSTRUCT_PTR(payload_pointer)                            \
  ((Chunk *)(((unsigned *)payload_pointer) - 2))

/* get pointer of tupe chunk from chunk pointer */
#define CHUNKPTR_TO_CHUNKSTRUCT_PTR(chunkptr)                                  \
  ((Chunk *)(((unsigned *)chunkptr) - 1))

/* get the freebit of a header */
#define GET_FREEBIT(header) (((unsigned)header) & 0b1)

/* get the sizebits of a header */
#define GET_SIZEBIT(header) (((unsigned)header) & ~0x1)

/* sets the freebit to 0 */
#define SET_ISFREE(header) (header &= ~0b1)

/* sets the freebit to 1 */
#define SET_NOTFREE(header) (header |= 0b1)

// set the size of chunk 
#define SET_SIZEBIT(header, size) (header |= ((unsigned)size) << 0b1)

/*
 * gives header pointer of next chunk
 * takes pointer to header
 */
#define JUMP_NEXT_CHUNK(chunkptr)                                              \
  ((unsigned *)((char *)chunkptr) + GET_SIZEBIT(*(unsigned *)(chunkptr)))

/* gives header pointer of prev chunk
 * takes pointer to header
 */
#define JUMP_PREV_CHUNK(chunkptr)                                              \
  (((unsigned *)((char *)chunkptr) - GET_SIZEBIT(*(unsigned *)chunkptr - 1)) + \
   1)

/*
 * ---------------------------------
 * Convenience functions for coding
 * ---------------------------------
 */

/* set the size of block - abstracted */
static inline void set_size(unsigned *header, unsigned payload_size) {
  unsigned aligned = ALIGN(payload_size);
  if (aligned >= MIN_CHUNKSIZE) {
    SET_SIZEBIT(*header, CALC_CHUNK_SIZE(aligned));
  } else {
    SET_SIZEBIT(*header, MIN_CHUNKSIZE);
  }
}

/* gives pointer to next chunk struct
 * takes pointer to chunk struct
 */
#define JUMP_NEXT_FROM_STRUCT(structptr)                                       \
  ((Chunk *)(((char *)structptr) + (GET_SIZEBIT(*((unsigned *)structptr + 1)))))

/* gives pointer to prev chunk struct
 * takes pointer to chunk struct
 */
#define JUMP_PREV_FROM_STRUCT(structptr)                                       \
  ((Chunk *)((char *)structptr) - GET_SIZEBIT(*(unsigned *)structptr))

/*
 * get the setbit of the chunk after the payload.
 * for easy access of nextchunk to this chunk
 * takes pointer to header of current chunk
 */
#define SET_FOOTER(structptr, size)                                            \
  SET_SIZEBIT(((JUMP_NEXT_FROM_STRUCT(structptr))->prev_size), size)

// set chunk after structptr to the last chunk in heap (size 0)
#define SET_LASTCHUNK(structptr)                                               \
  Chunk *last_chunk = JUMP_NEXT_FROM_STRUCT(structptr);                        \
  SET_NOTFREE(last_chunk->header);                                             \
  SET_SIZEBIT(last_chunk->header, 0);                                          \
  END = last_chunk;

/*
 * ---------------------------------
 * functions
 * ---------------------------------
 */

int mm_check(int line_num);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {

  int size_of_first_chunk = MIN_CHUNKSIZE;
  int size_of_last_chunk = sizeof(Chunk);

  void *heap = mem_sbrk(size_of_first_chunk + size_of_last_chunk);

  // Check if sbrk was successfull
  if (heap == (void *)-1) {
    return -1;
  }

  // this is fine here because prev_size = 0 is the bottom boundary of the heap
  FreeChunk *first_chunk = (FreeChunk *)heap;

  // bottom boundary
  first_chunk->prev_size = 1;

  SET_ISFREE(first_chunk->header);
  SET_SIZEBIT(first_chunk->header, size_of_first_chunk);
  first_chunk->next_chunk = first_chunk;
  first_chunk->prev_chunk = first_chunk;

  START = (Chunk *)first_chunk;
  END = JUMP_NEXT_FROM_STRUCT(START);

  SET_LASTCHUNK(first_chunk);

#ifdef CHECKHEAP
  mm_check(__LINE__);
#endif

  return 0;
}

FreeChunk *first_fit(size_t size) {

#ifdef CHECKHEAP
  mm_check(__LINE__);
#endif

  Chunk *find = START;

  while (GET_FREEBIT(find->header) == 1 && find != END) {
    find = JUMP_NEXT_FROM_STRUCT(find);
  }

  if (find == END) {
    return NULL;
  } else {
    // available free block
    FreeChunk *current = (FreeChunk *)find;

    while (PAYLOADSIZE_FROM_CHUNKSIZE(GET_SIZEBIT(current->header)) < size) {
      if (current->next_chunk == (FreeChunk *)find) {
        return NULL;
      }
      current = current->next_chunk;
    }

    return current;
  }

#ifdef CHECKHEAP
  mm_check(__LINE__);
#endif
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 *
 * try to insert if there is space - first fit
 * if no space grow heap
 */
void *mm_malloc(size_t size) {

#ifdef CHECKHEAP
  mm_check(__LINE__);
#endif

  FreeChunk *fit = first_fit(size);

  // no free chunks available
  if (fit == NULL) {
    int newsize = CALC_CHUNK_SIZE(size);
    if (newsize < MIN_CHUNKSIZE)
      newsize = MIN_CHUNKSIZE;
    void *p = mem_sbrk(newsize);

    if (p == (void *)-1) {
      return NULL;
    } else {
      Chunk *new_chunk = END;
      SET_SIZEBIT(new_chunk->header, newsize);
      SET_NOTFREE(new_chunk->header);

      SET_LASTCHUNK(new_chunk);
      return &new_chunk->payload;
    }
    // free chunks available
  } else {

    unsigned oldsize = GET_SIZEBIT(fit->header);
    unsigned calcedsize = CALC_CHUNK_SIZE(size);
    if (calcedsize < MIN_CHUNKSIZE)
      calcedsize = MIN_CHUNKSIZE;

    if (GET_SIZEBIT(fit->header) >= (calcedsize + MIN_CHUNKSIZE)) {
      // split

      SET_SIZEBIT(fit->header, calcedsize);
      SET_NOTFREE(fit->header);

      FreeChunk *new_split = (FreeChunk *)JUMP_NEXT_FROM_STRUCT(fit);

      new_split->prev_size = fit->header;
      SET_SIZEBIT(new_split->header, oldsize - calcedsize);
      SET_ISFREE(new_split->header);
      SET_FOOTER(new_split, new_split->header);

      new_split->next_chunk = fit->next_chunk;
      new_split->prev_chunk = fit->prev_chunk;

    } else {
      // dont split

      fit->prev_chunk->next_chunk = fit->next_chunk;
      fit->next_chunk->prev_chunk = fit->prev_chunk;

      SET_NOTFREE(fit->header);
      SET_FOOTER(fit, fit->header);
    }

    return &fit->payload;
  }
}

inline void coalesc(FreeChunk *first, FreeChunk *second) {

#ifdef CHECKHEAP
  mm_check(__LINE__);
#endif

  unsigned new_size = GET_SIZEBIT(first->header) + GET_SIZEBIT(second->header);

  SET_SIZEBIT(first->header, new_size);
  SET_FOOTER(first, new_size);

  second->next_chunk->prev_chunk = second->prev_chunk;
  second->prev_chunk->next_chunk = second->next_chunk;

#ifdef CHECKHEAP
  mm_check(__LINE__);
#endif
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {

#ifdef CHECKHEAP
  mm_check(__LINE__);
#endif

  FreeChunk *chunk = (FreeChunk *)CHUNKPTR_TO_CHUNKSTRUCT_PTR(ptr);
  if (GET_FREEBIT(chunk->header) == 1) {
    printf("Trying to freeing a non free chunk. Canceling\n");
    return;
  }

  printf("\n");

  // ---  Insert into free list ---
  SET_ISFREE(chunk->header);
  // SET_FOOTER(chunk, chunk->header);
  FreeChunk *next1 = (FreeChunk *)JUMP_NEXT_FROM_STRUCT(chunk);
  next1->prev_size = chunk->header;

  // find next prev free chunk
  FreeChunk *anker = (FreeChunk *)START;

  anker->next_chunk->prev_chunk = chunk;
  chunk->next_chunk = anker->next_chunk;
  anker->next_chunk = chunk;
  chunk->prev_chunk = anker;

#ifdef CHECKHEAP
  mm_check(__LINE__);
#endif

  // --- coalescing ---

  // coalesc prev
  if (chunk->prev_size != 1) {
    if (GET_FREEBIT(chunk->prev_size) == 0) {
      coalesc((FreeChunk *)JUMP_PREV_FROM_STRUCT(chunk), chunk);
    }
  }

#ifdef CHECKHEAP
  mm_check(__LINE__);
#endif

  FreeChunk *next = (FreeChunk *)JUMP_NEXT_FROM_STRUCT(chunk);

  // coalesc next
  if (next < (FreeChunk *)END) {
    if (GET_FREEBIT(next->header) == 0) {
      coalesc(chunk, next);
    }
  }

#ifdef CHECKHEAP
  mm_check(__LINE__);
#endif
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

void print_chunk(Chunk c) {
  unsigned footer = JUMP_NEXT_FROM_STRUCT(&c)->prev_size;
  printf("Chunk:\nsize: %u\nfree: %u\nfsize: %u\nffree: %u",
         GET_SIZEBIT(c.header), GET_FREEBIT(c.header), GET_SIZEBIT(footer),
         GET_FREEBIT(footer));
}

int mm_check(int line_num) {

  int was_error = 0;

  // printf("\n--------CHECKHEAP---------\n");

  Chunk *current = START;

  // check if every block added together equals the size of the heap
  unsigned size = 0;

  while (GET_SIZEBIT(current->header) != 0) {
    // printf("size: %u\n", GET_SIZEBIT(current->header));
    current = JUMP_NEXT_FROM_STRUCT(current);
    size += GET_SIZEBIT(current->header);
  }
  size += sizeof(Chunk);

  if (size != mem_heapsize()) {
    printf("Line %d: Heapsize(%u) does not match given Heapsize(%zu)\n",
           line_num, size, mem_heapsize());
  }
  //
  // if (((char *)END + sizeof(Chunk)) != mem_heap_hi()) {
  //   printf("Line %d: END does not equal given Heap End.\n", line_num);
  // }

  if (current != END) {
    was_error = 1;
    printf("Line %d: Traversed current does not equal END.\n", line_num);
  }

  FreeChunk *check_free = (FreeChunk *)START;

  while (check_free != (FreeChunk *)END &&
         GET_FREEBIT(check_free->header) == 1) {
    check_free = (FreeChunk *)JUMP_NEXT_FROM_STRUCT(check_free);
  }

  FreeChunk *anker = check_free;

  if (GET_FREEBIT(check_free->header) == 0) {
    while (check_free->next_chunk != anker) {
      if (GET_FREEBIT(check_free->header) == 1) {
        was_error = 1;
        printf("Line %d: Chunk in forward Freelist is not free\n", line_num);
      }
      check_free = check_free->next_chunk;
    }

    while (check_free->prev_chunk != anker) {
      if (GET_FREEBIT(check_free->header) == 1) {
        was_error = 1;
        printf("Line %d: Chunk in backwards Freelist is not free\n", line_num);
      }
      check_free = check_free->prev_chunk;
    }
  }

  if (was_error == 1) {
    printf("--------END---------\n");
  }
  return 0;
}
