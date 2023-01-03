# Malloc Lab

## Things to keep in mind

 - Use Macros and inline functions in order to speed up program and get more throughput
 - check pointer casting thoroughly
 - there will be no real structs for the heap and heap blocks because the datastructure is stored directly into memory
   -  Exception: There might be a way to utilize structs to access memory easier (example next and before pointer)
   -  Macros will be better suited for this, maybe
 - Do I have to initialiaze multiple blocks on start or just one?
   - initialize list with a minimum amount of blocks (maybe not needed) and add blocks using sbrk later

## Data Structure

Maybe store some heap information on start of heap (size)

Header with free bit and size of allocated memory
If free bit equals 1 then store pointers to the next and the before free element
  if block is the first or last one in heap it points to null
Data after headerbit has to be aligned properly

Sizes:
  Header size = unsigned int
  free header size = unsigned int + pointer + pointer = 4 Byte + 8 Byte + 8 Byte = 20 Byte
  
  pointer take up atleast a block(depending on alignment - 2 for alignment(4) and 1 for alignment(8))
  size needed in free is: 16byte(2 or 4 blocks)

### Implicit List

Using only size of allocated memory to find next spot in memory
Problem: when freeing the values coalescing behind is hard. This leads to fragmentation
Solution: Bidirectional coalescing. Store the size of block at end

### Explicit List

Using stored pointer to next and the before free block
Is only being stored inside of the free blocks

When freeing use first in first out or address ordered algorithm?


## Heap Checker

Goal: print the line where checker was called 
  and maybe at what point in the heap are nonsensical values (This is probably not needed)

How can I check that the Heap is correct?
Depends on the implementation of the heap datastructure


### Implicit List
 - Go through block by block and look at the size. 
 - Step to next block using the size
 - Incase of wrong size values(not aligned or 0) heap incorrect
 - Have to get to the end of the heap. If not there has been an error

### Explicit List
 - repeat steps of implicit list
 - also check if all free blocks point to other free blocks
   - additonaly check if the last pointer actually points to the last element

