/*
 * control.c  -  internal commands for control panel functions
 * 03.04.91
 */
 
#include <stdlib.h>
#include <tos.h>

#include "chario.h"
#include "getopt.h"
#include "mupfel.h"
#include "strsel.h"
#include "sysvec.h"
#include "version.h"

SCCS(control);

static int checkbaud(char *baud)
{
	if (baud==NULL)
		return -1;
		
	switch (atoi(baud))
	{
		case 19200:	return 0;
		case 9600:	return 1;
		case 4800:	return 2;
		case 3600:	return 3;
		case 2400:	return 4;
		case 2000:	return 5;
		case 1800:	return 6;
		case 1200:	return 7;
		case 600:		return 8;
		case 300:		return 9;
		case 200:		return 10;
		case 150:		return 11;
		case 134:		return 12;
		case 110:		return 13;
		case 75:		return 14;
		case 50:		return 15;
		default:		return -1;
	}
}	

int m_rsconf(ARGCV)
{
	GETOPTINFO G;
	int c;
	int parvalue, stops, csize;
	char *baud = NULL;
	int baudrate, proto = -1;
	union
	{
		struct {
			uint clock: 	1;
			uint wordlen:	2;
			uint	stop:	2;
			uint parenb:	1;
			uint evenpar:	1;
			uint unused:   1;
		} s;
		uchar u;
	} ucr;
	static int usexon, userts, noflow;
	struct option long_option[] =
	{
		{ "baudrate", TRUE, NULL, 0},
		{ "xon", FALSE, &usexon, TRUE },
		{ "rts", FALSE, &userts, TRUE },
		{ "noflow", FALSE, &noflow, TRUE },
		{ "parity", TRUE, NULL, 0 },
		{ "stopbits", TRUE, NULL, 0},
		{ "charsize", TRUE, NULL, 0 },
		{ NULL,0,0,0 },
	};
	int opt_index = 0;
	
	usexon = userts = noflow = FALSE;
	parvalue = stops = csize = 0;
	if (argc<2)
		return printusage(long_option);
		
	optinit (&G);
	
	while ((c = getopt_long (&G, argc, argv, "b:xrnp:s:c:",
		long_option,&opt_index)) != EOF)
		switch (c)
		{
			case 0:
				if (G.optarg)
					switch(opt_index)
					{
						case 0: goto baud;
						case 4: goto parity;
						case 5: goto stopbits;
						case 6: goto charsize;
					}
				break;
			case 'b':
			baud:
				baud = G.optarg;
				break;
			case 'x':
				if (proto == -1)
					proto = 0;
				proto |= 1;
				break;
			case 'r':
				if (proto == -1)
					proto = 0;
				proto |= 2;
				break;
			case 'n':
				proto = 0;
				break;
			case 'p':
			parity:
				parvalue = *G.optarg;
				break;
			case 's':
			stopbits:
				stops = *G.optarg;
				break;
			case 'c':
			charsize:
				csize = atoi (G.optarg);
				break;
			default:
				return printusage(long_option);
		}

	if (usexon)
	{
		if (proto == -1)
			proto = 0;
		proto |= 1;
	}
	if (userts)
	{
		if (proto == -1)
			proto = 0;
		proto |= 2;
	}
	if (noflow)
		proto = 0;

	baudrate = checkbaud(baud);
	if (baudrate == -2)
	{
		eprintf("rsconf: " CT_ILLBAUD "\n");
		return 1;
	}

	if (parvalue && strchr("eon",parvalue)==NULL)
	{
		eprintf("rsconf:" CT_ILLPAR "\n");
		return 1;
	}

	if (csize && (csize<5 || csize>8))
	{
		eprintf("rsconf: " CT_ILLCS "\n");
		return 1;
	}

	if (stops && strchr("123",stops)==NULL)
	{
		eprintf("rsconf: " CT_ILLSTOP "\n");
		return 1;
	}

	ucr.u = Rsconf(0,0,-1,-1,-1,-1) >> 24;
	ucr.s.clock = 1;
	ucr.s.unused = 0;
	ucr.s.wordlen = 8-csize;
	if (parvalue)
	{
		ucr.s.parenb = (parvalue != 'n');
		ucr.s.evenpar = (parvalue == 'e');
	}

	switch (stops)
	{
		case '1':
			ucr.s.stop = 1;
			break;
		case '2':
			ucr.s.stop = 3;
			break;
		case '3':
			ucr.s.stop = 2;
			break;
	}

	Rsconf(baudrate,proto,ucr.u,-1,-1,-1);
	return 0;
}

