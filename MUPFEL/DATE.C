/*
 * date.c - internal "date" and "touch" commands
 * 05.01.91
 *
 * 21.01.91 loctime.c eingebaut, date +format (jr)
 * stdate killed
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tos.h>

#include "alloc.h"
#include "chario.h"
#include "comm.h"
#include "date.h"
#include "getopt.h"
#include "handle.h"
#include "loctime.h"
#include "mupfel.h"
#include "toserr.h"

#define assign0(x,t,max)	if (t>max) return FALSE; else x=t;
#define assign1(x,t,max)	if (t<1 || t>max) return FALSE; else x=t;

SCCS(date);

xtime systime;

static uint tatoi(char *t,int offs)
{
	static char tmp[] = "??";
	
	strncpy(tmp,&t[offs],2);
	return atoi(tmp);
}

static int convdate(char *timestr, xtime *xt)
{
	int l=(int)strlen(timestr);

	switch (l)
	{
		case 10:
			assign0(xt->s.year,(tatoi(timestr,8)+20)%100,99);
		case 8:
			xt->s.sec = 0;
			assign0(xt->s.min,tatoi(timestr,6),59);
			assign0(xt->s.hour,tatoi(timestr,4),23);
			assign1(xt->s.day,tatoi(timestr,2),31);
			assign1(xt->s.month,tatoi(timestr,0),12);
			break;
		case 4:
			xt->s.sec = 0;
			assign0(xt->s.min,tatoi(timestr,2),59);
			assign0(xt->s.hour,tatoi(timestr,0),23);
			break;
		default:
			eprintf(DT_BADSPEC "\n");
			return FALSE;
	}
	return TRUE;
}

/* set date & time */
static int setdt(char *timestr)
{
	dostime dt;
	dosdate dd;
	
	if (convdate(timestr,&systime))
	{
		/* for old TOS versions, set GEMDOS date/time */
		dd.s.year = systime.s.year;
		dd.s.month = systime.s.month;
		dd.s.day = systime.s.day;
		dt.s.hour = systime.s.hour;
		dt.s.min = systime.s.min;
		dt.s.sec = systime.s.sec;
		Settime(systime.dt);
		Tsetdate(dd.d);
		Tsettime(dt.t);
		print_date (NULL);
		crlf ();
	 	return 0;
	}
	else
	{
		eprintf("date: " DT_BADCONV "\n");
		return printusage(NULL);
	}
}

int m_date (ARGCV)
{
	systime.dt = Gettime();

	if (argc > 2)
		return 2;		/* XXX: error message! */

	if (argc == 1)
	{	
		print_date (NULL);
		crlf ();
		return 0;
	}
	else
		if (argv[1][0] == '+')
		{
			print_date (&argv[1][1]);
			crlf ();
			return 0;
		}
		else
			return setdt(argv[1]);
}

static int touchfile(char *filename,int creatflag)
{
	int hnd;
	dosdate fdate;
	dostime ftime;
	DOSTIME dostime;

	if (isdir(filename))
	{
		eprintf("touch: " DT_ISDIR "\n",filename);
		return 1;
	}	
	hnd = Fopen(filename,0);
	if (bioserror(hnd))
		return ioerror("touch",filename,NULL,hnd);
	if (creatflag && (hnd < MINHND))
		hnd = Fcreate(filename,0);
	if (bioserror(hnd))
		return ioerror("touch",filename,NULL,hnd);
	
	if (hnd<MINHND)
	{
		if (creatflag)
			eprintf("touch: " DT_NONEXIST "\n", filename);
		else
			eprintf("touch: " DT_CANTCRT "\n",filename);
		return FALSE;
	}
	
	fdate.s.day = systime.s.day;
	fdate.s.month = systime.s.month;
	fdate.s.year = systime.s.year;
	ftime.s.hour = systime.s.hour;
	ftime.s.min = systime.s.min;
	ftime.s.sec = systime.s.sec;
	dostime.time = ftime.t;
	dostime.date = fdate.d;
	Fdatime(&dostime,hnd,SETDATE);
	if (dostime.date==0 && dostime.time==0)
		eprintf(DT_CANTSET "\n",filename);
	Fclose(hnd);
	CommInfo.dirty |= drvbit(filename);
	return TRUE;
}

int m_touch(ARGCV)
{
	GETOPTINFO G;
	int c, i;
	static int creatflag;
	int retcode = 0;
	char *datestr = NULL;
	struct option long_option[] =
	{
		{ "no-create", FALSE, &creatflag, FALSE},
		{ "date", TRUE, NULL, 0},
		{ NULL,0,0,0},
	};
	int opt_index = 0;

	creatflag = TRUE;	
	if (argc==1)
		return printusage(long_option);
		
	optinit (&G);
	while ((c = getopt_long (&G, argc, argv, "cd:", long_option,
		&opt_index))!=EOF)
		switch (c)
		{
			case 0:
				if (opt_index == 1)
					goto date;
				break;
			case 'c':
				creatflag = FALSE;
				break;
			case 'd':
			date:
				datestr = strdup (G.optarg);
				break;
			default:
				return printusage(long_option);
		}

	systime.dt = Gettime();

	if (datestr!=NULL && !convdate(datestr,&systime))
	{
		eprintf("touch: " DT_BADCONV "\n");
		free(datestr);
		return printusage(long_option);
	}

	if (G.optind == argc)
	{
		if (datestr)
			free(datestr);
		return printusage(long_option);
	}
			
	for (i = G.optind; i < argc; ++i)
		if (!touchfile(argv[i],creatflag))
			retcode = 1;
		
	if (datestr!=NULL)
		free(datestr);
	return retcode;
}
