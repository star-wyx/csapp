/*
 *
 * Explicit Linked List + Next Fit
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
        /* Team name */
        "ateam",
        /* First member's full name */
        "Harry Bovik",
        /* First member's email address */
        "bovik@cs.cmu.edu",
        /* Second member's full name (leave blank if none) */
        "",
        /* Second member's email address (leave blank if none) */
        ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


/* Basic constants and macros */
#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double word size (bytes) */
#define CHUNKSIZE (1<<11) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
//#define PUT(p, val) (*(unsigned int *)(p) =(unsigned int *) (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

void *heap_head;
void *free_head;

void take_out(void *bp) {
    void *prev = GET(bp);
    void *next = GET(bp + WSIZE);
    PUT(bp, 0);
    PUT(bp + WSIZE, 0);
    if (prev != 0 && next != 0) {
        PUT(prev + WSIZE, next);
        PUT(next, prev);
    } else if (prev == 0 && next != 0) {
        PUT(next, 0);
        free_head = next;
    } else if (prev != 0 && next == 0) {
        PUT(prev + WSIZE, 0);
    } else if (prev == 0 && next == 0) {
        free_head = 0;
    }
}

void insert(void *bp) {
    PUT(bp, 0);
    PUT(bp + WSIZE, free_head);
    if (free_head != 0) {
        PUT(free_head, bp);
    }
    free_head = bp;
}

void place(void *bp, size_t asize) {
    size_t size = GET_SIZE(HDRP(bp));

    if ((size - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK((size - asize), 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK((size - asize), 0));
        take_out(bp);
        insert(NEXT_BLKP(bp));
    } else {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        take_out(bp);
    }

}

void *find(size_t asize) {
    void *bp = free_head;
    size_t size;
    size_t alloc;
    while (bp != 0 && (size = GET_SIZE(HDRP(bp))) > 0) {
        alloc = GET_ALLOC(HDRP(bp));
        if (!alloc && size >= asize)
            return bp;
        bp = GET(bp + WSIZE);
    }
    return NULL;
}

void *coalesce(void *bp) {
    size_t pre_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (pre_alloc && next_alloc) {
        insert(bp);
    } else if (pre_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        take_out(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        insert(bp);
    } else if (!pre_alloc && next_alloc) {
        size += GET_SIZE(FTRP(PREV_BLKP(bp)));
        take_out(PREV_BLKP(bp));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        insert(bp);
    } else {
        size = size + GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(FTRP(PREV_BLKP(bp)));
        take_out(PREV_BLKP(bp));
        take_out(NEXT_BLKP(bp));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        insert(bp);
    }
    return bp;
}

void *expand_heap(size_t words) {
    void *bp;
    words = (words % 2) ? words + 1 : words;
    if ((long) (bp = mem_sbrk(words * WSIZE)) == -1)
        return NULL;
    PUT(HDRP(bp), PACK(words * WSIZE, 0));
    PUT(FTRP(bp), PACK(words * WSIZE, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}


/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    if ((heap_head = mem_sbrk(4 * WSIZE)) == (void *) -1)
        return -1;
    PUT(heap_head, 0);
    PUT(heap_head + 1 * WSIZE, PACK(DSIZE, 1));
    PUT(heap_head + 2 * WSIZE, PACK(DSIZE, 1));
    PUT(heap_head + 3 * WSIZE, PACK(0, 1));
    heap_head += 2 * WSIZE;
    free_head = 0;
    if (expand_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    size_t asize;
    size_t expandsize;
    void *bp;

    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
//        asize = size - size % DSIZE + 2 * DSIZE;
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    if ((bp = find(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    expandsize = MAX(asize, CHUNKSIZE);
    if ((bp = expand_heap(expandsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
//    insert(ptr);

    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    if (ptr == NULL)
        return mm_malloc(size);
    if (size == 0)
        mm_free(ptr);

    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = GET_SIZE(HDRP(newptr));
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}