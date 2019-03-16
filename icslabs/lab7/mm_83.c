/* 
 * Simple, 32-bit and 64-bit clean allocator based on implicit free
 * lists, first-fit placement, and boundary tag coalescing, as described
 * in the CS:APP3e text. Blocks must be aligned to doubleword (8 byte) 
 * boundaries. Minimum block size is 16 bytes. 
 * 
 * This is the implementation with explicit free lists and segragated fits.
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
static char *heap_listp = 0;  /* Pointer to first block */
static char *class0 = NULL;
static char *class1 = NULL;
static char *class2 = NULL;
static char *class3 = NULL;
static char *class4 = NULL;
static char *class5 = NULL;
static char *class6 = NULL;
static char *class7 = NULL;
static char *class8 = NULL;
static char *class9 = NULL;

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void free_from_list(void *bp);
static void attatch_to_list(void *bp);
static void write_class_by_num(int num, void *bp);
static int get_class_by_size(size_t size);
static void *retrive_class_by_num(int num);
static void check_free_list();

/* 
 * mm_init - Initialize the memory manager 
 */
int mm_init(void) 
{
    class0 = NULL;
    class1 = NULL;
    class2 = NULL;
    class3 = NULL;
    class4 = NULL;
    class5 = NULL;
    class6 = NULL;
    class7 = NULL;
    class8 = NULL;
    class9 = NULL;

    /* Create the initial empty heap */
    // printf("init:%d\n", 1);
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));
    heap_listp += (2*WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) 
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block with at least size bytes of payload 
 */
