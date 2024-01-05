/*
 * free.c  -  internal "free" command
 * 24.09.90
 */
 
#include <tos.h>

#include "chario.h"
#include "getopt.h"
#include "mupfel.h"

#define MEMBLKSIZE	20

static void showfree(int longlist)
{
	struct
	{
		void *addr;
		size_t size;
	} memblk[MEMBLKSIZE];
	int i,j;
	size_t s, freemem = 0, largest = 0;
	
	i = 0;
	while (i<MEMBLKSIZE && (s = (size_t)Malloc(-1L)) != 0)
	{
		memblk[i].addr = Malloc(s);
		memblk[i].size = s;
		freemem += s;
		if (s > largest)
			largest = s;
		++i;
	}
	if (i == MEMBLKSIZE)
		eprintf("free: " FR_FRAG "\n");
	for (j=0; j<i; ++j)
		Mfree(memblk[j].addr);
	if (longlist)
	{
		mprintf(FR_HEADER "\n");
		for (j=0; j<i && !intr(); ++j)
			mprintf("%-2d  0x%08p %8lu\n",
				j+1,memblk[j].addr,memblk[j].size);
	}
	mprintf(FR_INFO "\n",freemem,freemem/1024L,i,largest,largest/1024L);
}

int m_free(ARGCV)
{
	GETOPTINFO G;
	int c;
	static int longlist;
	struct option long_option[] =
	{
		{ "long", FALSE, &longlist, TRUE },
		{ NULL,0,0,0 },
	};
	int opt_index;

	longlist = FALSE;
	optinit (&G);

	while ((c = getopt_long (&G, argc, argv, "l", long_option,
		&opt_index)) != EOF)
		switch (c)
		{
			case 0:
				break;
			case 'l':
				longlist = TRUE;
				break;
			default:
				return printusage(long_option);
		}
		
	showfree(longlist);
	return 0;
}