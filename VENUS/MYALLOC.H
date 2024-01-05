/*
 * myalloc.h
 *
 * project: venus
 *
 * author: stefan eissing
 *
 * description: Header File for myalloc.c
 *
 * last change: 17.04.1989
 */

void *malloc(size_t nbytes);
void free(void *ap);
void *tmpmalloc(size_t nbytes);
void *tmpcalloc(size_t items, size_t size);
void tmpfree(void *ap);
void freeSysBlocks(void);
void freeTmpBlocks(void);
