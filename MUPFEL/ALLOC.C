/*
 * alloc.c  -  K&R memory allocation with usage checks
 * 01.10.90
 *
 * 25.01.91 new function umalloc for unsafe mallocs (jr)
 */
 
#include <string.h>
#include <tos.h>

#include "alert.h"
#include "alloc.h"
#include "chario.h"
#include "exit.h"
#include "mupfel.h"
#include "sysvec.h"

#define SYSSTACK	50		/* size of sysmalloc array */
#define NALLOC		2048		/* NALLOC * sizeof(Header) => 
						 * smallest arg for Malloc()
						 */

#define ALLOCCHECK	 1		/* Include debugging code */
#define ALLOCABORT	 1		/* Terminate if malloc fails */
#define SHOWMORECORE 0		/* Show morecore() calls */

#if ALLOCCHECK
#define MAXALLOC	1024

static struct
{
	void *addr;
 	size_t len;
} alloctab[MAXALLOC];
#endif

SCCS(alloc);

typedef struct header
{
	struct header *ptr;
	size_t size;
} Header;

static Header base;
static Header *freep = NULL;

static void *sysmalloc[SYSSTACK];
static int mstack = 0;

static void *mymalloc(size_t nbytes)
{
	Header *p, *prevp;
	Header *morecore(size_t nu);
	size_t nunits;
	
	nunits = (nbytes+sizeof(Header)-1)/sizeof(Header) + 1;
	if ((prevp=freep)==NULL)
	{
		base.ptr = freep = prevp = &base;
		base.size = 0;
	}
	for (p=prevp->ptr; ; prevp=p, p=p->ptr)
	{
		if (p->size >= nunits)
		{
			if (p->size == nunits)
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
		if (p==freep)
		{
			if ((p=morecore(nunits))==NULL)
				return NULL;
		}
	}
}

static void myfree(void *ap)
{
	Header *bp, *p;

	bp = (Header *)ap-1;

	for (p=freep; !(bp > p && bp < p->ptr); p = p->ptr)
		if (p >= p->ptr && (bp > p || bp < p->ptr))
			break;

	if (bp + bp->size == p->ptr)
	{
		bp->size += p->ptr->size;
		bp->ptr = p->ptr->ptr;
	}
	else
		bp->ptr = p->ptr;

	if (p + p->size == bp)
	{
		p->size += bp->size;
		p->ptr = bp->ptr;
	}
	else
		p->ptr = bp;

	freep = p;
}

static Header *morecore(size_t nu)
{
	void *cp;
	Header *up;
	BASPAG *currbp, bpbuf;
	extern BASPAG *_BasPag;

	if (nu<NALLOC)
		nu=NALLOC;

	if (shellcmd)
	{
		currbp = getactpd();
		bpbuf = *_BasPag;
		setactpd(_BasPag);
	}
	cp = Malloc(nu*sizeof(Header));
	if (shellcmd)
	{
		setactpd(currbp);
		*_BasPag = bpbuf;
	}
		
	if (cp==NULL)
		return NULL;

#if SHOWMORECORE
	alert(1,1,1,"Malloc(%ld)=%p","OK",nu*sizeof(Header),cp);
#endif

	if (mstack < SYSSTACK)
		sysmalloc[mstack++] = cp;
	else
		alertstr("Malloc-Stack overflow");
	
	up = (Header *)cp;
	up->size = nu;
	myfree((void *)(up+1));
	return freep;
}

#if ALLOCCHECK
static void enterm(void *m, size_t len)
{
	int i;

	for (i=0; i<MAXALLOC; ++i)
	{
		if (alloctab[i].addr == NULL)
		{
			alloctab[i].addr = m;
			alloctab[i].len = len;
			return;
		}
	}
	mprintf("malloc table overflow\n");
}

static int removem(void *m)
{
	int i;
	
	if (m==NULL)
	{
		mprintf("freeing 0\n");
		return FALSE;
	}
	for (i=0; i<MAXALLOC; ++i)
	{
		if (alloctab[i].addr == m)
		{
			alloctab[i].addr = NULL;
			alloctab[i].len = 0;
			return TRUE;
		}
	}
	mprintf("freeing unallocated memory (%p)\n",m);
	return FALSE;
}
#endif

void *malloc(size_t size)
{
	void *m = mymalloc(size);

	if (m==NULL)
	{
#if ALLOCABORT
		fatal("out of memory, needed %lu bytes\n",size);
#endif
		return m;
	}
#if ALLOCCHECK
	enterm(m,size);
#endif
	return m;
}

void *umalloc (size_t size)
{
	void *m = mymalloc(size);

#if ALLOCCHECK
	enterm (m, size);
#endif

	return m;
}

void *calloc(size_t nitems, size_t size)
{
	size_t tsize = nitems*size;
	void *m = malloc(tsize);
	
	if (m!=NULL)
		memset(m,0,tsize);
	return m;
}

void *ucalloc (size_t nitems, size_t size)
{
	size_t tsize = nitems*size;
	void *m = umalloc(tsize);
	
	if (m != NULL) memset (m, 0, tsize);
	return m;
}

static size_t getsize(void *m)
{
	Header *mp = (Header *)m-1;
	
	return (mp->size-1) * sizeof(Header);
}

static size_t min(size_t x,size_t y)
{
	return x<y ? x : y;
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

void *urealloc (void *block, size_t newsize)
{
	void *m;

	if (block==NULL)
		return umalloc (newsize);

	if ((m=umalloc(newsize))==NULL)
		return NULL;
	else
	{
		memcpy(m,block,min(newsize,getsize(block)));
		free(block);
		return m;
	}
}

void free(void *m)
{
#if ALLOCCHECK
	if (removem(m))
#endif
		myfree(m);
}

int alloccheck(void)
{
#if ALLOCCHECK
	int i, retcode = TRUE;
	
	for (i=0; i<MAXALLOC; ++i)
		if (alloctab[i].addr!=NULL)
		{
			retcode = FALSE;
			mprintf("unfreed memory at %p, size=%ld contains %s\n",
				alloctab[i].addr,alloctab[i].len,alloctab[i].addr);
		}
	return retcode;
#else
	return TRUE;
#endif
}

void allocinit(void)
{
#if ALLOCCHECK
	int i;

	for (i=0; i<MAXALLOC; ++i);
	{
		alloctab[i].addr = NULL;
		alloctab[i].len = 0;
	}
#endif
}

void allocexit(void)
{
	int wait = FALSE;
	int maxstack = mstack;

	while (mstack > 0)
	{
		if (Mfree(sysmalloc[mstack-1]))
		{
			mprintf("\nMfree() error (addr=%p, stack=%d, max=%d)",
				sysmalloc[mstack-1],mstack-1,maxstack);
			wait = TRUE;
		}
		--mstack;
	}
	if (wait)
	{
		mprintf("\npress the ANYKEY");
		inchar();
	}
}

size_t coreleft(void)
{
      Header *p;
      size_t sum = 0;
 
      if ((p=freep)==NULL)
	      return 0L;
      for (; p != NULL && p->ptr != freep; p = p->ptr)
	      sum += p->size;
      return sum;
}
