/*
 * print.c  -  internal "print" command
 * 06.04.91
 */

#include <stdlib.h>
#include <tos.h>

#include "chario.h"
#include "filebuf.h"
#include "getopt.h"
#include "handle.h"
#include "mupfel.h"
#include "keys.h"
#include "sysvec.h"

SCCS(print);

#define PRINT_TEST	0	/* set to 1 to force output to CON */

static enum { PRT, AUX, CON } prt_dev = PRT;
static int lines, tabsize;

static int timer(long count)
{
	static long starttime = 0L;
	static long stoptime = 0L;
	
	if (count)
	{
		stoptime = count * 20L;
		starttime = gethz200();
		return TRUE;
	}
	else
	{
		count = gethz200() - starttime;
		return count <= stoptime;
	}
}

static int prnout(int c)
{
	if (Bcostat(prt_dev) == 0L)
	{
		timer(100);			/* start timer  10 Sek. */
		while (timer(0))
		{
			if (Bcostat(prt_dev) != 0L)
			{
				Bconout(prt_dev,c);
				return TRUE;
			}
			if (checkintr())
				return FALSE;
		}
		return FALSE;
	}
	Bconout(prt_dev,c);
	return TRUE;
}

static int printfile(const char *file,int formfeed)
{
	int handle;
	int c, printintr = FALSE;
	int linecount = 0;
	char *buffer, *buf;
	size_t end;
	int column;
	
	if (Bcostat(prt_dev) == 0L)
	{
		eprintf("print: " PR_NORESP "\n");
		return FALSE;
	}
	if ((handle=Fopen(file,0))<MINHND)
	{
		eprintf("print: " PR_CANTOPEN "\n",file);
		return 1;
	}
	if ((end=filebuf(handle,file,&buffer,"print"))==BUF_ERR)
		return 1;
	buf = buffer;
	column = 0;
	while (end > 0 && !printintr)
	{
		c = *buf++;
		--end;

		if (c=='\t' && tabsize > 0)
		{
			do
			{
				if (!prnout(' '))
					goto printabort;
				++column;
			} while (column % tabsize);
		}
		else
		{
			if (!prnout(c))
				goto printabort;
			++column;
		}
		
		if (c=='\n')
		{
			printintr = checkintr();
			++linecount;
			column = 0;
		}
		if (linecount==lines)
		{
			linecount = 0;
			if (!prnout('\f'))
				goto printabort;
		}
	}
	Mfree(buffer);
	if (formfeed && !prnout('\f'))
		return 1;
	return 0;
printabort:
	Mfree(buffer);
	return 1;
}

int m_print(ARGCV)
{
	GETOPTINFO G;
	int i, c, prt_config, retcode = 0;
	static int formfeed;
	struct option long_option[] =
	{
		{ "formfeed", FALSE, &formfeed, TRUE },
		{ "lines", TRUE, NULL, 0 },
		{ "tabsize", TRUE, NULL, 0 },
		{ NULL,0,0,0 },
	};
	int opt_index = 0;
	
	optinit (&G);
	lines = -1;
	tabsize = -1;
	formfeed = FALSE;
	while ((c = getopt_long (&G, argc, argv, "fl:t:", long_option,
		&opt_index)) != EOF)
		switch(c)
		{
			case 0:
				if (G.optarg)
					switch(opt_index)
					{
						case 1: goto lines;
						case 2: goto tabsize;
					}
				break;
			case 'l':
			lines:
				lines = atoi (G.optarg);
				break;
			case 'f':
				formfeed = TRUE;
				break;
			case 't':
			tabsize:
				tabsize = atoi (G.optarg);
				break;
			default:
				return printusage(long_option);
		}

	if (G.optind==argc || lines==0 || tabsize==0)
		return printusage(long_option);

	prt_config  = Setprt(-1);
	if (prt_config & 16)
		prt_dev = AUX;
	else
		prt_dev = PRT;
#if PRINT_TEST
	prt_dev = CON;
#endif

	for (i = G.optind; i < argc; ++i)
		if (printfile(argv[i],formfeed))
			retcode = 1;
			
	return retcode;
}
