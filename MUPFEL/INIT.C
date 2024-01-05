/*
 * init.c  -  internal "init" command
 * 01.12.90
 */
 
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>

#include "chario.h"
#include "comm.h"
#include "getopt.h"
#include "init.h"
#include "label.h"
#include "mupfel.h"
#include "toserr.h"

SCCS(init);

/* offsets in bootsector */
#define	NSECTS	0x13		/* sectors/disk */
#define	SPF		0x16		/* sectors/fat */
#define	SPT		0x18		/* sectors/track */
#define	NSIDES	0x1A		/* sides/disk */
#define	CHKSUM	0x1FF	/* low byte of checksum */

int cleartrack(int trk, char *trkbuf, int drv, int sides, int sectors)
{
	int retcode;
	
	retcode = Flopwr(trkbuf, 0L, drv, 1, trk, 0, sectors);
	if (retcode == 0 && sides == 2)
		retcode = Flopwr(trkbuf, 0L, drv, 1, trk, 1, sectors);
	return !retcode;
}

/*
 * store an int in 80x86 format into buf at offset
 */
static void storeint(char *buf, int offset, int value)
{
	buf[offset] = value & 0xFF;
	buf[offset+1] = value >> 8;
}

static int getint(uchar *buf,int offset)
{
	return buf[offset] | (buf[offset+1]<<8);
}

int writebootsector(char *trkbuf,int drv,int sides,int sectors,int tracks)
{
	int success = FALSE;
	int nsects = sides * sectors * tracks;	/* sectors on disk */
	int i, spf;
	uint chk = 0;
	uint *bufp = (uint *)trkbuf;
	
	Protobt(trkbuf,0x1000000L,sides+1,0);

	trkbuf[0] = 0xe9;
	trkbuf[1] = 0x00;
	trkbuf[2] = 0x4e;
	trkbuf[0x15] = 0xF7 + sides;
	storeint(trkbuf,NSECTS,nsects);	/* sectors/disk */
	storeint(trkbuf,SPT,sectors);		/* sectors/track */
	storeint(trkbuf,NSIDES,sides);	/* sides/disk */
	spf = getint((uchar *)trkbuf,SPF);
	/* make sure boot sector isn't executable */
	for (i=0; i<256; ++i)
	{
		chk += *bufp++;
		++i;
	}
	if (chk == 0x1234)
		--trkbuf[CHKSUM];
		
	if (Rwabs(3,trkbuf,1,0,drv-'A')==0)
	{
		/* from mediach.s */
		extern void cdecl mediach(int drv);

		/* write MS-DOS compatible media descr. to FAT */
		memset(trkbuf,0,512);
		trkbuf[0] = 0xF9;
		trkbuf[1] = trkbuf[2] = 0xFF;
		/* 1st FAT at sector 1 */
		Rwabs(3,trkbuf,1,1,drv-'A');
		/* 2nd FAT at sector spf+1 */
		Rwabs(3,trkbuf,1,spf+1,drv-'A');
		/* force media change */
		mediach(drv-'A');
		success = TRUE;
	}
	return success;
}

static int readbootsector(char drv, int *sides, int *sectors, int *nsects, int *tracks)
{
	uchar buf[512];
	
	if (Floprd(buf,0L,drv-'A',1,0,0,1)==0)
	{
		*sectors = getint(buf,SPT);	/* sectors/track */
		*sides = getint(buf,NSIDES);	/* sides/disk */
		*nsects = getint(buf,NSECTS);	/* sectors/disk */
		*tracks = *nsects / (*sides * *sectors);
		return TRUE;
	}
	return FALSE;
}

static int checkvalues(int sides, int sectors, int nsects, int tracks)
{
	if (sides<1 || sides>2)
		return FALSE;
	if (sectors<8 || sectors>15)
		return FALSE;
	if (tracks<70 || tracks>90)
		return FALSE;
	if (nsects<300 || nsects>1800)
		return FALSE;
	return TRUE;
}

