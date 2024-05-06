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

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

// align to 8 bytes
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

#define MIN_BLOCK_SIZE 16

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - (WSIZE))
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define SIZE_TO_FTRP(bp, size) ((char *)(bp) + size - DSIZE)
#define SIZE_TO_NEXT_BLKP(bp, size) ((char *)(bp) + size)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp) - (WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - (GET_SIZE((char *)(bp) - (DSIZE))))

static void *heap_listp;

static void *prev_listp;

static void *coalesce(void *bp) {
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if (prev_alloc && next_alloc)
    return bp;
  else if (prev_alloc && !next_alloc) {
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  } else if (!prev_alloc && next_alloc) {
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  } else {
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  return bp;
}

static void *extend_heap(size_t words) {
  char *bp;
  size_t size;

  size = (words + (words & 1)) * WSIZE;

  if (((long)(bp = mem_sbrk(size))) == -1) {
    return NULL;
  }

  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

  return coalesce(bp);
}

static void *next_fit(size_t asize) {
  char *bp = prev_listp;
  size_t b_size;

  while ((b_size = GET_SIZE(HDRP(bp)))) {
    if (!GET_ALLOC(HDRP(bp)) && b_size >= asize) {
      prev_listp = bp;
      return bp;
    }
    bp = SIZE_TO_NEXT_BLKP(bp, b_size);
  }

  for (bp = heap_listp; bp != prev_listp; bp = SIZE_TO_NEXT_BLKP(bp, b_size)) {
    b_size = GET_SIZE(HDRP(bp));
    if (!GET_ALLOC(HDRP(bp)) && b_size >= asize) {
      prev_listp = bp;
      return bp;
    }
  }
  return NULL;
}

static void *find_fit(size_t asize) {
  char *bp = heap_listp;
  size_t b_size;

  while ((b_size = GET_SIZE(HDRP(bp)))) {
    if (!GET_ALLOC(HDRP(bp)) && b_size >= asize) {
      return bp;
    }
    bp = SIZE_TO_NEXT_BLKP(bp, b_size);
  }
  return NULL;
}

static void place(void *bp, size_t asize) {
  size_t size = GET_SIZE(HDRP(bp));
  size_t padding = size - asize;

  if (padding >= MIN_BLOCK_SIZE) {
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(SIZE_TO_FTRP(bp, asize), PACK(asize, 1));
    PUT(HDRP(SIZE_TO_NEXT_BLKP(bp, asize)), PACK(padding, 0));
    PUT(bp + size - DSIZE, PACK(padding, 0));
    coalesce(SIZE_TO_NEXT_BLKP(bp, asize));
  } else {
    PUT(HDRP(bp), PACK(size, 1));
    PUT(bp + size - DSIZE, PACK(size, 1));
  }
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  if (((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)) {
    return -1;
  }
  PUT(heap_listp, 0);
  PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
  PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
  PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
  heap_listp += (2 * WSIZE);

  prev_listp = heap_listp;

  if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
    return -1;
  return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
  size_t asize;
  size_t extendsize;
  char *bp;
  if (size == 0) {
    return NULL;
  }

  if (size <= DSIZE) {
    asize = 2 * DSIZE;
  } else {
    asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
  }

  if ((bp = next_fit(asize)) != NULL) {
    place(bp, asize);
    return bp;
  }

  extendsize = MAX(asize, CHUNKSIZE);
  if ((bp = extend_heap(extendsize / WSIZE)) == NULL) {
    return NULL;
  }

  place(bp, asize);
  return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
  size_t size = GET_SIZE(HDRP(ptr));
  void *coal_ptr;

  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));

  coal_ptr = coalesce(ptr);

  if (coal_ptr < prev_listp) {
    prev_listp = coal_ptr;
  }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
  void *oldptr = ptr;
  void *newptr;
  size_t copySize;
  size_t asize;

  if (size <= DSIZE) {
    asize = 2 * DSIZE;
  } else {
    asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
  }

  copySize = GET_SIZE(HDRP(oldptr));
  if (size == copySize) {
    return ptr;
  } else if (copySize > asize) {
    place(oldptr, asize);
    return ptr;
  }
  newptr = SIZE_TO_NEXT_BLKP(ptr, copySize);

  if (!GET_ALLOC(HDRP(newptr)) &&
      (copySize += (GET_SIZE(HDRP(newptr)))) >= asize) {
    PUT(HDRP(oldptr), PACK(copySize, 1));
    place(oldptr, asize);
    return oldptr;
  }

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
