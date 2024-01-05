/*
 * linklist.h - definitions for linklist.c
 * 28.10.89
 */

#ifndef _M_LINKLIST
#define _M_LINKLIST

#ifndef LL_ENTRY
#error "linked list structure undefined"
#endif

typedef int (*ENTRYFUNC1)(LL_ENTRY *e);
typedef int (*ENTRYFUNC2)(LL_ENTRY *e1,LL_ENTRY *e2);

void insert(LL_ENTRY *newentry, LL_ENTRY **head, size_t size);

void append(LL_ENTRY *newentry, LL_ENTRY **head, size_t size);

LL_ENTRY *sortlist(LL_ENTRY *list, int count, ENTRYFUNC2 cmp);

void walklist(LL_ENTRY *start, ENTRYFUNC1 func);

void freelist(LL_ENTRY *start, ENTRYFUNC1 func);

LL_ENTRY *search(LL_ENTRY *head, LL_ENTRY *what,ENTRYFUNC2 eq);

LL_ENTRY *delete(LL_ENTRY **head, LL_ENTRY *what,ENTRYFUNC2 eq,
	ENTRYFUNC1 func);

#endif