# Malloc Lab

## Things to keep in mind

- Do I have to initialiaze multiple blocks on start or just one?
 - initialize list with a minimum amount of blocks (maybe not needed) and add blocks using sbrk later
- Check alignment methods

### Explicit List

Using stored pointer to next and the before free block
Is only being stored inside of the free blocks

When freeing use first in first out or address ordered algorithm?


## Heap Checker

Goal: print the line where checker was called 
  and maybe at what point in the heap are nonsensical values (This is probably not needed)

How can I check that the Heap is correct?
Depends on the implementation of the heap datastructure

- repeat steps of implicit list
- also check if all free blocks point to other free blocks
 - additonaly check if the last pointer actually points to the last element

## Data Structures 

Non free block:

---------------
- Size | free - 
---------------
-   Payload   -
---------------
- Size | free - 
---------------

Free block:

---------------
- Size | free - 
---------------
-  next_free  -
---------------
-  prev_free  -
---------------
-   ununsed   -
---------------
- Size | free - 
---------------

first block in the heap has prevsize 0

Last block in the Heap has to have Sizebit set to 0
next_free and prev_free have to be cirucularly linked in the end


### Struct(easy access to Metadata)

Starts at chunk minus sizeof(unsigned)

Non free chunk:

  prev_size
  size
  payload
  
free chunk:
  
  prev_size
  size
  next_free
  prev_free

## Free list implementation

global first_free pointer
doubly linked list ordered in memory

### inserting into freelist

1. set to free
2. go backwards until a free block is found
  a. Set next of found block to current
  b. If not found: set current next to FIRST_FREE and set FIRST_FREE to current and current prev to NULL
3. go forwards until a free block is found
  a.Set prev of found block to current
  b. If not found: set current next to NULL

