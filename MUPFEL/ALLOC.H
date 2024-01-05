/*
 * alloc.h - definitions for alloc.c
 * 05.08.90
 *
 * 24.01.91 - new function umalloc for unsafe mallocs (jr)
 */

#ifndef _M_ALLOC
#define _M_ALLOC
 
typedef unsigned long size_t;

#define free0(x)	free(x),x=NULL

void *malloc(size_t size);	/* allocate memory */
void *umalloc (size_t size);	/* same, allow NULL return */
void *calloc(size_t nitems, size_t size);	/* alloc nitems*size */
void *ucalloc (size_t nitems, size_t size); /* see above */
void *realloc(void *block, size_t newsize);	/* chg size of block */
void *urealloc (void *block, size_t newsize); /* see above */
void free(void *mem);	/* free memory */
int alloccheck(void);	/* check for unfreed blocks */
void allocinit(void);	/* initialize alloc table */
void allocexit(void);	/* Mfree allocated memory */
size_t coreleft(void);	/* What is free in my allocated blocks? */

#endif