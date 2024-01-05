/*
 * mkdir.c  -  internal "mkdir" command
 * 11.09.90
 */
 
#include <string.h>
#include <tos.h>

#include "alloc.h"
#include "chario.h"
#include "comm.h"
#include "getopt.h"
#include "mkdir.h"
#include "mupfel.h"
#include "toserr.h"

SCCS(mkdir);

static int pathmkdir;

static int domkdir(const char *dir)
{
	if (access(dir,A_READ) || (bioserr ? FALSE : isdir(dir)))
	{
		oserr = EACCDN;
		eprintf("mkdir: " MK_EXISTS "\n",dir);
		return FALSE;
	}
	if (bioserr)
		return ioerror("mkdir",dir,NULL,bioserr);
	return Dcreate(dir)>=0;
}

int mkdir(char *dir)
{
	int direxist, retcode = TRUE;
	char *dirdup, *dirpart, *d;
	
	strlwr(dir);
	if (lastchr(dir)=='\\' && strlen(dir)>1)
		dir[strlen(dir)-1] = '\0';
	
	if (pathmkdir)
	{
		dirdup = strdup(dir);
		dirpart = strdup(dir);

		*dirpart = '\0';
		d = strtok(dirdup,"\\");
		while (d != NULL)
		{
			strcat(dirpart,d);
			direxist = isdir(dirpart);
			if (bioserr)
			{
				ioerror("mkdir",dirpart,NULL,bioserr);
				retcode = FALSE;
				break;
			}
			if (!direxist)
				if (!domkdir(dirpart))
				{
					retcode = FALSE;
					break;
				}
			d = strtok(NULL,"\\");
			chrapp(dirpart,'\\');
		}
		free(dirpart);
		free(dirdup);
		return retcode;
	}
	else
	{
		if (!domkdir(dir))
		{
			oserr = EACCDN;
			eprintf("mkdir: " MK_CANTCRT "\n",dir);
			return FALSE;
		}
	}
	CommInfo.dirty |= drvbit(dir);
	return retcode;
}

int m_mkdir(ARGCV)
{
	GETOPTINFO G;
	int i, c, retcode = 0;
	struct option long_option[] =
	{
		{ "path", FALSE, &pathmkdir, TRUE },
		{ NULL,0,0,0 },
	};
	int opt_index;

	pathmkdir = FALSE;
	optinit (&G);
	
	while ((c = getopt_long (&G, argc, argv, "p", long_option,
		&opt_index)) != EOF)
		switch(c)
		{
			case 0:
				break;
			case 'p':
				pathmkdir = TRUE;
				break;
			default:
				return printusage(long_option);
		}	

	if (G.optind == argc)
	{
		pathmkdir = FALSE;
		return printusage(long_option);
	}
		
	for (i = G.optind; i < argc && retcode == 0; ++i)
		if (!mkdir(argv[i]))
			retcode = 1;
	pathmkdir = FALSE;
	return retcode;
}