int m_prtconf(ARGCV)
{
	GETOPTINFO G;
	int opt_index = 0, c;
	int config;
	static int matrix, mono, atari, draft, centron, fanfold;
	struct option long_option[] =
	{
		{ "matrix",	  FALSE, &matrix,  TRUE },
		{ "wheel",	  FALSE, &matrix,  FALSE },
		{ "black",	  FALSE, &mono,    TRUE },	
		{ "color",	  FALSE, &mono,    FALSE },
		{ "epson",	  FALSE, &atari,   FALSE },
		{ "atari",	  FALSE, &atari,   TRUE },
		{ "nlq",		  FALSE, &draft,   FALSE },
		{ "draft",	  FALSE, &draft,   TRUE },
		{ "rs232",	  FALSE, &centron, FALSE },
		{ "parallel",    FALSE, &centron, TRUE },
		{ "singlesheet", FALSE, &fanfold, FALSE },
		{ "fanfold",     FALSE, &fanfold, TRUE },
		{ NULL,0,0,0 },
	};

	config  = Setprt(-1);
	matrix  = !(config & 1);
	mono    = !((config & 2) >>1);
	atari   = !((config & 4) >>2);
	draft   = !((config & 8) >>3);
	centron = !((config & 16) >>4);
	fanfold = !((config & 32) >>5);

	if (argc==1)
	{
		mprintf(CT_PRTCONF "\n");
		mprintf("%s" CT_PRINTER "\n",matrix  ? CT_DOTMAT  : CT_DAISY);
		mprintf("%s\n",              mono    ? CT_MONO    : CT_COLOR);
		mprintf("%s" CT_PRINTER "\n",atari   ? CT_ATARI   : CT_EPSON);
		mprintf("%s\n",              draft   ? CT_DRAFT   : CT_LQ);
		mprintf("%s\n",              centron ? CT_CENTR   : CT_RS232);
		mprintf("%s\n",              fanfold ? CT_FANFOLD : CT_SINGLE);
		return 0;
	}
	
	optinit (&G);

	while ((c = getopt_long (&G, argc, argv, "mwbcealdrpsf",
		long_option, &opt_index)) != EOF)
		switch (c)
		{
			case 0: break;
			case 'm': matrix  = TRUE; break;
			case 'w':	matrix  = FALSE; break;
			case 'b': mono    = TRUE; break;
			case 'c': mono    = FALSE; break;
			case 'a': atari   = TRUE; break;
			case 'e': atari   = FALSE; break;
			case 'd':	draft   = TRUE; break;
			case 'l': draft   = FALSE; break;
			case 'p': centron = TRUE; break;
			case 'r': centron = FALSE; break;
			case 'f': fanfold = TRUE; break;
			case 's': fanfold = FALSE; break;
			default:
				return printusage(long_option);
		}
		
	config = !matrix | (!mono<<1) | (!atari<<2) | (!draft<<3) |
			(!centron<<4) | (!fanfold<<5);
	Setprt(config);
	return 0;
}


int m_kbrate(ARGCV)
{
	GETOPTINFO G;
	int oldrate;
	int initial = -1, repeat = -1;
	int c, opt_index = 0;
	struct option long_option[] =
	{
		{ "initial", TRUE, NULL, 0 },
		{ "repeat", TRUE, NULL, 0 },
		{ NULL,0,0,0 },
	};
	
	if (argc==1)
	{
		oldrate = Kbrate(-1,-1);
		initial = oldrate >> 8;
		repeat = oldrate & 0xFF;
		mprintf(CT_KBRATE "\n",
			initial * 20, repeat * 20);
		return 0;
	}
		
	optinit (&G);
	
	while ((c = getopt_long (&G, argc, argv, "i:r:", long_option,
		&opt_index))!=EOF)
		switch (c)
		{
			case 0:
				if (G.optarg)
					switch (opt_index)
					{
						case 0: goto initial;
						case 1: goto repeat;
					}
				break;
			case 'i':
			initial:
				initial = atoi (G.optarg);
				break;
			case 'r':
			repeat:
				repeat = atoi (G.optarg);
				break;
			default:
				return printusage(long_option);
		}
		
	if (initial == 0 || repeat == 0)
	{
		mprintf("kbrate: " CT_KBARG "\n");
		return 1;
	}
	
	oldrate = Kbrate(-1,-1);
	if (initial == -1)
		initial = oldrate >>8;
	else
		initial /= 20;
		
	if (repeat == -1)
		repeat = oldrate & 0xFF;
	else
		repeat /= 20;
		
	Kbrate(initial,repeat);
	return 0;
}

int m_kclick(ARGCV)
{
	int retcode = 0;
	
	if (argc==1)
		mprintf(CT_KEYCLICK "\n",getclick() ? CT_KCON : CT_KCOFF);
	else
	{
		STRSELECT(argv[1])
		WHEN2("on","1")
			keyclick(TRUE);
		WHEN2("off","0")
			keyclick(FALSE);
		DEFAULT
			retcode = printusage(NULL);
		ENDSEL
	}
	return retcode;
}