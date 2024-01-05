/*
 * getopt.c  -  option letter handling
 * public domain implementation of UNIX's getopt(3C) by Henry Spencer
 * 15.05.90
 */
 
#include <ctype.h>
#include <string.h>

#include "chario.h"
#include "getopt.h"
#include "mupfel.h"

#define ARGCH	(int)':'
#define BADCH	(int)'?'
#define EMSG	""

SCCS(getopt);

int opterr, optind, optopt;
char *optarg;
static char *place;

#define tell(s) mprint("%s%s%c\n",*nargv,s,optopt); return BADCH;

void optinit(void)
{
	opterr = optind = 1;
	place = EMSG;
}

int getopt(int nargc, char **nargv, char *ostr)
{
	char *oli;
	
	if (!*place)
	{
		if (optind >= nargc || *(place = nargv[optind]) != '-' ||  !*++place)
			return (EOF);
		if (*place == '-')
		{
			++optind;
			return EOF;
		}
	}
	
	if ((optopt=(int)*place++) == ARGCH ||
	    (oli=strchr(ostr,optopt))==NULL)
	{
		if (!*place)
			++optind;
		tell(GO_ILLOPT);
	}
	
	if (*++oli != ARGCH)
	{
		optarg = NULL;
		if (!*place)
			++optind;
	}
	else
	{
		if (*place)
			optarg = place;
		else
			if (nargc <= ++optind)
			{
				place = EMSG;
				tell(GO_OPTARG);
			}
			else
				optarg = nargv[optind];
			place = EMSG;
		++optind;
	}
	return optopt;
}
