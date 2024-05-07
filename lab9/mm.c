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

#define MAX_BLOCK_SIZE 4096

#define LIST_SIZE 10

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - (WSIZE))
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define SIZE_TO_FTRP(bp, size) ((char *)(bp) + (size) - DSIZE)
#define SIZE_TO_NEXT_BLKP(bp, size) ((char *)(bp) + (size) - WSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp) - (WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - (GET_SIZE((char *)(bp) - (DSIZE))))

#define NEXT_PTR(bp, size) ((char *)(bp) + size)

#define SET_PREV(bp, offset) (*((unsigned int *)(bp) + 1) = (offset))

#define SET_SUCC(bp, offset) (*((unsigned int *)(bp)) = (offset))

#define GET_PREV(bp) (*((unsigned int *)(bp) + 1))

#define GET_SUCC(bp) (*((unsigned int *)(bp)))

#define LIST_ENTRY(index, offset) ((char *)heap_listp + 4 * WSIZE + offset)

#define MOVE_PTR(bp, offset) ((char *)bp + offset)

#define HEAP_HEAD (((char *)(heap_listp)) + LIST_SIZE + 2)

static void *heap_listp;

static int ch[10] = {0};

static int ceil_order(unsigned int num) {
    if (num <= 16) {
        return 0;
    } else if (num > 4096) {
        return LIST_SIZE - 1;
    }
    num -= 1;
    int r;
    size_t shift;
    r = (num > 0xFFFF) << 4;
    num >>= r;
    shift = (num > 0xFF) << 3;
    num >>= shift;
    r |= shift;
    shift = (num > 0xF) << 2;
    num >>= shift;
    r |= shift;
    shift = (num > 0x3) << 1;
    num >>= shift;
    r |= shift;
    r |= (num >> 1);

    return r - 3;
}

static int floor_order(unsigned int num) {
    if (num < 32) {
        return 0;
    } else if (num > 4096) {
        return LIST_SIZE - 1;
    }
    int r;
    size_t shift;
    r = (num > 0xFFFF) << 4;
    num >>= r;
    shift = (num > 0xFF) << 3;
    num >>= shift;
    r |= shift;
    shift = (num > 0xF) << 2;
    num >>= shift;
    r |= shift;
    shift = (num > 0x3) << 1;
    num >>= shift;
    r |= shift;
    r |= (num >> 1);

    return r - 4;
}

static void list_add(int index, char *ptr) {
    unsigned int offset = ptr - (char *) heap_listp;
    char *list_ptr = heap_listp + index * WSIZE;
    unsigned int old_offset = GET(list_ptr);
    SET_PREV(ptr, index * WSIZE);
    SET_SUCC(ptr, old_offset);
    PUT(list_ptr, offset);
    if (old_offset != 0)
        SET_PREV(list_ptr + old_offset, offset);
    ch[index]++;
}

static void list_drop(int index, unsigned int size) {
    char *old_ptr = heap_listp + size;
    char *list_ptr = heap_listp + index * WSIZE;
    // succ ptr
    ch[index]--;
    size = GET_SUCC(old_ptr);
    PUT(list_ptr, size);
    if (size != 0)
        SET_PREV(heap_listp + size, index * WSIZE);
}

static void drop_entry(char *entry) {
    unsigned int prev_size = GET_PREV(entry);
    unsigned int succ_size = GET_SUCC(entry);
    unsigned int size = GET_SIZE(HDRP(entry));

    SET_SUCC(MOVE_PTR(heap_listp, prev_size), succ_size);
    if (succ_size != 0)
        SET_PREV(MOVE_PTR(heap_listp, succ_size), prev_size);
}

static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    int index;

    if (prev_alloc && next_alloc) {
        list_add(floor_order(size), bp);
        return bp;
    } else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        ch[floor_order(GET_SIZE(HDRP(NEXT_BLKP(bp))))]--;
        index = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        drop_entry(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        ch[floor_order(GET_SIZE(HDRP(PREV_BLKP(bp))))]--;
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        drop_entry(bp);
    } else {
        drop_entry(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        ch[floor_order(GET_SIZE(HDRP(PREV_BLKP(bp))))]--;
        ch[floor_order(GET_SIZE(FTRP(NEXT_BLKP(bp))))]--;

        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        drop_entry(bp);
    }

    list_add(floor_order(size), bp);

    return bp;
}

static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    size = (words + (words & 1)) * WSIZE;

    if (((long) (bp = mem_sbrk(size))) == -1) {
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return bp;
}