static int initdrive(char drv, int sides, int sectors, int tracks, char *label)
{
	char *trkbuf;
	int retcode = 1;
	
	trkbuf = Malloc(8192);
	memset(trkbuf,0,8192);

	if (cleartrack(0,trkbuf,drv-'A',sides,sectors))
		if (cleartrack(1,trkbuf,drv-'A',sides,sectors))
			if (!writebootsector(trkbuf,drv,sides,sectors,tracks))
			{
				oserr = WRITE_FAULT;
				eprintf("init: " IN_WRTBOOT "\n",drv);		
			}
			else
				retcode = 0;

	Mfree(trkbuf);
	if (retcode)
	{
		oserr = WRITE_FAULT;
		eprintf("init: " IN_WRTERR "\n",drv);
	}
	else
	{
		if (label != NULL && strcmp(label,"-"))
			setlabel(drv,label,"init");
		CommInfo.dirty |= 1 << (drv-'A');
	}
	return !retcode;
}
	
m_init(ARGCV)
{
	GETOPTINFO G;
	int c;
	static int sureinit;
	int keepsides, keepsectors, keeplabel;
	char answer, drv;
	int newsides = 2;
	int newsectors = 9;
	int newtracks = 80;
	char *newlabel = NULL;
	struct option long_option[] =
	{
		{ "yes", FALSE, &sureinit, TRUE},
		{ "sides", TRUE, NULL, 0},
		{ "sectors", TRUE, NULL, 0},
		{ "label", TRUE, NULL, 0},
		{ NULL, 0,0,0},
	};
	int opt_index = 0;

	sureinit = FALSE;
	keepsides = keepsectors = keeplabel = TRUE;
		
	if (argc<2)
		return printusage(long_option);
		
	optinit (&G);
	
	while ((c = getopt_long (&G, argc, argv, "ys:c:l:", long_option,
		&opt_index)) != EOF)
		switch (c)
		{
			case 0:
				if (G.optarg)
					switch (opt_index)
					{
						case 1: goto sides;
						case 2: goto sectors;
						case 3: goto label;
					} 
				break;
			case 'y':
				sureinit = TRUE;
				break;
			case 's':
			sides:
				newsides = *G.optarg - '0';
				keepsides = FALSE;
				break;
			case 'c':
			sectors:
				newsectors = atoi (G.optarg);
				keepsectors = FALSE;
				break;
			case 'l':
			label:
				newlabel = G.optarg;
				keeplabel = FALSE;
				break;
			default:
				return printusage(long_option);
		}

	if ((newsides!=1 && newsides!=2) || argc == G.optind)
		return printusage(long_option);
	
	if (newsectors<9 || newsectors>10)
	{
		oserr = ERROR;
		eprintf("init: " IN_ILLSECS "\n");
		return 1;
	}
	
	drv = toupper(*argv[G.optind]);
	if (drv < 'A' || drv > 'B')
	{
		oserr = EACCDN;
		eprintf("init: " IN_ILLDRV "\n",drv);
		return 1;
	}

	if (!shellcmd && !sureinit)
	{
		mprintf(IN_CONFIRM,drv);
		answer = charselect(IN_CHOICE);
		if (answer == 'J')
			answer = 'Y';
	}
	else
		answer = 'Y';
		
	if (answer=='Y')
	{
		if (keepsides || keepsectors)
		{
			int nsects, sides, sectors, tracks;
			
			if (!readbootsector(drv,&sides,&sectors,&nsects,&tracks))
			{
				oserr = ERROR;
				eprintf("init: " IN_READERR "\n",drv);
				return 1;
			}
			if (!checkvalues(sides,sectors,nsects,tracks))
			{
				oserr = ERROR;
				eprintf("init: " IN_NONTOS "\n");
				return 1;
			}
			if (keepsides)
				newsides = sides;
			if (keepsectors)
				newsectors = sectors;
			newtracks = tracks;
		}
		if (keeplabel)
			newlabel = getlabel(drv);
		return !initdrive(drv,newsides,newsectors,newtracks,newlabel);
	}
	else
		return 0;
}
