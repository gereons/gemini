/*
 * wc.c  -  internal "wc" function
 * 26.08.90
 */

#include <stddef.h>
#include <tos.h>

#include "chario.h"
#include "filebuf.h"
#include "getopt.h"
#include "handle.h"
#include "mupfel.h"
#include "redir.h"

#define OUT	0
#define IN	1

SCCS(wc);

static int wordcnt, linecnt, charcnt;
static long wsum, lsum, csum;

static void wcvals(char *str, long l, long w, long c)
{
	if (*str)
		mprintf("%s: ",str);
	if (linecnt)
		mprintf("%ld ",l);
	if (wordcnt)
		mprintf("%ld ",w);
	if (charcnt)
		mprintf("%ld",c);
	crlf();
}

static int wc(char *file,int showname)
{
	int handle;
	int c, state;
	long nc, nl, nw;
	char *buffer, *bufsave;
	size_t end;
	
	if ((handle=Fopen(file,0))<MINHND)
	{
		eprintf("wc: " WC_CANTOPEN "\n",file);
		return 1;
	}
	else
	{
		if ((end=filebuf(handle,file,&buffer,"wc"))==BUF_ERR)
			return 1;
		bufsave = buffer;
		state = OUT;
		nc = nl = nw = 0;
		while (end > 0)
		{
			c = *buffer++;
			--end;
			++nc;
			if (c=='\n')
				++nl;
			if (c==' ' || c=='\n' || c=='\t')
				state=OUT;
			else
				if (state==OUT)
				{
					state=IN;
					++nw;
				}
		}
		Mfree(bufsave);
	}
	wcvals(showname ? file : "",nl,nw,nc);
	lsum += nl;
	wsum += nw;
	csum += nc;
	return 0;
}

int
m_wc(ARGCV)
{
	GETOPTINFO G;
	int c, showname;
	struct option long_option[] =
	{
		{ "linecount", FALSE, &linecnt, TRUE },
		{ "charcount", FALSE, &charcnt, TRUE },
		{ "wordcount", FALSE, &wordcnt, TRUE },
		{ NULL,0,0,0 },
	};
	int opt_index = 0;
	
	wordcnt = linecnt = charcnt = FALSE;
	wsum = lsum = csum = 0;
	optinit (&G);

	while ((c = getopt_long (&G, argc, argv, "lcw", long_option,
		&opt_index)) != EOF)
		switch (c)
		{
			case 0:
				break;
			case 'l':
				linecnt=TRUE;
				break;
			case 'w':
				wordcnt=TRUE;
				break;
			case 'c':
				charcnt=TRUE;
				break;
			default:
				return printusage(long_option);
		}
	
	if (!wordcnt && !linecnt && !charcnt)
		wordcnt=linecnt=charcnt=TRUE;
	
	showname = G.optind < argc - 1;
	if (G.optind < argc)
	{
		for (; G.optind<argc && !intr(); ++G.optind)
			wc(argv[G.optind],showname);
	}
	else
	{
		if (redirect.in.file)
			wc(redirect.in.file,FALSE);
		else
			return printusage(long_option);
	}
	if (!intr() && showname)
		wcvals("total",lsum,wsum,csum);
	return 0;
}
