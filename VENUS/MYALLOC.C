/*
 * @(#) Gmni\MyAlloc.c
 *
 * the Storage Allocator from K&R
 * a little bit modified by Stefan Eissing
 * 03.04.1991
 */

#include <string.h>
#include <tos.h>
#include <nls\nls.h>

#include "vs.h"
#include "venuserr.h"
#include "stand.h"

/* internal texts
 */
#define NlsLocalSection		"Gmni.myalloc"
enum NlsLocalText{
T_ERROR,		/*Kann nicht genug Speicher bekommen! Seien sie
 vorsichtig im weitern Umgang mit dem Programm!*/
};

#define SHOWMORECORE	FALSE	/* alerts on call to morecore */

#define NALLOC		1024L		/* minimum #units to request */
#define MSTACKSIZE	50			/* maximum of morecores in malloc */

struct header
{
	struct header *ptr;
	size_t size;	
};
typedef struct header Header;

static Header base;				/* empty list to get startet */
static Header *freep = NULL;	/* start of free list */
static word index = 0;
static void **mallocStack;

#if STANDALONE
static Header sysBase;			/* variables for permanent malloc */
static Header *sysFreep = NULL;
static word sysindex = 0;
static void *sysMallocStack[MSTACKSIZE];
#endif

static Header tmpBase;			/* variables for temporary malloc */
static Header *tmpFreep = NULL;
static word tmpindex = 0;
static void *tmpMallocStack[MSTACKSIZE];
	

/* put Block ap in free list
*/
static void myfree(void *ap)
{
	Header *bp, *p;
	
	bp = (Header *)ap -1;		/* pointer to block Header */

	for(p = freep; !(bp > p && bp < p->ptr); p = p->ptr)
		if(p >= p->ptr && (bp > p || bp < p->ptr))
			break;	/* freed block at start or end of arena */
	
	if(bp + bp->size == p->ptr)	/* join to upper */
	{
		bp->size += p->ptr->size;
		bp->ptr = p->ptr->ptr;
	}
	else
		bp->ptr = p->ptr;
	if(p + p->size == bp)		/* join to lower */
	{
		p->size += bp->size;
		p->ptr = bp->ptr;
	}
	else
		p->ptr = bp;
	freep = p;
}

	

/* fetch nu * sizeof(Header) Bytes from TOS
*/
static Header *morecore(size_t nu)
{
	Header *up;

/*	form_alert(1,"[1][morecore called.][ok]");
 */
	if(nu < NALLOC)
		nu = NALLOC;
	up = Malloc(nu * sizeof(Header));
	if(up == NULL)
		return NULL;
	up->size = nu;
	
	if(index > MSTACKSIZE)			/* remember block malloced */
		venusDebug("Stack Overflow in morecore!");
	else
		mallocStack[index++] = up;

	myfree((void *)(up+1));
	return freep;
}

/* allocate Storage of n bytes
*/
static void *myalloc(size_t nbytes)
{
	Header *p, *prevp;
	long nunits;
	
	nunits = (nbytes+sizeof(Header)-1)/sizeof(Header) + 1;
	if((prevp = freep) == NULL)	/* no free list yet */
	{
		base.ptr = freep = prevp = &base;
		base.size = 0;
	}
	for(p = prevp->ptr; ; prevp = p, p = p->ptr)
	{
		if(p->size >= nunits)	/* big enough */
		{
			if(p->size == nunits)	/* exactly */
				prevp->ptr = p->ptr;
			else
			{
				p->size -= nunits;
				p += p->size; 
				p->size = nunits;
			}
			freep = prevp;

			return (void *)(p+1);
		}
		if(p == freep)		/* wrapped around the free list */
			if((p = morecore(nunits)) == NULL)
			{
				venusErr(NlsStr (T_ERROR));
				return NULL;	/* none left */
			}
	}
}

#if STANDALONE
void *malloc(size_t nbytes)
{
	void *p;
	
	base = sysBase;				/* set global variables for */
	freep = sysFreep;			/* permanent malloc */
	index = sysindex;
	mallocStack = sysMallocStack;
	
	p = myalloc(nbytes);
	
	sysBase = base;				/* and restore them */
	sysFreep = freep;

#if SHOWMORECORE
	if(sysindex != index)
		venusErr("morecore in malloc()");
#endif

 	sysindex = index;
	return p;
}

void *calloc(size_t nitems, size_t size)	/* alloc nitems*size */
{
	void *p;
	size_t nbytes = nitems * size;
	
	p = malloc(nbytes);
	if (p)
		memset(p, 0, nbytes);
	
	return p;
}

void free(void *ap)
{
	base = sysBase;				/* set global variables for */
	freep = sysFreep;			/* permanent malloc */

	myfree(ap);
	
	sysBase = base;
	sysFreep = freep;
}

static size_t getsize(void *m)
{
	Header *mp = (Header *)m-1;
	
	return (mp->size-1) * sizeof(Header);
}

void *realloc(void *block, size_t newsize)
{
	void *m;

	if (block==NULL)
		return malloc(newsize);

	if ((m=malloc(newsize))==NULL)
		return NULL;
	else
	{
		memcpy(m,block,min(newsize,getsize(block)));
		free(block);
		return m;
	}
}

#endif

void *tmpmalloc(size_t nbytes)
{
	void *p;
	
	base = tmpBase;				/* set global variables for */
	freep = tmpFreep;			/* temporary malloc */
	index = tmpindex;
	mallocStack = tmpMallocStack;
	
	p = myalloc(nbytes);
	
	tmpBase = base;				/* and restore them */
	tmpFreep = freep;

#if SHOWMORECORE
	if(tmpindex != index)
		venusErr("morecore in tmpmalloc()");
#endif

	tmpindex = index;
	return p;
}

void *tmpcalloc(size_t items, size_t size)
{
	void *p;
	size_t len = items * size;
	
	p = tmpmalloc(len);
	if (p)
		memset(p, 0, len);
	return p;
}

void tmpfree(void *ap)
{
	base = tmpBase;				/* set global variables for */
	freep = tmpFreep;			/* temporary malloc */

	myfree(ap);

	tmpBase = base;
	tmpFreep = freep;
}

/*
 * void freeTmpBlocks(void)
 * free all Gemdos Memoryblocks allocated through tmpmalloc()
 */
void freeTmpBlocks(void)
{
	while(tmpindex > 0)
	{
		if(Mfree(tmpMallocStack[tmpindex-1]))
			venusDebug("Mfree in tmpfree failed!");
		tmpindex--;
	}
	tmpFreep = NULL;
}

#if STANDALONE
/*
 * void freeSysBlocks(void)
 * free all Gemdos Memoryblocks allocated through malloc()
 */
void freeSysBlocks(void)
{
	while(sysindex > 0)
	{
		if(Mfree(sysMallocStack[sysindex-1]))
			venusDebug("Mfree in sysfree failed!");
		sysindex--;
	}
	sysFreep = NULL;
}
#endif