/*
 * format.c  -  internal "format" command
 * 09.02.91
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>

#include "attrib.h"
#include "chario.h"
#include "comm.h"
#include "getopt.h"
#include "init.h"
#include "mupfel.h"
#include "version.h"
#include "stand.h"
#include "toserr.h"

SCCS(format);

static int *spiral(int max_side, int track, int side, int spt)
{
	uint start_sector, spiral;
	int i;
	static int sector_field[18];

	spiral = spt - 3;

	start_sector = (1+(((track+1)*max_side-1+side)-1)*spiral) % spt;
	if (start_sector == 0)
		start_sector = 1;
	for (i=0; i<spt; ++i)
		sector_field[i] = ((start_sector+i-1) % spt)+1;

	return sector_field;
}

static int formatdrive(int drv,int sides, int sectors,int verbose)
{
	char *buf = Malloc(8192);
	int trk, side, err;
	int retcode = FALSE;
	long filler;
	int intleave;
	
	if (buf==NULL)
	{
		oserr = ENSMEM;
		eprintf("format: " FM_NOROOM "\n");
		return FALSE;
	}
	for (trk=79; trk>=0; --trk)
	{
		for (side=sides-1; side>=0; --side)
		{
#if MERGED
			if (CommInfo.fun.fmt_progress)
				CommInfo.fun.fmt_progress(
					1000 - (int)(((trk * 2 + side) * 1000L) / 160L));
#endif
			if (verbose)
			{
				canout('\r');
				mprintf(FM_INFO,trk,side);
			}
			if (tosversion() >= 0x102)
			{
				filler = (long)spiral(sides,trk,side,sectors);
				intleave = -1;
			}
			else
			{
				filler = 0L;
				intleave = 1;
			}
			err = Flopfmt(buf,filler,drv-'A',sectors,trk,side,
					intleave,0x87654321L,0xe5e5);
			if (err)
			{
				oserr = WRITE_FAULT;
				if (verbose)
					crlf();
				eprintf("format: " FM_FMTERR "\n",trk);
				Mfree(buf);
				return FALSE;
			}
			if (checkintr())
			{
				if (verbose)
					crlf();
				oserr = ERROR;
				eprintf("format: " FM_ABORT "\n");
				Mfree(buf);
				return FALSE;
			}
		}
	}

	if (verbose)
		crlf();
	/* write empty root dir, FAT and boot sector */
	memset(buf,0,8192);

	if (cleartrack(0,buf,drv-'A',sides,sectors))
		if (cleartrack(1,buf,drv-'A',sides,sectors))
			if (!writebootsector(buf,drv,sides,sectors,80))
			{
				oserr = WRITE_FAULT;
				eprintf("format: " FM_WRTBOOT "\n",drv);
			}
			else
				retcode = TRUE;
	if (!retcode)
		eprintf("format: " FM_WRTFAT "\n",drv);
		
	Mfree(buf);
	return retcode;
}

int m_format(ARGCV)
{
	GETOPTINFO G;
	int c;
	int drv, sides = '2';
	int sectors = 9;
	static int sureformat, verbose;
	char answer;
	char *label = NULL;
	struct option long_option[] =
	{
		{ "verbose", FALSE, &verbose, TRUE},
		{ "yes", FALSE, & sureformat, TRUE},
		{ "sides", TRUE, NULL, 0},
		{ "sectors", TRUE, NULL, 0},
		{ "label", TRUE, NULL, 0},
		{ NULL,0,0,0 },
	};
	int opt_index = 0;
	
	verbose = sureformat = FALSE;
	optinit (&G);

	while ((c = getopt_long (&G, argc, argv, "vys:c:l:", long_option,
		&opt_index)) != EOF)
		switch (c)
		{
			case 0:
				if (G.optarg)
					switch(opt_index)
					{
						case 2: goto sides;
						case 3: goto label;
						case 4: goto sectors;
					}
				break;
			case 'v':
				verbose = TRUE;
				break;
			case 's':
			sides:
				sides = *G.optarg;
				break;
			case 'y':
				sureformat = TRUE;
				break;
			case 'c':
			sectors:
				sectors = atoi (G.optarg);
				break;
			case 'l':
			label:
				label = G.optarg;
				break;
			default:
				return printusage(long_option);
		}

	if (sides!='1' && sides!='2' || argc == G.optind)
		return printusage(long_option);
	sides -= '0';
	
	if (sectors<9 || sectors>10)
	{
		oserr = EACCDN;
		eprintf("format: " FM_ILLSECS "\n");
		return 1;
	}
	
	drv = toupper(*argv[G.optind]);
	if (drv < 'A' || drv > 'B')
	{
		oserr=EACCDN;
		eprintf("format: " FM_ILLDRV "\n",drv);
		return 1;
	}

	if (!shellcmd && !sureformat)
	{
		eprintf(FM_CONFIRM,drv);
		answer = charselect(FM_CHOICE);
		if (answer == 'J')
			answer = 'Y';
	}
	else
		answer = 'Y';
		
	if (answer=='Y')
	{
		if (formatdrive(drv,sides,sectors,verbose))
		{
			if (label!=NULL)
			{
				char lbl[16];

				sprintf(lbl,"%c:\\",drv);
				strncat(lbl,label,11);
				if (Fcreate(lbl,FA_LABEL)<0)
				{
					oserr = EACCDN;
					eprintf("format: " FM_WRTLBL "\n",drv);
				}
				return 1;
			}
		}
		else
			return 1;
	}
	return 0;
}

