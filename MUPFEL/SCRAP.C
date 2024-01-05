/*
 * scrap.c  -  internal "setscrap" command
 * 26.08.90
 */
 
#include <aes.h>
#include <string.h>

#include "alloc.h"
#include "chario.h"
#include "getopt.h"
#include "mupfel.h"

SCCS(scrap);

static int forcescrap;
static int quiet;

static int showscrap(void)
{
	char scrap[100];
	
	if (!scrp_read(scrap))
	{
		eprintf("setscrap: " SC_CANTGET "\n");
		return FALSE;
	}
	if (*scrap)
		mprintf(SC_CURRSCRP "\n",scrap);
	else
		mprintf(SC_NOSCRP "\n");
	return TRUE;
}

static int setscrap(const char *scrap)
{
	int retcode;
	char newscrap[100];
	
	if (!scrp_read(newscrap) && !quiet)
	{
		oserr = -1;
		eprintf("setscrap: " SC_CANTGET "\n");
		return FALSE;
	}
	if (!forcescrap && *newscrap)
	{
		if (!quiet)
		{
			oserr = -1;
			mprintf("setscrap: " SC_OLDSCRP "\n",newscrap);
		}
		return FALSE;
	}
	if (!quiet && (!*scrap || *scrap=='.' || scrap[1]!=':'))
	{
		oserr = -1;
		eprintf("setscrap: " SC_ABSPATH "\n");
		return FALSE;
	}
	if (!quiet && !isdir(scrap))
	{
		oserr = -1;
		eprintf("setscrap: " SC_NODIR "\n",scrap);
		return FALSE;
	}
	strcpy(newscrap,scrap);
	strupr(newscrap);
	retcode = scrp_write(newscrap);
	if (!quiet && !retcode)
	{
		oserr = -1;
		eprintf("setscrap: " SC_CANTSET "\n");	
	}
	return retcode;
}

int m_setscrap(ARGCV)
{
	GETOPTINFO G;
	int c;
	struct option long_option[] =
	{
		{ "force", FALSE, &forcescrap, TRUE },
		{ "quiet", FALSE, &quiet, TRUE },
		{ NULL,0,0,0 },
	};
	int opt_index;
	
	forcescrap = quiet = FALSE;
	optinit (&G);
	
	while ((c = getopt_long (&G, argc, argv, "fq", long_option,
		&opt_index)) != EOF)
		switch (c)
		{
			case 0:
				break;
			case 'f':
				forcescrap = TRUE;
				break;
			case 'q':
				quiet = TRUE;
				break;
			default:
				return printusage(long_option);
		}

	switch (argc - G.optind)
	{
		case 0:
			return !showscrap();
		case 1:
			return !setscrap(argv[G.optind]);
		default:
			return printusage(long_option);
	}
}
