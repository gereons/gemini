/*
 * linklist.c  -  linked list management
 * 28.10.89
 */
 
#include <string.h>
#include "alloc.h"
#include "mupfel.h"

SCCS(linklist);

typedef struct entry
{
	struct entry *next;
} entry;

#define LL_ENTRY entry
#include "linklist.h"

static entry **parray;

/*
 * void insert(entry *newentry, entry **head, size_t size)
 * Insert a new entry at the start of the linked list pointed to by
 * *head. *head will point to the new entry.
 */
void insert(entry *newentry, entry **head, size_t size)
{
	entry *new = malloc(size);

	memcpy(new,newentry,size);

	if (*head == NULL) /* first time called ? */
		new->next = NULL;
	else
		new->next = *head;
	*head = new;
}

/*
 * void append(entry *newentry, entry **head, size_t size)
 * Append a new entry to the linked list pointed to by
 * *head. *head will point to the new entry if the list was empty.
 */
void append(entry *newentry, entry **head, size_t size)
{
	entry *new = malloc(size);
	entry *h;

	memcpy(new,newentry,size);	
	new->next = NULL;
	
	if (*head == NULL)
		*head = new;
	else
	{
		h = *head;
		while (h->next != NULL)
			h = h->next;
		h->next = new;
	}
}

/*
 * exchange two elements of parray
 */
static void swap(int i, int j)
{
	entry *tmp;

	if (i==j)
		return;
	tmp  = parray[i];
	parray[i] = parray[j];
	parray[j] = tmp;
}

/*
 * quicksort parray
 */
static void doqsort(int left,int right,ENTRYFUNC2 cmp)
{
	int i, last;

	if (left >= right)
		return;

	swap(left,(left+right)/2);
	last = left;
	for (i=left+1; i<=right; i++)
		if (cmp(parray[i],parray[left])<0)
			swap(++last,i);
	swap(left,last);
	doqsort(left,last-1,cmp);
	doqsort(last+1,right,cmp);
}

/*
 * entry *sortlist(entry *list,int cnt, int (*cmp)(entry *e1, entry *e2))
 * quicksort the linked list pointed to by list. Elements are ex-
 * changed when cmp returns a positive value. cnt is the number of
 * elements in the list, if it's 0, the number is determined by
 * sortlist.
 * Returns pointer to the new head.
 */
entry *sortlist(entry *list,int cnt,ENTRYFUNC2 cmp)
{
	entry *e;
	int i;
	
	/* no count specified? */
	if (cnt==0)
	{
		e = list;
		while (e)
		{
			++cnt;
			e = e->next;
		}
	}
	/* 1 element is always sorted */
	if (cnt==1)
		return list;

	parray = calloc(cnt,sizeof(entry *));

	/* fill parray with addresses */
	e = list;
	for (i=0; i<cnt; ++i)
	{
		parray[i] = e;
		e = e->next;
	}

	doqsort(0,cnt-1,cmp);

	/* put the sorted addresses back into the list */
	e = parray[0];
	for (i=0; i<cnt; ++i)
	{
		e->next = parray[i];
		e = e->next;
	}
	e->next = NULL;
	e = parray[0];
	free(parray);
	return e;
}

/*
 * void walklist(entry *start, int (*func)(entry *e))
 * Walk through the linked list pointed to by start, calling func 
 * for every entry. Stopped when func returns FALSE.
 */
void walklist(entry *start, ENTRYFUNC1 func)
{
	while (start != NULL && func(start))
		start = start->next;
}

/*
 * void freelist(entry *start)
 * Release memory for all entries in the list pointed to by start.
 * func should point to a function that frees memory for each
 * entry.
 */
void freelist(entry *start,ENTRYFUNC1 func)
{
	entry *old;
	
	while (start != NULL)
	{
		old = start;
		if (func)
			func(old);
		start = start->next;
		free(old);
	}
}

/*
 * entry *search(entry *head, entry *what,int (*eq)(entry *e1, entry *e2))
 * Search the list pointed to by head for an entry where eq(what,xx)==TRUE.
 * Returns a pointer to that entry or NULL if none found.
 */
entry *search(entry *head, entry *what,ENTRYFUNC2 eq)
{
	while (head != NULL)
	{
		if (eq(what,head))
			return head;
		head = head->next;
	}
	return NULL;
}

/*
 * entry *delete(entry **head, entry *what,
 *	int (*eq)(entry *e1, entry *e2),
 *	int (*func)(entry *e))
 * Delete the entry where eq(what,xx)==TRUE from the linked list pointed to
 * by head. func is called for that entry.
 * Returns a pointer to the new head, NULL if the list is now empty, 
 * or (entry *)-1 if no entry matches *what.
 */
entry *delete(entry **head, entry *what,ENTRYFUNC2 eq,
	ENTRYFUNC1 func)
{
	entry *prev, *h = *head;
	int found = FALSE;

	while (h != NULL)
	{
		if ((found = eq(what,h))==TRUE)
			break;
		prev = h;
		h = h->next;
	}
	if (!found)
		return (entry *)-1;

	if (h == *head)
		*head = (*head)->next;
	else
		prev->next = h->next;

	func(h);
	free(h);
	return *head;
}
