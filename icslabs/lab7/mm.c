/* 
 * Simple, 32-bit and 64-bit clean allocator based on implicit free
 * lists, first-fit placement, and boundary tag coalescing, as described
 * in the CS:APP3e text. Blocks must be aligned to doubleword (8 byte) 
 * boundaries. Minimum block size is 16 bytes. 
 * 
 * This is the implementation with explicit free lists and segragated fits.
 * I used another 10 * 8 bytes space for my 10 segragated lists, 
 * And each block contains a header, footer and a 8-byte ptr, pointing 
 * to the next free block on the list.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mm.h"
#include "memlib.h"

/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* Read and write pointer at address p */
#define GET_SUC(p)       ((*(long *)(p)))
#define PUT_SUC(p, ptr)  (*(long *)(p) = (long)(ptr))
#define GET_LIST(p)      ((void *)GET_SUC(p))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
#define NEXT_BLKP_CHAIN(bp) ((void *)GET_SUC(bp))

/* Global variables */
static char *heap_listp = 0;
static char *class_listp = 0;

/* helper function*/
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void free_from_list(void *bp);
static void attatch_to_list(void *bp);
static void *get_class(size_t size);

/* 
 * mm_init - Initialize
 */
int mm_init(void) 
{
    void *class;
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(10*DSIZE+4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (10*DSIZE+1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (10*DSIZE+2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (10*DSIZE+3*WSIZE), PACK(0, 1));
    class_listp = heap_listp;
    heap_listp += (10*DSIZE+2*WSIZE);

    /* Initialize class_list */
    for (int i=0; i<10; i++) {
        class = (char *)class_listp + i * DSIZE;
        PUT_SUC(class, NULL);
    }

    /* Extend the empty heap with chunksize bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) 
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block according to the size. 
 */
void *mm_malloc(size_t size) 
{
    size_t asize;
    size_t extendsize;
    char *bp;      

    if (heap_listp == 0 || class_listp == 0){
        mm_init();
    }
    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found*/
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)  
        return NULL;
    place(bp, asize);
    return bp;
} 

/* 
 * mm_free - Free a block 
 */
void mm_free(void *bp)
{
    if (bp == 0) 
        return;

   
    size_t size = GET_SIZE(HDRP(bp));
    
    if (heap_listp == 0){
        mm_init();
    }

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * coalesce - Boundary tag coalescing and add the coalesced block to free lists. Return ptr to coalesced block
 */
static void *coalesce(void *bp) 
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && !next_alloc) {            /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        free_from_list(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        free_from_list(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else if (!prev_alloc && !next_alloc) {     /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        free_from_list(PREV_BLKP(bp));
        free_from_list(NEXT_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    /* attatch the coalesced block to the free list */
    attatch_to_list(bp);
    return bp;
}

/*
 * mm_realloc - Naive implementation of realloc
 */
void *mm_realloc(void *ptr, size_t size)
{
    size_t oldsize;
    void *newptr;

    if(size == 0) {
        mm_free(ptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(ptr == NULL) {
        return mm_malloc(size);
    }

    newptr = mm_malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if(!newptr) {
        return 0;
    }

    /* Copy the old data. */
    oldsize = GET_SIZE(HDRP(ptr));
    if(size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    mm_free(ptr);

    return newptr;
}

/* 
 * The remaining routines are helper functions
 */

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 */
static void *extend_heap(size_t words) 
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)  
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ 

    /* Coalesce if the previous block was free */
    return coalesce(bp);                                        
}

/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));   
    free_from_list(bp);
    void *split_bp;

    if ((csize - asize) >= (2*DSIZE)) { 
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        split_bp = NEXT_BLKP(bp);
        PUT(HDRP(split_bp), PACK(csize-asize, 0));
        PUT(FTRP(split_bp), PACK(csize-asize, 0));
        attatch_to_list(split_bp);
    }
    else { 
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}


/* 
 * find_fit - Find a fit for a block with asize bytes 
 */
static void *find_fit(size_t asize)
{
    /* First-fit search */
    void *class;
    void *listp;

    int num = -1;
    for (int i=0; i<=8; i++) {
        if (asize < (1<<(i+5))) {
            num = i;
            break;
        }
    }
    if (asize >= (1<<13)) {
        num = 9;
    }
    
    while (num <= 9) {
        class = (void *)((char *)class_listp + num * DSIZE);
        listp = GET_LIST(class);
        while (listp != NULL) {
            if (asize <= GET_SIZE(HDRP(listp))) return listp;
            listp = NEXT_BLKP_CHAIN(listp);
        } 
        num++;
    }
    return NULL; /* No fit */
}

/* 
 * free_from_list - cut a block from  free block chain.
 */
static void free_from_list(void *bp) {
    void *class;
    void *listp;
    void *cur_bp;
    void *prev_bp;

    size_t size = GET_SIZE(HDRP(bp));
    class = get_class(size);
    listp = GET_LIST(class);
    cur_bp = listp;
    if (listp == NULL) {
        return;
    } else {
        if (cur_bp == bp) {
            listp = NEXT_BLKP_CHAIN(bp);
            PUT_SUC(class, listp);
            return;
        }
        prev_bp = cur_bp;
        cur_bp = NEXT_BLKP_CHAIN(cur_bp);
        while (cur_bp != NULL) {
            if (cur_bp == bp) {
                PUT_SUC(prev_bp, NEXT_BLKP_CHAIN(bp));
                return;
            }
            prev_bp = cur_bp;
            cur_bp = NEXT_BLKP_CHAIN(cur_bp);
        }
    }
}

/* 
 * attatch_to_list - attatch freed block onto free lists
 */
static void attatch_to_list(void *bp) {
    void *class;
    void *listp;
    size_t size = GET_SIZE(HDRP(bp));
    class = get_class(size);
    listp = GET_LIST(class);
    if (listp == NULL) {
        listp = bp;
        PUT_SUC(bp, NULL);
        PUT_SUC(class, listp);
        return;
    } else {
        PUT_SUC(bp, listp);
        listp = bp;
        PUT_SUC(class, listp);
        return;
    }
}

/* 
 * get_class - get class list ptr according to size.
 */
static void *get_class(size_t size) {
    int num = -1;
    for (int i=0; i<=8; i++) {
        if (size < (1<<(i+5))) {
            num = i;
            break;
        }
    }
    if (size >= (1<<13)) {
        num = 9;
    }
    
    if (num == -1) {
        return NULL;
    }
    return (void *)((char *)(class_listp) + num * DSIZE);
}