// bp is the found ptr ,b_size is needed size
static void split(char *bp, unsigned int b_size) {
    unsigned int size = GET_SIZE(HDRP(bp));
    size -= b_size;
    if (size < DSIZE) {
        PUT(HDRP(bp), PACK(size + b_size, 1));
        PUT(SIZE_TO_FTRP(bp, size + b_size), PACK(size + b_size, 1));
    } else {
        PUT(HDRP(bp), PACK(b_size, 1));
        PUT(SIZE_TO_FTRP(bp, b_size), PACK(b_size, 1));

        SIZE_TO_NEXT_BLKP(bp, b_size);

        PUT(HDRP(bp), PACK(size, 0));
        PUT(SIZE_TO_FTRP(bp, size), PACK(size, 0));

        list_add(floor_order(size), bp);
    }
}

static void *find_fit(size_t asize) {
    unsigned int b_size;
    char *bp;
    if (asize > MAX_BLOCK_SIZE) {
        bp = heap_listp + 9 * WSIZE;
        b_size = GET(bp);
        while (b_size) {
            bp = MOVE_PTR(heap_listp, b_size);
            if (GET_SIZE(HDRP(bp)) >= asize) {
                ch[9]--;
                drop_entry(bp);
                split(bp, asize);
                return bp;
            }
            b_size = GET_SUCC(bp);
        }
    } else {
        int order = ceil_order(asize);
        asize = ((1 << (order + 4)));
        for (; order < LIST_SIZE; ++order) {
            b_size = GET(heap_listp + order * WSIZE);
            if (b_size) {
                bp = MOVE_PTR(heap_listp, b_size);
                list_drop(order, b_size);
                split(bp, asize);
                return bp;
            }
        }
    }
    return NULL;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    char *bp;
    if (((heap_listp = mem_sbrk(14 * WSIZE)) == (void *) -1)) {
        return -1;
    }

    PUT(heap_listp, 0);                             // size <= 16
    PUT(heap_listp + (1 * WSIZE), 0);               // size <= 32
    PUT(heap_listp + (2 * WSIZE), 0);               // size <= 64
    PUT(heap_listp + (3 * WSIZE), 0);               // size <= 128
    PUT(heap_listp + (4 * WSIZE), 0);               // size <= 256
    PUT(heap_listp + (5 * WSIZE), 0);               // size <= 512
    PUT(heap_listp + (6 * WSIZE), 0);               // size <= 1024
    PUT(heap_listp + (7 * WSIZE), 0);               // size <= 2048
    PUT(heap_listp + (8 * WSIZE), 0);               // size <= 4096
    PUT(heap_listp + (9 * WSIZE), 0);               // size > 4096
    PUT(heap_listp + (10 * WSIZE), 0);              // alignment padding
    PUT(heap_listp + (11 * WSIZE), PACK(DSIZE, 1)); // prologue header
    PUT(heap_listp + (12 * WSIZE), PACK(DSIZE, 1)); // prologue footer
    PUT(heap_listp + (13 * WSIZE), PACK(0, 1));     // epilogue header

    if ((bp = extend_heap(CHUNKSIZE / WSIZE)) == NULL)
        return -1;

    list_add(6, bp);
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    printf("\nmalloc\n");
    size_t asize;
    size_t extendsize;
    char *bp;
    if (size == 0) {
        return NULL;
    }

    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    } else {
        asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
    }

    if ((bp = find_fit(asize)) != NULL) {
        // place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);

    if ((bp = extend_heap(extendsize / WSIZE)) == NULL) {
        return NULL;
    }

    split(bp, asize);
    // list_add(ceil_order(extendsize), bp);

    // place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    printf("\nfree\n");
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    SET_PREV(ptr, 0);
    SET_SUCC(ptr, 0);
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    // size_t asize;

    // if (size <= DSIZE) {
    //   asize = 2 * DSIZE;
    // } else {
    //   asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    // }

    // copySize = GET_SIZE(HDRP(oldptr));
    // if (size == copySize) {
    //   return ptr;
    // } else if (copySize > asize) {
    //   place(oldptr, asize);
    //   return ptr;
    // }
    // newptr = SIZE_TO_NEXT_BLKP(ptr, copySize);

    // if (!GET_ALLOC(HDRP(newptr)) &&
    //     (copySize += (GET_SIZE(HDRP(newptr)))) >= asize) {
    //   PUT(HDRP(oldptr), PACK(copySize, 1));
    //   place(oldptr, asize);
    //   return oldptr;
    // }

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *) ((char *) oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