void *mm_malloc(size_t size) 
{
    // printf("malloc:%d\n", 1);
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;      

    if (heap_listp == 0){
        mm_init();
    }
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
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
    // printf("free:%d\n", 1);
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
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
static void *coalesce(void *bp) 
{
    // printf("coalesce:%d\n", 1);
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // printf("prev_alloc: %ld, next_alloc: %ld \n", prev_alloc, next_alloc);
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

    /* If size == 0 then this is just free, and we return NULL. */
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
 * The remaining routines are internal helper routines 
 */

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 */
static void *extend_heap(size_t words) 
{
    // printf("extend:%d\n", 1);
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
    // printf("place \n");
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
    // printf("find fit\n");
    /* First-fit search */
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
    if (num == -1) return NULL;
    while (num <= 9) {
        listp = retrive_class_by_num(num);
        while (listp != NULL) {
            if (asize <= GET_SIZE(HDRP(listp))) return listp;
            listp = NEXT_BLKP_CHAIN(listp);
        } 
        num++;
    }
    return NULL; /* No fit */
}

static void free_from_list(void *bp) {
    // printf("free from list\n");
    void *cur_bp;
    void *prev_bp;
    int num;

    size_t size = GET_SIZE(HDRP(bp));
    num = get_class_by_size(size);
    cur_bp = retrive_class_by_num(num);
    if (cur_bp == NULL) {
        // printf("free from list failed\n");
        // check_free_list();
        return;
    } else {
        if (cur_bp == bp) {
            write_class_by_num(num, NEXT_BLKP_CHAIN(bp));
            // check_free_list();
            return;
        }
        prev_bp = cur_bp;
        cur_bp = NEXT_BLKP_CHAIN(cur_bp);
        while (cur_bp != NULL) {
            if (cur_bp == bp) {
                PUT_SUC(prev_bp, NEXT_BLKP_CHAIN(bp));
                // check_free_list();
                return;
            }
            prev_bp = cur_bp;
            cur_bp = NEXT_BLKP_CHAIN(cur_bp);
        }
    }
    // printf("free from list failed\n");
}

static void attatch_to_list(void *bp) {
    // printf("attatch to list\n");
    void *listp;
    int num;
    size_t size = GET_SIZE(HDRP(bp));
    num = get_class_by_size(size);
    listp = retrive_class_by_num(num);
    if (listp == NULL) {
        write_class_by_num(num, bp);
        PUT_SUC(bp, NULL);
        // check_free_list();
        return;
    } else {
        PUT_SUC(bp, listp);
        write_class_by_num(num, bp);
        // check_free_list();
        return;
    }
}

static void check_free_list() {
    void *listp;
    size_t blksize;

    listp = class0;
    printf("%d freelist: ", 0);
    while (listp != NULL) {
        blksize = GET_SIZE(HDRP(listp));
        printf("-> %ld ",blksize);
        listp = NEXT_BLKP_CHAIN(listp);
    }
    printf(" -> null.\n"); 

    listp = class1;
    printf("%d freelist: ", 1);
    while (listp != NULL) {
        blksize = GET_SIZE(HDRP(listp));
        printf("-> %ld ",blksize);
        listp = NEXT_BLKP_CHAIN(listp);
    }
    printf(" -> null.\n"); 

    listp = class2;
    printf("%d freelist: ", 2);
    while (listp != NULL) {
        blksize = GET_SIZE(HDRP(listp));
        printf("-> %ld ",blksize);
        listp = NEXT_BLKP_CHAIN(listp);
    }
    printf(" -> null.\n"); 

    listp = class3;
    printf("%d freelist: ", 3);
    while (listp != NULL) {
        blksize = GET_SIZE(HDRP(listp));
        printf("-> %ld ",blksize);
        listp = NEXT_BLKP_CHAIN(listp);
    }
    printf(" -> null.\n"); 

    listp = class4;
    printf("%d freelist: ", 4);
    while (listp != NULL) {
        blksize = GET_SIZE(HDRP(listp));
        printf("-> %ld ",blksize);
        listp = NEXT_BLKP_CHAIN(listp);
    }
    printf(" -> null.\n"); 

    listp = class5;
    printf("%d freelist: ", 5);
    while (listp != NULL) {
        blksize = GET_SIZE(HDRP(listp));
        printf("-> %ld ",blksize);
        listp = NEXT_BLKP_CHAIN(listp);
    }
    printf(" -> null.\n"); 

    listp = class6;
    printf("%d freelist: ", 6);
    while (listp != NULL) {
        blksize = GET_SIZE(HDRP(listp));
        printf("-> %ld ",blksize);
        listp = NEXT_BLKP_CHAIN(listp);
    }
    printf(" -> null.\n"); 

    listp = class7;
    printf("%d freelist: ", 7);
    while (listp != NULL) {
        blksize = GET_SIZE(HDRP(listp));
        printf("-> %ld ",blksize);
        listp = NEXT_BLKP_CHAIN(listp);
    }
    printf(" -> null.\n"); 

    listp = class8;
    printf("%d freelist: ", 8);
    while (listp != NULL) {
        blksize = GET_SIZE(HDRP(listp));
        printf("-> %ld ",blksize);
        listp = NEXT_BLKP_CHAIN(listp);
    }
    printf(" -> null.\n"); 

    listp = class9;
    printf("%d freelist: ", 9);
    while (listp != NULL) {
        blksize = GET_SIZE(HDRP(listp));
        printf("-> %ld ",blksize);
        listp = NEXT_BLKP_CHAIN(listp);
    }
    printf(" -> null.\n"); 
}

static int get_class_by_size(size_t size) {
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
    
    return num;
    
}

static void write_class_by_num(int num, void *bp) {
    switch (num) {
        case 0:  class0 = bp; break;
        case 1:  class1 = bp; break;
        case 2:  class2 = bp; break;
        case 3:  class3 = bp; break;
        case 4:  class4 = bp; break;
        case 5:  class5 = bp; break;
        case 6:  class6 = bp; break;
        case 7:  class7 = bp; break;
        case 8:  class8 = bp; break;
        case 9:  class9 = bp; break;
        default: return;
    }
    // if (num == 0) {
    //     class0 = bp;
    // }
    // if (num == 1) {
    //     class1 = bp;
    // }
    // if (num == 2) {
    //     class2 = bp;
    // }
    // if (num == 3) {
    //     class3 = bp;
    // }
    // if (num == 4) {
    //     class4 = bp;
    // }
    // if (num == 5) {
    //     class5 = bp;
    // }
    // if (num == 6) {
    //     class6 = bp;
    // }
    // if (num == 7) {
    //     class7 = bp;
    // }
    // if (num == 8) {
    //     class8 = bp;
    // }
    // if (num == 9) {
    //     class9 = bp;
    // }
}

static void *retrive_class_by_num(int num) {
    switch (num) {
        case 0:  return class0;
        case 1:  return class1;
        case 2:  return class2;
        case 3:  return class3;
        case 4:  return class4;
        case 5:  return class5;
        case 6:  return class6;
        case 7:  return class7;
        case 8:  return class8;
        case 9:  return class9;
        default: return NULL;
    }
    // if (num == 0) {
    //     return class0;
    // }
    // if (num == 1) {
    //     return class1;
    // }
    // if (num == 2) {
    //     return class2;
    // }
    // if (num == 3) {
    //     return class3;
    // }
    // if (num == 4) {
    //     ;return class4;
    // }
    // if (num == 5) {
    //     return class5;
    // }
    // if (num == 6) {
    //     return class6;
    // }
    // if (num == 7) {
    //     return class7;
    // }
    // if (num == 8) {
    //     return class8;
    // }
    // if (num == 9) {
    //     return class9;
    // }
    // return NULL;
}